#include <controller.h>
#include <cache.h>
#include <sqlite3.h>
#include <cstdlib>
#include <cstring>
#include <configcontainer.h>

#include <sstream>
#include <fstream>
#include <iostream>
#include <cassert>
#include <rss.h>
#include <logger.h>
#include <config.h>
#include <exceptions.h>
#include <utils.h>

namespace newsbeuter {

struct cb_handler {
		cb_handler() : c(-1) { }
		void set_count(int i) {
			c = i;
		}
		int count() {
			return c;
		}
	private:
		int c;
};

struct header_values {
	time_t lastmodified;
	std::string etag;
};

static int count_callback(void * handler, int argc, char ** argv, char ** /* azColName */) {
	cb_handler * cbh = static_cast<cb_handler *>(handler);

	if (argc>0) {
		std::istringstream is(argv[0]);
		int x;
		is >> x;
		cbh->set_count(x);
	}

	return 0;
}

static int single_string_callback(void * handler, int argc, char ** argv, char ** /* azColName */) {
	std::string * value = reinterpret_cast<std::string *>(handler);
	if (argc>0 && argv[0]) {
		*value = argv[0];
	}
	return 0;
}

static int rssfeed_callback(void * myfeed, int argc, char ** argv, char ** /* azColName */) {
	std::shared_ptr<rss_feed>* feed = static_cast<std::shared_ptr<rss_feed>* >(myfeed);
	// normaly, this shouldn't happen, but we keep the assert()s here nevertheless
	assert(argc == 3);
	assert(argv[0] != nullptr);
	assert(argv[1] != nullptr);
	assert(argv[2] != nullptr);
	(*feed)->set_title(argv[0]);
	(*feed)->set_link(argv[1]);
	(*feed)->set_rtl(strcmp(argv[2],"1")==0);
	LOG(LOG_INFO, "rssfeed_callback: title = %s link = %s is_rtl = %s",argv[0],argv[1], argv[2]);
	return 0;
}

static int lastmodified_callback(void * handler, int argc, char ** argv, char ** /* azColName */) {
	header_values * result = static_cast<header_values *>(handler);
	assert(argc == 2);
	assert(result != nullptr);
	if (argv[0]) {
		std::istringstream is(argv[0]);
		is >> result->lastmodified;
	} else {
		result->lastmodified = 0;
	}
	if (argv[1]) {
		result->etag = argv[1];
	} else {
		result->etag = "";
	}
	LOG(LOG_INFO, "lastmodified_callback: lastmodified = %d etag = %s", result->lastmodified, result->etag.c_str());
	return 0;
}

static int vectorofstring_callback(void * vp, int argc, char ** argv, char ** /* azColName */) {
	std::vector<std::string> * vectorptr = static_cast<std::vector<std::string> *>(vp);
	assert(argc == 1);
	assert(argv[0] != nullptr);
	vectorptr->push_back(std::string(argv[0]));
	LOG(LOG_INFO, "vectorofstring_callback: element = %s", argv[0]);
	return 0;
}

static int rssitem_callback(void * myfeed, int argc, char ** argv, char ** /* azColName */) {
	std::shared_ptr<rss_feed>* feed = static_cast<std::shared_ptr<rss_feed>* >(myfeed);
	assert (argc == 13);
	std::shared_ptr<rss_item> item(new rss_item(nullptr));
	item->set_guid(argv[0]);
	item->set_title(argv[1]);
	item->set_author(argv[2]);
	item->set_link(argv[3]);

	std::istringstream is(argv[4]);
	time_t t;
	is >> t;
	item->set_pubDate(t);

	item->set_size(utils::to_u(argv[5]));
	item->set_unread((std::string("1") == argv[6]));

	item->set_feedurl(argv[7]);

	item->set_enclosure_url(argv[8] ? argv[8] : "");
	item->set_enclosure_type(argv[9] ? argv[9] : "");
	item->set_enqueued((std::string("1") == (argv[10] ? argv[10] : "")));
	item->set_flags(argv[11] ? argv[11] : "");
	item->set_base(argv[12] ? argv[12] : "");

	//(*feed)->items().push_back(item);
	(*feed)->add_item(item);
	return 0;
}

static int fill_content_callback(void * myfeed, int argc, char ** argv, char ** /* azColName */) {
	rss_feed * feed = static_cast<rss_feed *>(myfeed);
	assert(argc == 2);
	if (argv[0]) {
		std::shared_ptr<rss_item> item = feed->get_item_by_guid_unlocked(argv[0]);
		item->set_description(argv[1] ? argv[1] : "");
	}
	return 0;
}

static int search_item_callback(void * myfeed, int argc, char ** argv, char ** /* azColName */) {
	std::vector<std::shared_ptr<rss_item>> * items = static_cast<std::vector<std::shared_ptr<rss_item>> *>(myfeed);
	assert (argc == 13);
	std::shared_ptr<rss_item> item(new rss_item(nullptr));
	item->set_guid(argv[0]);
	item->set_title(argv[1]);
	item->set_author(argv[2]);
	item->set_link(argv[3]);

	std::istringstream is(argv[4]);
	time_t t;
	is >> t;
	item->set_pubDate(t);

	item->set_size(utils::to_u(argv[5]));
	item->set_unread((std::string("1") == argv[6]));
	item->set_feedurl(argv[7]);

	item->set_enclosure_url(argv[8] ? argv[8] : "");
	item->set_enclosure_type(argv[9] ? argv[9] : "");
	item->set_enqueued((std::string("1") == argv[10]));
	item->set_flags(argv[11] ? argv[11] : "");
	item->set_base(argv[12] ? argv[12] : "");

	items->push_back(item);
	return 0;
}



cache::cache(const std::string& cachefile, configcontainer * c) : db(0),cfg(c) {
	int error = sqlite3_open(cachefile.c_str(),&db);
	if (error != SQLITE_OK) {
		LOG(LOG_ERROR,"couldn't sqlite3_open(%s): error = %d", cachefile.c_str(), error);
		throw dbexception(db);
	}

	populate_tables();
	set_pragmas();

	clean_old_articles();

	// we need to manually lock all DB operations because SQLite has no explicit support for multithreading.
}

cache::~cache() {
	sqlite3_close(db);
}

void cache::set_pragmas() {
	int rc;

	// first, we need to swithc off synchronous writing as it's slow as hell
	rc = sqlite3_exec(db, "PRAGMA synchronous = OFF;", nullptr, nullptr, nullptr);

	if (rc != SQLITE_OK) {
		LOG(LOG_CRITICAL,"setting PRAGMA synchronous = OFF failed");
		throw dbexception(db);
	}

	// then we disable case-sensitive matching for the LIKE operator in SQLite, for search operations
	rc = sqlite3_exec(db, "PRAGMA case_sensitive_like=OFF;", nullptr, nullptr, nullptr);

	if (rc != SQLITE_OK) {
		LOG(LOG_CRITICAL,"setting PRAGMA case_sensitive_like = OFF failed");
		throw dbexception(db);
	}

}

static const schema_queries_list schemaQueries {
	"CREATE TABLE rss_feed ( "
	" rssurl VARCHAR(1024) PRIMARY KEY NOT NULL, "
	" url VARCHAR(1024) NOT NULL, "
	" title VARCHAR(1024) NOT NULL ); ",

	"CREATE TABLE rss_item ( "
	" id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
	" guid VARCHAR(64) NOT NULL, "
	" title VARCHAR(1024) NOT NULL, "
	" author VARCHAR(1024) NOT NULL, "
	" url VARCHAR(1024) NOT NULL, "
	" feedurl VARCHAR(1024) NOT NULL, "
	" pubDate INTEGER NOT NULL, "
	" content VARCHAR(65535) NOT NULL,"
	" unread INTEGER(1) NOT NULL );",

	 "CREATE TABLE google_replay ( "
	" id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
	" guid VARCHAR(64) NOT NULL, "
	" state INTEGER NOT NULL, "
	" ts INTEGER NOT NULL );",

	/* we need to do these ALTER TABLE statements because we need to store
	 * additional data for the podcast support */
	"ALTER TABLE rss_item ADD enclosure_url VARCHAR(1024);",

	"ALTER TABLE rss_item ADD enclosure_type VARCHAR(1024);",

	"ALTER TABLE rss_item ADD enqueued INTEGER(1) NOT NULL DEFAULT 0;",

	"ALTER TABLE rss_item ADD flags VARCHAR(52);",

	/* create indexes to speed up certain queries */
	"CREATE INDEX IF NOT EXISTS idx_rssurl ON rss_feed(rssurl);",

	"CREATE INDEX IF NOT EXISTS idx_guid ON rss_item(guid);",

	"CREATE INDEX IF NOT EXISTS idx_feedurl ON rss_item(feedurl);",
	/* we analyse the indices for better statistics */
	"ANALYZE;",

	"ALTER TABLE rss_feed ADD lastmodified INTEGER(11) NOT NULL DEFAULT 0;",

	"CREATE INDEX IF NOT EXISTS idx_lastmodified ON rss_feed(lastmodified);",

	"ALTER TABLE rss_item ADD deleted INTEGER(1) NOT NULL DEFAULT 0;",

	"CREATE INDEX IF NOT EXISTS idx_deleted ON rss_item(deleted);",

	"ALTER TABLE rss_feed ADD is_rtl INTEGER(1) NOT NULL DEFAULT 0;",

	"ALTER TABLE rss_feed ADD etag VARCHAR(128) NOT NULL DEFAULT \"\";",

	"ALTER TABLE rss_item ADD base VARCHAR(128) NOT NULL DEFAULT \"\";",
};

void cache::populate_tables() {
	int rc;
	for (const auto& query : schemaQueries) {
		rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
		LOG(LOG_DEBUG, "cache::populate_tables: %s rc = %d",
			query.c_str(), rc);
	}
}

void cache::fetch_lastmodified(const std::string& feedurl, time_t& t, std::string& etag) {
	std::lock_guard<std::mutex> lock(mtx);
	std::string query = prepare_query("SELECT lastmodified, etag FROM rss_feed WHERE rssurl = '%q';", feedurl.c_str());
	LOG(LOG_DEBUG, "running: query: %s", query.c_str());
	header_values result = { 0, "" };
	int rc = sqlite3_exec(db, query.c_str(), lastmodified_callback, &result, nullptr);
	if (rc != SQLITE_OK) {
		LOG(LOG_CRITICAL, "query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	}
	t = result.lastmodified;
	etag = result.etag;
	LOG(LOG_DEBUG, "cache::fetch_lastmodified: t = %d etag = %s", t, etag.c_str());
}

void cache::update_lastmodified(const std::string& feedurl, time_t t, const std::string& etag) {
	if (t == 0 && etag.length() == 0) {
		LOG(LOG_INFO, "cache::update_lastmodified: both time and etag are empty, not updating anything");
		return;
	}
	std::lock_guard<std::mutex> lock(mtx);
	std::string query = "UPDATE rss_feed SET ";
	if (t > 0)
		query.append(utils::strprintf("lastmodified = '%d'", t));
	if (etag.length() > 0)
		query.append(utils::strprintf("%c etag = %s", (t > 0 ? ',' : ' '), prepare_query("'%q'", etag.c_str()).c_str()));
	query.append(" WHERE rssurl = ");
	query.append(prepare_query("'%q'", feedurl.c_str()));
	int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
	LOG(LOG_DEBUG, "ran SQL statement: %s result = %d", query.c_str(), rc);
}

void cache::mark_item_deleted(const std::string& guid, bool b) {
	std::lock_guard<std::mutex> lock(mtx);
	std::string query = prepare_query("UPDATE rss_item SET deleted = %u WHERE guid = '%q'", b ? 1 : 0, guid.c_str());
	int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
	LOG(LOG_DEBUG, "cache::mark_item_deleted ran SQL statement: %s result = %d", query.c_str(), rc);
}


std::vector<std::string> cache::get_feed_urls() {
	std::lock_guard<std::mutex> lock(mtx);
	std::string query = "SELECT rssurl FROM rss_feed;";

	std::vector<std::string> urls;

	int rc = sqlite3_exec(db, query.c_str(), vectorofstring_callback,&urls, nullptr);
	assert(rc == SQLITE_OK);

	return urls;
}


// this function writes an rss_feed including all rss_items to the database
void cache::externalize_rssfeed(std::shared_ptr<rss_feed> feed, bool reset_unread) {
	scope_measure m1("cache::externalize_feed");
	if (feed->rssurl().substr(0,6) == "query:")
		return;

	std::lock_guard<std::mutex> lock(mtx);
	std::lock_guard<std::mutex> feedlock(feed->item_mutex);
	//scope_transaction dbtrans(db);

	cb_handler count_cbh;
	int rc = sqlite3_exec(db,prepare_query("SELECT count(*) FROM rss_feed WHERE rssurl = '%q';", feed->rssurl().c_str()).c_str(),count_callback,&count_cbh,nullptr);
	if (rc != SQLITE_OK) {
		LOG(LOG_CRITICAL,"query failed: error = %d", rc);
		throw dbexception(db);
	}

	int count = count_cbh.count();
	LOG(LOG_DEBUG, "cache::externalize_rss_feed: rss_feeds with rssurl = '%s': found %d",feed->rssurl().c_str(), count);
	if (count > 0) {
		std::string updatequery = prepare_query("UPDATE rss_feed SET title = '%q', url = '%q', is_rtl = %u WHERE rssurl = '%q';",
		                                        feed->title_raw().c_str(),feed->link().c_str(), feed->is_rtl() ? 1 : 0, feed->rssurl().c_str());
		rc = sqlite3_exec(db,updatequery.c_str(),nullptr,nullptr,nullptr);
		if (rc != SQLITE_OK) {
			LOG(LOG_CRITICAL,"query \"%s\" failed: error = %d", updatequery.c_str(), rc);
			throw dbexception(db);
		}
		LOG(LOG_DEBUG,"ran SQL statement: %s", updatequery.c_str());
	} else {
		std::string insertquery = prepare_query("INSERT INTO rss_feed (rssurl, url, title, is_rtl) VALUES ( '%q', '%q', '%q', %u );",
		                                        feed->rssurl().c_str(), feed->link().c_str(), feed->title_raw().c_str(), feed->is_rtl() ? 1 : 0);
		rc = sqlite3_exec(db,insertquery.c_str(),nullptr,nullptr,nullptr);
		if (rc != SQLITE_OK) {
			LOG(LOG_CRITICAL,"query \"%s\" failed: error = %d", insertquery.c_str(), rc);
			throw dbexception(db);
		}
		LOG(LOG_DEBUG,"ran SQL statement: %s", insertquery.c_str());
	}

	unsigned int max_items = cfg->get_configvalue_as_int("max-items");

	LOG(LOG_INFO, "cache::externalize_feed: max_items = %u feed.total_item_count() = %u", max_items, feed->total_item_count());

	if (max_items > 0 && feed->total_item_count() > max_items) {
		auto it=feed->items().begin();
		for (unsigned int i=0; i<max_items; ++i)
			++it;
		if (it != feed->items().end())
			feed->erase_items(it, feed->items().end()); // delete entries that are too much
	}

	unsigned int days = cfg->get_configvalue_as_int("keep-articles-days");
	time_t old_time = time(nullptr) - days * 24*60*60;

	// the reverse iterator is there for the sorting foo below (think about it)
	for (auto it=feed->items().rbegin(); it != feed->items().rend(); ++it) {
		if (days == 0 || (*it)->pubDate_timestamp() >= old_time)
			update_rssitem_unlocked(*it, feed->rssurl(), reset_unread);
	}
}

// this function reads an rss_feed including all of its rss_items.
// the feed parameter needs to have the rssurl member set.
std::shared_ptr<rss_feed> cache::internalize_rssfeed(std::string rssurl, rss_ignores * ign) {
	scope_measure m1("cache::internalize_rssfeed");

	std::shared_ptr<rss_feed> feed(new rss_feed(this));
	feed->set_rssurl(rssurl);

	if (rssurl.substr(0,6) == "query:")
		return feed;

	std::lock_guard<std::mutex> lock(mtx);
	std::lock_guard<std::mutex> feedlock(feed->item_mutex);

	/* first, we check whether the feed is there at all */
	std::string query = prepare_query("SELECT count(*) FROM rss_feed WHERE rssurl = '%q';",rssurl.c_str());
	cb_handler count_cbh;
	LOG(LOG_DEBUG,"running query: %s",query.c_str());
	int rc = sqlite3_exec(db,query.c_str(),count_callback,&count_cbh,nullptr);
	if (rc != SQLITE_OK) {
		LOG(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	}

	if (count_cbh.count() == 0) {
		return feed;
	}

	/* then we first read the feed from the database */
	query = prepare_query("SELECT title, url, is_rtl FROM rss_feed WHERE rssurl = '%q';",rssurl.c_str());
	LOG(LOG_DEBUG,"running query: %s",query.c_str());
	rc = sqlite3_exec(db,query.c_str(),rssfeed_callback,&feed,nullptr);
	if (rc != SQLITE_OK) {
		LOG(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	}

	/* ...and then the associated items */
	query = prepare_query("SELECT guid,title,author,url,pubDate,length(content),unread,feedurl,enclosure_url,enclosure_type,enqueued,flags,base FROM rss_item WHERE feedurl = '%q' AND deleted = 0 ORDER BY pubDate DESC, id DESC;", rssurl.c_str());
	LOG(LOG_DEBUG,"running query: %s",query.c_str());
	rc = sqlite3_exec(db,query.c_str(),rssitem_callback,&feed,nullptr);
	if (rc != SQLITE_OK) {
		LOG(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	}

	std::vector<std::shared_ptr<rss_item>> filtered_items;
	for (auto item : feed->items()) {
		try {
			if (!ign || (ign && ! ign->matches(item.get()))) {
				item->set_cache(this);
				item->set_feedptr(feed);
				item->set_feedurl(feed->rssurl());
				filtered_items.push_back(item);
			}
		} catch(const matcherexception& ex) {
			LOG(LOG_DEBUG, "oops, matcher exception: %s", ex.what());
		}
	}
	feed->set_items(filtered_items);

	unsigned int max_items = cfg->get_configvalue_as_int("max-items");

	if (max_items > 0 && feed->total_item_count() > max_items) {
		std::vector<std::shared_ptr<rss_item>> flagged_items;
		auto it=feed->items().begin();
		for (unsigned int j=0; j<max_items; ++j)
			++it;
		for (unsigned int j=max_items; j<feed->total_item_count(); ++j) {
			if (feed->items()[j]->flags().length() == 0) {
				delete_item(feed->items()[j]);
			} else {
				flagged_items.push_back(feed->items()[j]);
			}
		}
		feed->erase_items(it, feed->items().end()); // delete old entries

		// if some flagged articles were saved, append them
		feed->add_items(flagged_items);
	}
	feed->sort_unlocked(cfg->get_configvalue("article-sort-order"));
	return feed;
}

std::vector<std::shared_ptr<rss_item>> cache::search_for_items(const std::string& querystr, const std::string& feedurl) {
	assert(feedurl.substr(0,6) != "query:");
	std::string query;
	std::vector<std::shared_ptr<rss_item>> items;
	int rc;

	std::lock_guard<std::mutex> lock(mtx);
	if (feedurl.length() > 0) {
		query = prepare_query("SELECT guid,title,author,url,pubDate,length(content),unread,feedurl,enclosure_url,enclosure_type,enqueued,flags,base FROM rss_item WHERE (title LIKE '%%%q%%' OR content LIKE '%%%q%%') AND feedurl = '%q' AND deleted = 0 ORDER BY pubDate DESC, id DESC;",querystr.c_str(), querystr.c_str(), feedurl.c_str());
	} else {
		query = prepare_query("SELECT guid,title,author,url,pubDate,length(content),unread,feedurl,enclosure_url,enclosure_type,enqueued,flags,base FROM rss_item WHERE (title LIKE '%%%q%%' OR content LIKE '%%%q%%') AND deleted = 0 ORDER BY pubDate DESC, id DESC;",querystr.c_str(), querystr.c_str());
	}

	LOG(LOG_DEBUG,"running query: %s",query.c_str());

	rc = sqlite3_exec(db,query.c_str(),search_item_callback,&items,nullptr);
	for (auto item: items) {
		item->set_cache(this);
	}
	if (rc != SQLITE_OK) {
		LOG(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	}

	return items;
}

void cache::delete_item(const std::shared_ptr<rss_item> item) {
	std::string query = prepare_query("DELETE FROM rss_item WHERE guid = '%q';",item->guid().c_str());
	LOG(LOG_DEBUG,"running query: %s",query.c_str());
	int rc = sqlite3_exec(db,query.c_str(),nullptr,nullptr,nullptr);
	if (rc != SQLITE_OK) {
		LOG(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	}
}

void cache::do_vacuum() {
	std::lock_guard<std::mutex> lock(mtx);
	const char * vacuum_query = "VACUUM;";
	int rc = sqlite3_exec(db,vacuum_query,nullptr,nullptr,nullptr);
	if (rc != SQLITE_OK) {
		LOG(LOG_CRITICAL,"query \"%s\" failed: error = %d", vacuum_query, rc);
	}
}

void cache::cleanup_cache(std::vector<std::shared_ptr<rss_feed>>& feeds) {
	mtx.lock(); // we don't use the std::lock_guard<> here... see comments below

	/*
	 * cache cleanup means that all entries in both the rss_feed and rss_item tables that are associated with
	 * an RSS feed URL that is not contained in the current configuration are deleted.
	 * Such entries are the result when a user deletes one or more lines in the urls configuration file. We
	 * then assume that the user isn't interested anymore in reading this feed, and delete all associated entries
	 * because they would be non-accessible.
	 *
	 * The behaviour whether the cleanup is done or not is configurable via the configuration file.
	 */
	if (cfg->get_configvalue_as_bool("cleanup-on-quit")) {
		LOG(LOG_DEBUG,"cache::cleanup_cache: cleaning up cache...");
		std::string list = "(";
		int rc;

		for (auto feed : feeds) {
			std::string name = prepare_query("'%q'",feed->rssurl().c_str());
			list.append(name);
			list.append(", ");
		}
		list.append("'')");

		std::string cleanup_rss_feeds_statement("DELETE FROM rss_feed WHERE rssurl NOT IN ");
		cleanup_rss_feeds_statement.append(list);
		cleanup_rss_feeds_statement.append(1,';');

		std::string cleanup_rss_items_statement("DELETE FROM rss_item WHERE feedurl NOT IN ");
		cleanup_rss_items_statement.append(list);
		cleanup_rss_items_statement.append(1,';');

		std::string cleanup_read_items_statement("DELETE FROM rss_item WHERE unread = 0");

		// std::cerr << "statements: " << cleanup_rss_feeds_statement << " " << cleanup_rss_items_statement << std::endl;

		LOG(LOG_DEBUG,"running query: %s", cleanup_rss_feeds_statement.c_str());
		rc = sqlite3_exec(db,cleanup_rss_feeds_statement.c_str(),nullptr,nullptr,nullptr);
		if (rc != SQLITE_OK) {
			LOG(LOG_CRITICAL,"query \"%s\" failed: error = %d", cleanup_rss_feeds_statement.c_str(), rc);
			throw dbexception(db);
		}

		LOG(LOG_DEBUG,"running query: %s", cleanup_rss_items_statement.c_str());
		rc = sqlite3_exec(db,cleanup_rss_items_statement.c_str(),nullptr,nullptr,nullptr);
		if (rc != SQLITE_OK) {
			LOG(LOG_CRITICAL,"query \"%s\" failed: error = %d", cleanup_rss_items_statement.c_str(), rc);
			throw dbexception(db);
		}

		if (cfg->get_configvalue_as_bool("delete-read-articles-on-quit")) {
			LOG(LOG_DEBUG,"running query: %s", cleanup_read_items_statement.c_str());
			rc = sqlite3_exec(db,cleanup_read_items_statement.c_str(),nullptr,nullptr,nullptr);
			if (rc != SQLITE_OK) {
				LOG(LOG_CRITICAL,"query \"%s\" failed: error = %d", cleanup_read_items_statement.c_str(), rc);
				throw dbexception(db);
			}
		}

		// WARNING: THE MISSING UNLOCK OPERATION IS MISSING FOR A PURPOSE!
		// It's missing so that no database operation can occur after the cache cleanup!
		// mtx->unlock();
	} else {
		LOG(LOG_DEBUG,"cache::cleanup_cache: NOT cleaning up cache...");
	}
}

void cache::update_rssitem_unlocked(std::shared_ptr<rss_item> item, const std::string& feedurl, bool reset_unread) {
	std::string query = prepare_query("SELECT count(*) FROM rss_item WHERE guid = '%q';",item->guid().c_str());
	cb_handler count_cbh;
	LOG(LOG_DEBUG,"running query: %s", query.c_str());
	int rc = sqlite3_exec(db,query.c_str(),count_callback,&count_cbh,nullptr);
	if (rc != SQLITE_OK) {
		LOG(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	}
	if (count_cbh.count() > 0) {
		if (reset_unread) {
			std::string content;
			query = prepare_query("SELECT content FROM rss_item WHERE guid = '%q';", item->guid().c_str());
			rc = sqlite3_exec(db, query.c_str(), single_string_callback, &content, nullptr);
			if (rc != SQLITE_OK) {
				LOG(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
				throw dbexception(db);
			}
			if (content != item->description_raw()) {
				LOG(LOG_DEBUG, "cache::update_rssitem_unlocked: '%s' is different from '%s'", content.c_str(), item->description_raw().c_str());
				query = prepare_query("UPDATE rss_item SET unread = 1 WHERE guid = '%q';", item->guid().c_str());
				rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
				if (rc != SQLITE_OK) {
					LOG(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
					throw dbexception(db);
				}
			}
		}
		std::string update;
		if (item->override_unread()) {
			update = prepare_query("UPDATE rss_item SET title = '%q', author = '%q', url = '%q', feedurl = '%q', content = '%q', enclosure_url = '%q', enclosure_type = '%q', base = '%q', unread = '%d' WHERE guid = '%q'",
			                       item->title_raw().c_str(), item->author_raw().c_str(), item->link().c_str(),
			                       feedurl.c_str(), item->description_raw().c_str(),
			                       item->enclosure_url().c_str(), item->enclosure_type().c_str(), item->get_base().c_str(),
			                       (item->unread() ? 1 : 0),
			                       item->guid().c_str());
		} else {
			update = prepare_query("UPDATE rss_item SET title = '%q', author = '%q', url = '%q', feedurl = '%q', content = '%q', enclosure_url = '%q', enclosure_type = '%q', base = '%q' WHERE guid = '%q'",
			                       item->title_raw().c_str(), item->author_raw().c_str(), item->link().c_str(),
			                       feedurl.c_str(), item->description_raw().c_str(),
			                       item->enclosure_url().c_str(), item->enclosure_type().c_str(), item->get_base().c_str(),
			                       item->guid().c_str());
		}
		LOG(LOG_DEBUG,"running query: %s", update.c_str());
		rc = sqlite3_exec(db,update.c_str(),nullptr,nullptr,nullptr);
		if (rc != SQLITE_OK) {
			LOG(LOG_CRITICAL,"query \"%s\" failed: error = %d", update.c_str(), rc);
			throw dbexception(db);
		}
	} else {
		std::string insert = prepare_query("INSERT INTO rss_item (guid,title,author,url,feedurl,pubDate,content,unread,enclosure_url,enclosure_type,enqueued, base) "
		                                   "VALUES ('%q','%q','%q','%q','%q','%u','%q','%d','%q','%q',%d, '%q')",
		                                   item->guid().c_str(), item->title_raw().c_str(), item->author_raw().c_str(),
		                                   item->link().c_str(), feedurl.c_str(), item->pubDate_timestamp(), item->description_raw().c_str(), (item->unread() ? 1 : 0),
		                                   item->enclosure_url().c_str(), item->enclosure_type().c_str(), item->enqueued() ? 1 : 0, item->get_base().c_str());
		LOG(LOG_DEBUG,"running query: %s", insert.c_str());
		rc = sqlite3_exec(db,insert.c_str(),nullptr,nullptr,nullptr);
		if (rc != SQLITE_OK) {
			LOG(LOG_CRITICAL,"query \"%s\" failed: error = %d", insert.c_str(), rc);
			throw dbexception(db);
		}
	}
}

void cache::catchup_all(std::shared_ptr<rss_feed> feed) {
	std::lock_guard<std::mutex> lock(mtx);
	std::lock_guard<std::mutex> itemlock(feed->item_mutex);
	std::string query = "UPDATE rss_item SET unread = '0' WHERE unread != '0' AND guid IN (";

	for (auto item : feed->items()) {
		query.append(prepare_query("'%q',", item->guid().c_str()));
	}
	query.append("'');");

	LOG(LOG_DEBUG, "running query: %s", query.c_str());
	int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
	if (rc != SQLITE_OK) {
		LOG(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	}
}

/* this function marks all rss_items (optionally of a certain feed url) as read */
void cache::catchup_all(const std::string& feedurl) {
	std::lock_guard<std::mutex> lock(mtx);

	std::string query;
	if (feedurl.length() > 0) {
		query = prepare_query("UPDATE rss_item SET unread = '0' WHERE unread != '0' AND feedurl = '%q';", feedurl.c_str());
	} else {
		query = prepare_query("UPDATE rss_item SET unread = '0' WHERE unread != '0';");
	}
	LOG(LOG_DEBUG, "running query: %s", query.c_str());
	int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
	if (rc != SQLITE_OK) {
		LOG(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	}
}

void cache::update_rssitem_unread_and_enqueued(rss_item* item, const std::string& feedurl) {
	std::lock_guard<std::mutex> lock(mtx);

	std::string query = prepare_query("SELECT count(*) FROM rss_item WHERE guid = '%q';",item->guid().c_str());
	cb_handler count_cbh;
	LOG(LOG_DEBUG,"running query: %s", query.c_str());
	int rc = sqlite3_exec(db,query.c_str(),count_callback,&count_cbh,nullptr);
	if (rc != SQLITE_OK) {
		LOG(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	}

	if (count_cbh.count() > 0) {
		std::string update = prepare_query("UPDATE rss_item SET unread = '%d', enqueued = '%d' WHERE guid = '%q'",
		                                   item->unread()?1:0, item->enqueued()?1:0, item->guid().c_str());
		LOG(LOG_DEBUG,"running query: %s", update.c_str());
		rc = sqlite3_exec(db,update.c_str(),nullptr,nullptr,nullptr);
		if (rc != SQLITE_OK) {
			LOG(LOG_CRITICAL,"query \"%s\" failed: error = %d", update.c_str(), rc);
			throw dbexception(db);
		}
	} else {
		std::string insert = prepare_query("INSERT INTO rss_item (guid,title,author,url,feedurl,pubDate,content,unread,enclosure_url,enclosure_type,enqueued,flags,base) "
		                                   "VALUES ('%q','%q','%q','%q','%q','%u','%q','%d','%q','%q',%d, '%q', '%q')",
		                                   item->guid().c_str(), item->title_raw().c_str(), item->author_raw().c_str(),
		                                   item->link().c_str(), feedurl.c_str(), item->pubDate_timestamp(), item->description_raw().c_str(), item->unread() ? 1 : 0,
		                                   item->enclosure_url().c_str(), item->enclosure_type().c_str(), item->enqueued() ? 1 : 0, item->flags().c_str(),
		                                   item->get_base().c_str());
		LOG(LOG_DEBUG,"running query: %s", insert.c_str());
		rc = sqlite3_exec(db,insert.c_str(),nullptr,nullptr,nullptr);
		if (rc != SQLITE_OK) {
			LOG(LOG_CRITICAL,"query \"%s\" failed: error = %d", insert.c_str(), rc);
			throw dbexception(db);
		}
	}
}

/* this function updates the unread and enqueued flags */
void cache::update_rssitem_unread_and_enqueued(std::shared_ptr<rss_item> item, const std::string& feedurl) {
	update_rssitem_unread_and_enqueued(item.get(), feedurl);
}

/* helper function to wrap std::string around the sqlite3_*mprintf function */
std::string cache::prepare_query(const char * format, ...) {
	std::string result;
	va_list ap;
	va_start(ap, format);
	char * query = sqlite3_vmprintf(format, ap);
	if (query) {
		result = query;
		sqlite3_free(query);
	}
	va_end(ap);
	return result;
}

void cache::update_rssitem_flags(rss_item* item) {
	std::lock_guard<std::mutex> lock(mtx);

	std::string update = prepare_query("UPDATE rss_item SET flags = '%q' WHERE guid = '%q';", item->flags().c_str(), item->guid().c_str());
	LOG(LOG_DEBUG,"running query: %s", update.c_str());
	int rc = sqlite3_exec(db,update.c_str(), nullptr, nullptr, nullptr);
	if (rc != SQLITE_OK) {
		LOG(LOG_CRITICAL, "query \"%s\" failed: error = %d", update.c_str(), rc);
		throw dbexception(db);
	}
}

void cache::remove_old_deleted_items(const std::string& rssurl, const std::vector<std::string>& guids) {
	scope_measure m1("cache::remove_old_deleted_items");
	if (guids.size() == 0) {
		LOG(LOG_DEBUG, "cache::remove_old_deleted_items: not cleaning up anything because last reload brought no new items (detected no changes)");
		return;
	}
	std::string guidset = "(";
	for (auto guid : guids) {
		guidset.append(prepare_query("'%q', ", guid.c_str()));
	}
	guidset.append("'')");
	std::string query = prepare_query("DELETE FROM rss_item WHERE feedurl = '%q' AND deleted = 1 AND guid NOT IN %s;", rssurl.c_str(), guidset.c_str());
	std::lock_guard<std::mutex> lock(mtx);
	int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
	if (rc != SQLITE_OK) {
		LOG(LOG_CRITICAL, "query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	} else {
		LOG(LOG_DEBUG, "cache::remove_old_deleted_items: executed query successfully: %s", query.c_str());
	}
}

unsigned int cache::get_unread_count() {
	std::lock_guard<std::mutex> lock(mtx);

	std::string countquery = "SELECT count(id) FROM rss_item WHERE unread = 1;";
	cb_handler count_cbh;
	int rc = sqlite3_exec(db,countquery.c_str(),count_callback,&count_cbh,nullptr);
	unsigned int count = static_cast<unsigned int>(count_cbh.count());
	LOG(LOG_DEBUG, "cache::get_unread_count: rc = %d count = %u", rc, count);
	return count;
}

void cache::mark_items_read_by_guid(const std::vector<std::string>& guids) {
	scope_measure m1("cache::mark_items_read_by_guid");
	std::string guidset("(");
	for (auto guid : guids) {
		guidset.append(prepare_query("'%q', ", guid.c_str()));
	}
	guidset.append("'')");

	std::string updatequery = utils::strprintf("UPDATE rss_item SET unread = 0 WHERE unread = 1 AND guid IN %s;", guidset.c_str());

	int rc;
	{
		std::lock_guard<std::mutex> lock(mtx);
		rc = sqlite3_exec(db, updatequery.c_str(), nullptr, nullptr, nullptr);
	}

	if (rc != SQLITE_OK) {
		LOG(LOG_CRITICAL, "query \"%s\" failed: error = %d", updatequery.c_str(), rc);
		throw dbexception(db);
	} else {
		LOG(LOG_DEBUG, "cache::mark_items_read_by_guid: executed query successfully: %s", updatequery.c_str());
	}
}

std::vector<std::string> cache::get_read_item_guids() {
	std::vector<std::string> guids;
	std::string query = "SELECT guid FROM rss_item WHERE unread = 0;";

	int rc;
	{
		std::lock_guard<std::mutex> lock(mtx);
		rc = sqlite3_exec(db, query.c_str(), vectorofstring_callback, &guids, nullptr);
	}

	if (rc != SQLITE_OK) {
		LOG(LOG_CRITICAL, "query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	} else {
		LOG(LOG_DEBUG, "cache::get_read_item_guids: executed query successfully: %s", query.c_str());
	}
	return guids;
}

void cache::clean_old_articles() {
	std::lock_guard<std::mutex> lock(mtx);

	unsigned int days = cfg->get_configvalue_as_int("keep-articles-days");
	if (days > 0) {
		time_t old_date = time(nullptr) - days*24*60*60;

		std::string query(utils::strprintf("DELETE FROM rss_item WHERE pubDate < %d", old_date));
		LOG(LOG_DEBUG, "cache::clean_old_articles: about to delete articles with a pubDate older than %d", old_date);
		int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
		LOG(LOG_DEBUG, "cache::clean_old_artgicles: old article delete result: rc = %d", rc);
	} else {
		LOG(LOG_DEBUG, "cache::clean_old_articles, days == 0, not cleaning up anything");
	}
}

void cache::fetch_descriptions(rss_feed * feed) {
	std::vector<std::string> guids;
	for (auto item : feed->items()) {
		guids.push_back(prepare_query("'%q'", item->guid().c_str()));
	}
	std::string in_clause = utils::join(guids, ", ");

	std::string query = prepare_query("SELECT guid,content FROM rss_item WHERE guid IN (%s);", in_clause.c_str());

	LOG(LOG_DEBUG, "running query: %s", query.c_str());

	int rc = sqlite3_exec(db, query.c_str(), fill_content_callback, feed, nullptr);
	if (rc != SQLITE_OK) {
		LOG(LOG_CRITICAL, "query: \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	}
}

}

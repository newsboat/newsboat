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

using namespace newsbeuter;

//-------------------------------------------

struct cb_handler {
		cb_handler() : c(-1) { }
		void set_count(int i) { c = i; }
		int count() { return c; }
	private:
		int c;
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

static int single_int_callback(void * handler, int argc, char ** argv, char ** /* azColName */) {
	int * value = static_cast<int *>(handler);
	if (argc>0 && argv[0]) {
		std::istringstream is(argv[0]);
		int x;
		is >> x;
		*value = x;
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
	std::tr1::shared_ptr<rss_feed>* feed = static_cast<std::tr1::shared_ptr<rss_feed>* >(myfeed);
	// normaly, this shouldn't happen, but we keep the assert()s here nevertheless
	assert(argc == 3);
	assert(argv[0] != NULL);
	assert(argv[1] != NULL);
	assert(argv[2] != NULL);
	(*feed)->set_title(argv[0]);
	(*feed)->set_link(argv[1]);
	(*feed)->set_rtl(strcmp(argv[2],"1")==0);
	GetLogger().log(LOG_INFO, "rssfeed_callback: title = %s link = %s is_rtl = %s",argv[0],argv[1], argv[2]);
	return 0;
}

static int vectorofstring_callback(void * vp, int argc, char ** argv, char ** /* azColName */) {
	std::vector<std::string> * vectorptr = static_cast<std::vector<std::string> *>(vp);
	assert(argc == 1);
	assert(argv[0] != NULL);
	vectorptr->push_back(std::string(argv[0]));
	GetLogger().log(LOG_INFO, "vectorofstring_callback: element = %s", argv[0]);
	return 0;
}

static int rssitem_callback(void * myfeed, int argc, char ** argv, char ** /* azColName */) {
	std::tr1::shared_ptr<rss_feed>* feed = static_cast<std::tr1::shared_ptr<rss_feed>* >(myfeed);
	assert (argc == 12);
	std::tr1::shared_ptr<rss_item> item(new rss_item(NULL));
	item->set_guid(argv[0]);
	item->set_title(argv[1]);
	GetLogger().log(LOG_DEBUG, "rssitem_callback: title = %s", argv[1]);
	item->set_author(argv[2]);
	item->set_link(argv[3]);
	
	std::istringstream is(argv[4]);
	time_t t;
	is >> t;
	item->set_pubDate(t);
	
	item->set_description(argv[5]);
	item->set_unread((std::string("1") == argv[6]));

	item->set_feedurl(argv[7]);

	item->set_enclosure_url(argv[8] ? argv[8] : "");
	item->set_enclosure_type(argv[9] ? argv[9] : "");
	item->set_enqueued((std::string("1") == (argv[10] ? argv[10] : "")));
	item->set_flags(argv[11] ? argv[11] : "");

	(*feed)->items().push_back(item);
	return 0;
}

static int rssitemvector_callback(void * vector, int argc, char ** argv, char ** /* azColName */) {
	std::vector<std::tr1::shared_ptr<rss_item> >* items = static_cast<std::vector<std::tr1::shared_ptr<rss_item> > *>(vector);

	assert (argc == 12);
	std::tr1::shared_ptr<rss_item> item(new rss_item(NULL));
	item->set_guid(argv[0]);
	item->set_title(argv[1]);
	item->set_author(argv[2]);
	item->set_link(argv[3]);
	
	std::istringstream is(argv[4]);
	time_t t;
	is >> t;
	item->set_pubDate(t);
	
	item->set_description(argv[5]);
	item->set_unread((std::string("1") == argv[6]));

	item->set_feedurl(argv[7]);

	item->set_enclosure_url(argv[8] ? argv[8] : "");
	item->set_enclosure_type(argv[9] ? argv[9] : "");
	item->set_enqueued((std::string("1") == (argv[10] ? argv[10] : "")));
	item->set_flags(argv[11] ? argv[11] : "");

	items->push_back(item);
	return 0;
}

static int search_item_callback(void * myfeed, int argc, char ** argv, char ** /* azColName */) {
	std::vector<std::tr1::shared_ptr<rss_item> > * items = static_cast<std::vector<std::tr1::shared_ptr<rss_item> > *>(myfeed);
	assert (argc == 12);
	std::tr1::shared_ptr<rss_item> item(new rss_item(NULL));
	item->set_guid(argv[0]);
	item->set_title(argv[1]);
	item->set_author(argv[2]);
	item->set_link(argv[3]);
	
	std::istringstream is(argv[4]);
	time_t t;
	is >> t;
	item->set_pubDate(t);
	
	item->set_description(argv[5]);
	item->set_unread((std::string("1") == argv[6]));
	item->set_feedurl(argv[7]);

	item->set_enclosure_url(argv[8] ? argv[8] : "");
	item->set_enclosure_type(argv[9] ? argv[9] : "");
	item->set_enqueued((std::string("1") == argv[10]));
	item->set_flags(argv[11] ? argv[11] : "");

	items->push_back(item);
	return 0;
}



cache::cache(const std::string& cachefile, configcontainer * c) : db(0),cfg(c), mtx(0) {
	bool file_exists = false;
	std::fstream f;
	f.open(cachefile.c_str(), std::fstream::in | std::fstream::out);
	if (f.is_open()) {
		file_exists = true;
	}
	int error = sqlite3_open(cachefile.c_str(),&db);
	// TODO: this should be refactored into an exception
	if (error != SQLITE_OK) {
		GetLogger().log(LOG_ERROR,"couldn't sqlite3_open(%s): error = %d", cachefile.c_str(), error);
		std::cout << utils::strprintf(_("Error: opening the cache file `%s' failed: %s"), cachefile.c_str(), sqlite3_errmsg(db)) << std::endl;
		sqlite3_close(db);
		::exit(EXIT_FAILURE);
	}

	populate_tables();
	set_pragmas();

	// we need to manually lock all DB operations because SQLite has no explicit support for multithreading.
	mtx = new mutex();
}

cache::~cache() {
	sqlite3_close(db);
	delete mtx;
}

void cache::set_pragmas() {
	int rc;
	
	// first, we need to swithc off synchronous writing as it's slow as hell
	rc = sqlite3_exec(db, "PRAGMA synchronous = OFF;", NULL, NULL, NULL);
	
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"setting PRAGMA synchronous = OFF failed");
		throw dbexception(db);
	}

	// then we disable case-sensitive matching for the LIKE operator in SQLite, for search operations
	rc = sqlite3_exec(db, "PRAGMA case_sensitive_like=OFF;", NULL, NULL, NULL);

	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"setting PRAGMA case_sensitive_like = OFF failed");
		throw dbexception(db);
	}

}

void cache::populate_tables() {
	int rc;

	rc = sqlite3_exec(db,"CREATE TABLE rss_feed ( "
						" rssurl VARCHAR(1024) PRIMARY KEY NOT NULL, "
						" url VARCHAR(1024) NOT NULL, "
						" title VARCHAR(1024) NOT NULL ); " , NULL, NULL, NULL);

	GetLogger().log(LOG_DEBUG, "cache::populate_tables: CREATE TABLE rss_feed rc = %d", rc);

	rc = sqlite3_exec(db,"CREATE TABLE rss_item ( "
						" id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
						" guid VARCHAR(64) NOT NULL, "
						" title VARCHAR(1024) NOT NULL, "
						" author VARCHAR(1024) NOT NULL, "
						" url VARCHAR(1024) NOT NULL, "
						" feedurl VARCHAR(1024) NOT NULL, "
						" pubDate INTEGER NOT NULL, "
						" content VARCHAR(65535) NOT NULL,"
						" unread INTEGER(1) NOT NULL );", NULL, NULL, NULL);
	GetLogger().log(LOG_DEBUG, "cache::populate_tables: CREATE TABLE rss_item rc = %d", rc);

	/* we need to do these ALTER TABLE statements because we need to store additional data for the podcast support */
	rc = sqlite3_exec(db, "ALTER TABLE rss_item ADD enclosure_url VARCHAR(1024);", NULL, NULL, NULL);
	GetLogger().log(LOG_DEBUG, "cache::populate_tables: ALTER TABLE rss_item (1) rc = %d", rc);

	rc = sqlite3_exec(db, "ALTER TABLE rss_item ADD enclosure_type VARCHAR(1024);", NULL, NULL, NULL);
	GetLogger().log(LOG_DEBUG, "cache::populate_tables: ALTER TABLE rss_item (2) rc = %d", rc);

	rc = sqlite3_exec(db, "ALTER TABLE rss_item ADD enqueued INTEGER(1) NOT NULL DEFAULT 0;", NULL, NULL, NULL);
	GetLogger().log(LOG_DEBUG, "cache::populate_tables: ALTER TABLE rss_item (3) rc = %d", rc);

	rc = sqlite3_exec(db, "ALTER TABLE rss_item ADD flags VARCHAR(52);", NULL, NULL, NULL);
	GetLogger().log(LOG_DEBUG, "cache::populate_tables: ALTER TABLE rss_item (4) rc = %d", rc);

	/* create indexes to speed up certain queries */
	rc = sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_rssurl ON rss_feed(rssurl);", NULL, NULL, NULL);
	GetLogger().log(LOG_DEBUG, "cache::populate_tables: CREATE INDEX ON rss_feed(rssurl) (4) rc = %d", rc);

	rc = sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_guid ON rss_item(guid);", NULL, NULL, NULL);
	GetLogger().log(LOG_DEBUG, "cache::populate_tables: CREATE INDEX ON rss_item(guid) (5) rc = %d", rc);

	rc = sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_feedurl ON rss_item(feedurl);", NULL, NULL, NULL);
	GetLogger().log(LOG_DEBUG, "cache::populate_tables: CREATE INDEX ON rss_item(feedurl) (5) rc = %d", rc);
	if(rc == SQLITE_OK){
		/* we analyse the indices for better statistics */
		rc = sqlite3_exec(db, "ANALYZE;", NULL, NULL, NULL);
		GetLogger().log(LOG_DEBUG, "cache::populate_tables: ANALYZE indices (6) rc = %d", rc);
	}

	rc = sqlite3_exec(db, "ALTER TABLE rss_feed ADD lastmodified INTEGER(11);", NULL, NULL, NULL);
	GetLogger().log(LOG_DEBUG, "cache::populate_tables: ALTER TABLE rss_feed ADD lastmodified: rc = %d", rc);

	rc = sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_lastmodified ON rss_feed(lastmodified);", NULL, NULL, NULL);
	GetLogger().log(LOG_DEBUG, "cache::populate_tables: CREATE INDEX ON rss_feed(lastmodified) rc = %d", rc);

	rc = sqlite3_exec(db, "ALTER TABLE rss_item ADD deleted INTEGER(1) NOT NULL DEFAULT 0;", NULL, NULL, NULL);
	GetLogger().log(LOG_DEBUG, "cache::populate_tables: ALTER TABLE rss_item (7) rc = %d", rc);

	rc = sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_deleted ON rss_item(deleted);", NULL, NULL, NULL);
	GetLogger().log(LOG_DEBUG, "cache::populate_tables: CREATE INDEX ON rss_item(deleted) rc = %d", rc);

	rc = sqlite3_exec(db, "ALTER TABLE rss_feed ADD is_rtl INTEGER(1) NOT NULL DEFAULT 0;", NULL, NULL, NULL);
	GetLogger().log(LOG_DEBUG, "cache::populate_tables: ALTER TABLE rss_feed (8) rc = %d", rc);

}

time_t cache::get_lastmodified(const std::string& feedurl) {
	scope_mutex lock(mtx);
	time_t result = 0;
	std::string query = prepare_query("SELECT lastmodified FROM rss_feed WHERE rssurl = '%q';", feedurl.c_str());
	GetLogger().log(LOG_DEBUG, "running: query: %s", query.c_str());
	int rc = sqlite3_exec(db, query.c_str(), single_int_callback, &result, NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL, "query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	}
	return result;
}

void cache::set_lastmodified(const std::string& feedurl, time_t lastmod) {
	if (lastmod > 0) {
		std::string query = prepare_query("UPDATE rss_feed SET lastmodified = '%d' WHERE rssurl = '%q';", lastmod, feedurl.c_str());
		int rc = sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
		GetLogger().log(LOG_DEBUG, "ran SQL statement: %s result = %d", query.c_str(), rc);
	}
}

void cache::mark_item_deleted(const std::string& guid, bool b) {
	std::string query = prepare_query("UPDATE rss_item SET deleted = %u WHERE guid = '%q'", b ? 1 : 0, guid.c_str());
	int rc = sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
	GetLogger().log(LOG_DEBUG, "cache::mark_item_deleted ran SQL statement: %s result = %d", query.c_str(), rc);
}


std::vector<std::string> cache::get_feed_urls() {
	scope_mutex lock(mtx);
	std::string query = "SELECT rssurl FROM rss_feed;";

	std::vector<std::string> urls;

	int rc = sqlite3_exec(db, query.c_str(), vectorofstring_callback,&urls, NULL);
	assert(rc == SQLITE_OK);

	return urls;
}


// this function writes an rss_feed including all rss_items to the database
void cache::externalize_rssfeed(std::tr1::shared_ptr<rss_feed> feed, bool reset_unread) {
	scope_measure m1("cache::externalize_feed");
	if (feed->rssurl().substr(0,6) == "query:")
		return;

	sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);

	{
		scope_mutex lock(mtx);

		std::string countquery = prepare_query("SELECT count(*) FROM rss_feed WHERE rssurl = '%q';", feed->rssurl().c_str());
		cb_handler count_cbh;
		int rc = sqlite3_exec(db,countquery.c_str(),count_callback,&count_cbh,NULL);
		int count = count_cbh.count();
		GetLogger().log(LOG_DEBUG, "cache::externalize_rss_feed: rss_feeds with rssurl = '%s': found %d",feed->rssurl().c_str(), count);
		if (count > 0) {
			std::string updatequery = prepare_query("UPDATE rss_feed SET title = '%q', url = '%q', is_rtl = %u WHERE rssurl = '%q';",
				feed->title_raw().c_str(),feed->link().c_str(), feed->is_rtl() ? 1 : 0, feed->rssurl().c_str());
			rc = sqlite3_exec(db,updatequery.c_str(),NULL,NULL,NULL);
			GetLogger().log(LOG_DEBUG,"ran SQL statement: %s", updatequery.c_str());
		} else {
			std::string insertquery = prepare_query("INSERT INTO rss_feed (rssurl, url, title, is_rtl) VALUES ( '%q', '%q', '%q', %u );", 
				feed->rssurl().c_str(), feed->link().c_str(), feed->title_raw().c_str(), feed->is_rtl() ? 1 : 0);
			rc = sqlite3_exec(db,insertquery.c_str(),NULL,NULL,NULL);
			GetLogger().log(LOG_DEBUG,"ran SQL statement: %s", insertquery.c_str());
		}
	}
	
	unsigned int max_items = cfg->get_configvalue_as_int("max-items");

	GetLogger().log(LOG_INFO, "cache::externalize_feed: max_items = %u feed.items().size() = %u", max_items, feed->items().size());
	
	if (max_items > 0 && feed->items().size() > max_items) {
		std::vector<std::tr1::shared_ptr<rss_item> >::iterator it=feed->items().begin();
		for (unsigned int i=0;i<max_items;++i)
			++it;	
		if (it != feed->items().end())
			feed->items().erase(it, feed->items().end()); // delete entries that are too much
	}

	// the reverse iterator is there for the sorting foo below (think about it)
	for (std::vector<std::tr1::shared_ptr<rss_item> >::reverse_iterator it=feed->items().rbegin(); it != feed->items().rend(); ++it) {
		update_rssitem(*it, feed->rssurl(), reset_unread);
	}
	sqlite3_exec(db, "END;", NULL, NULL, NULL);
}

// this function reads an rss_feed including all of its rss_items.
// the feed parameter needs to have the rssurl member set.
void cache::internalize_rssfeed(std::tr1::shared_ptr<rss_feed> feed) {
	scope_measure m1("cache::internalize_rssfeed");
	if (feed->rssurl().substr(0,6) == "query:")
		return;

	scope_mutex lock(mtx);

	/* first, we check whether the feed is there at all */
	std::string query = prepare_query("SELECT count(*) FROM rss_feed WHERE rssurl = '%q';",feed->rssurl().c_str());
	cb_handler count_cbh;
	GetLogger().log(LOG_DEBUG,"running query: %s",query.c_str());
	int rc = sqlite3_exec(db,query.c_str(),count_callback,&count_cbh,NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	}

	if (count_cbh.count() == 0) {
		return;
	}

	/* then we first read the feed from the database */
	query = prepare_query("SELECT title, url, is_rtl FROM rss_feed WHERE rssurl = '%q';",feed->rssurl().c_str());
	GetLogger().log(LOG_DEBUG,"running query: %s",query.c_str());
	rc = sqlite3_exec(db,query.c_str(),rssfeed_callback,&feed,NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	}

	feed->items().clear();

	/* ...and then the associated items */
	query = prepare_query("SELECT guid,title,author,url,pubDate,content,unread,feedurl,enclosure_url,enclosure_type,enqueued,flags FROM rss_item WHERE feedurl = '%q' AND deleted = 0 ORDER BY pubDate DESC, id DESC;",feed->rssurl().c_str());
	GetLogger().log(LOG_DEBUG,"running query: %s",query.c_str());
	rc = sqlite3_exec(db,query.c_str(),rssitem_callback,&feed,NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	}

	for (std::vector<std::tr1::shared_ptr<rss_item> >::iterator it=feed->items().begin(); it != feed->items().end(); ++it) {
		(*it)->set_cache(this);
		(*it)->set_feedptr(feed);
		(*it)->set_feedurl(feed->rssurl());
	}
	
	unsigned int max_items = cfg->get_configvalue_as_int("max-items");
	
	if (max_items > 0 && feed->items().size() > max_items) {
		std::vector<std::tr1::shared_ptr<rss_item> > flagged_items;
		std::vector<std::tr1::shared_ptr<rss_item> >::iterator it=feed->items().begin();
		for (unsigned int i=0;i<max_items;++i)
			++it;
		for (unsigned int i=max_items;i<feed->items().size();++i) {
			if (feed->items()[i]->flags().length() == 0) {
				delete_item(feed->items()[i]);
			} else {
				flagged_items.push_back(feed->items()[i]);
			}
		}	
		feed->items().erase(it, feed->items().end()); // delete old entries
		if (flagged_items.size() > 0) {
			feed->items().insert(feed->items().end(), flagged_items.begin(), flagged_items.end()); // if some flagged articles were saved, append them
		}
	}
	feed->sort(cfg->get_configvalue("article-sort-order"));
}

void cache::get_latest_items(std::vector<std::tr1::shared_ptr<rss_item> >& items, unsigned int limit) {
	scope_mutex lock(mtx);
	std::string query = prepare_query("SELECT guid,title,author,url,pubDate,content,unread,feedurl,enclosure_url,enclosure_type,enqueued,flags "
									"FROM rss_item WHERE deleted = 0 ORDER BY pubDate DESC, id DESC LIMIT %d;", limit);
	GetLogger().log(LOG_DEBUG, "running query: %s", query.c_str());
	int rc = sqlite3_exec(db, query.c_str(), rssitemvector_callback, &items, NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	}
}

std::tr1::shared_ptr<rss_feed> cache::get_feed_by_url(const std::string& feedurl) {
	std::tr1::shared_ptr<rss_feed> feed(new rss_feed(this));
	std::string query;
	int rc;

	scope_mutex lock(mtx);

	query = prepare_query("SELECT title, url, is_rtl FROM rss_feed WHERE rssurl = '%q';",feedurl.c_str());
	GetLogger().log(LOG_DEBUG,"running query: %s",query.c_str());

	rc = sqlite3_exec(db,query.c_str(),rssfeed_callback,&feed,NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	}

	return feed;
}

std::vector<std::tr1::shared_ptr<rss_item> > cache::search_for_items(const std::string& querystr, const std::string& feedurl) {
	std::string query;
	std::vector<std::tr1::shared_ptr<rss_item> > items;
	int rc;

	scope_mutex lock(mtx);
	if (feedurl.length() > 0) {
		query = prepare_query("SELECT guid,title,author,url,pubDate,content,unread,feedurl,enclosure_url,enclosure_type,enqueued,flags FROM rss_item WHERE (title LIKE '%%%q%%' OR content LIKE '%%%q%%') AND feedurl = '%q' AND deleted = 0 ORDER BY pubDate DESC, id DESC;",querystr.c_str(), querystr.c_str(), feedurl.c_str());
	} else {
		query = prepare_query("SELECT guid,title,author,url,pubDate,content,unread,feedurl,enclosure_url,enclosure_type,enqueued,flags FROM rss_item WHERE (title LIKE '%%%q%%' OR content LIKE '%%%q%%') AND deleted = 0 ORDER BY pubDate DESC, id DESC;",querystr.c_str(), querystr.c_str());
	}

	GetLogger().log(LOG_DEBUG,"running query: %s",query.c_str());

	rc = sqlite3_exec(db,query.c_str(),search_item_callback,&items,NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	}

	return items;
}

void cache::delete_item(const std::tr1::shared_ptr<rss_item>& item) {
	std::string query = prepare_query("DELETE FROM rss_item WHERE guid = '%q';",item->guid().c_str());
	GetLogger().log(LOG_DEBUG,"running query: %s",query.c_str());
	int rc = sqlite3_exec(db,query.c_str(),NULL,NULL,NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	}
}

void cache::do_vacuum() {
	scope_mutex lock(mtx);
	const char * vacuum_query = "VACUUM;";
	int rc = sqlite3_exec(db,vacuum_query,NULL,NULL,NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", vacuum_query, rc);
	}
}

void cache::cleanup_cache(std::vector<std::tr1::shared_ptr<rss_feed> >& feeds) {
	mtx->lock(); // we don't use the scope_mutex here... see comments below

	/*
	 * cache cleanup means that all entries in both the rss_feed and rss_item tables that are associated with
	 * an RSS feed URL that is not contained in the current configuration are deleted.
	 * Such entries are the result when a user deletes one or more lines in the urls configuration file. We
	 * then assume that the user isn't interested anymore in reading this feed, and delete all associated entries
	 * because they would be non-accessible.
	 *
	 * The behaviour whether the cleanup is done or not is configurable via the configuration file.
	 */
	if (cfg->get_configvalue_as_bool("cleanup-on-quit")==true) {
		GetLogger().log(LOG_DEBUG,"cache::cleanup_cache: cleaning up cache...");
		std::string list = "(";
		int rc;
		unsigned int i = 0;
		unsigned int feed_size = feeds.size();

		for (std::vector<std::tr1::shared_ptr<rss_feed> >::iterator it=feeds.begin();it!=feeds.end();++it,++i) {
			std::string name = prepare_query("'%q'",(*it)->rssurl().c_str());
			list.append(name);
			if (i < feed_size-1) {
				list.append(", ");
			}
		}
		list.append(")");

		std::string cleanup_rss_feeds_statement("DELETE FROM rss_feed WHERE rssurl NOT IN ");
		cleanup_rss_feeds_statement.append(list);
		cleanup_rss_feeds_statement.append(1,';');

		std::string cleanup_rss_items_statement("DELETE FROM rss_item WHERE feedurl NOT IN ");
		cleanup_rss_items_statement.append(list);
		cleanup_rss_items_statement.append(1,';');

		// std::cerr << "statements: " << cleanup_rss_feeds_statement << " " << cleanup_rss_items_statement << std::endl;

		GetLogger().log(LOG_DEBUG,"running query: %s", cleanup_rss_feeds_statement.c_str());
		rc = sqlite3_exec(db,cleanup_rss_feeds_statement.c_str(),NULL,NULL,NULL);
		if (rc != SQLITE_OK) {
			GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", cleanup_rss_feeds_statement.c_str(), rc);
			mtx->unlock(); // see above
			throw dbexception(db);
		}

		GetLogger().log(LOG_DEBUG,"running query: %s", cleanup_rss_items_statement.c_str());
		rc = sqlite3_exec(db,cleanup_rss_items_statement.c_str(),NULL,NULL,NULL);
		if (rc != SQLITE_OK) {
			GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", cleanup_rss_items_statement.c_str(), rc);
			mtx->unlock(); // see above
			throw dbexception(db);
		}

		// WARNING: THE MISSING UNLOCK OPERATION IS MISSING FOR A PURPOSE!
		// It's missing so that no database operation can occur after the cache cleanup!
		// mtx->unlock();
	} else {
		GetLogger().log(LOG_DEBUG,"cache::cleanup_cache: NOT cleaning up cache...");
	}
}

/* this function writes an rss_item to the database, also checking whether this item already exists in the database */
void cache::update_rssitem(std::tr1::shared_ptr<rss_item>& item, const std::string& feedurl, bool reset_unread) {
	scope_mutex lock(mtx);
	std::string query = prepare_query("SELECT count(*) FROM rss_item WHERE guid = '%q';",item->guid().c_str());
	cb_handler count_cbh;
	GetLogger().log(LOG_DEBUG,"running query: %s", query.c_str());
	int rc = sqlite3_exec(db,query.c_str(),count_callback,&count_cbh,NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	}
	if (count_cbh.count() > 0) {
		if (reset_unread) {
			std::string content;
			query = prepare_query("SELECT content FROM rss_item WHERE guid = '%q';", item->guid().c_str());
			rc = sqlite3_exec(db, query.c_str(), single_string_callback, &content, NULL);
			if (rc != SQLITE_OK) {
				GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
				throw dbexception(db);
			}
			if (content != item->description_raw()) {
				query = prepare_query("UPDATE rss_item SET unread = 1 WHERE guid = '%q';", item->guid().c_str());
				rc = sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
				if (rc != SQLITE_OK) {
					GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
					throw dbexception(db);
				}
			}
		}
		std::string update = prepare_query("UPDATE rss_item SET title = '%q', author = '%q', url = '%q', feedurl = '%q', content = '%q', enclosure_url = '%q', enclosure_type = '%q' WHERE guid = '%q'",
			item->title_raw().c_str(), item->author_raw().c_str(), item->link().c_str(), 
			feedurl.c_str(), item->description_raw().c_str(), 
			item->enclosure_url().c_str(), item->enclosure_type().c_str(),
			item->guid().c_str());
		GetLogger().log(LOG_DEBUG,"running query: %s", update.c_str());
		rc = sqlite3_exec(db,update.c_str(),NULL,NULL,NULL);
		if (rc != SQLITE_OK) {
			GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", update.c_str(), rc);
			throw dbexception(db);
		}
	} else {
		std::string insert = prepare_query("INSERT INTO rss_item (guid,title,author,url,feedurl,pubDate,content,unread,enclosure_url,enclosure_type,enqueued) "
								"VALUES ('%q','%q','%q','%q','%q','%u','%q',1,'%q','%q',%d)",
								item->guid().c_str(), item->title_raw().c_str(), item->author_raw().c_str(), 
								item->link().c_str(), feedurl.c_str(), item->pubDate_timestamp(), item->description_raw().c_str(),
								item->enclosure_url().c_str(), item->enclosure_type().c_str(), item->enqueued() ? 1 : 0);
		GetLogger().log(LOG_DEBUG,"running query: %s", insert.c_str());
		rc = sqlite3_exec(db,insert.c_str(),NULL,NULL,NULL);
		if (rc != SQLITE_OK) {
			GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", insert.c_str(), rc);
			throw dbexception(db);
		}
	}
}

void cache::catchup_all(std::tr1::shared_ptr<rss_feed> feed) {
	scope_mutex lock(mtx);
	std::string query = "UPDATE rss_item SET unread = '0' WHERE unread != '0' AND guid IN (";

	for (std::vector<std::tr1::shared_ptr<rss_item> >::iterator it=feed->items().begin();it!=feed->items().end();++it) {
		query.append(prepare_query("'%q',", (*it)->guid().c_str()));
	}
	query.append("'');");

	GetLogger().log(LOG_DEBUG, "running query: %s", query.c_str());
	int rc = sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	}
}

/* this function marks all rss_items (optionally of a certain feed url) as read */
void cache::catchup_all(const std::string& feedurl) {
	scope_mutex lock(mtx);

	std::string query;
	if (feedurl.length() > 0) {
		query = prepare_query("UPDATE rss_item SET unread = '0' WHERE unread != '0' AND feedurl = '%q';", feedurl.c_str());
	} else {
		query = prepare_query("UPDATE rss_item SET unread = '0' WHERE unread != '0';");
	}
	GetLogger().log(LOG_DEBUG, "running query: %s", query.c_str());
	int rc = sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	}
}

void cache::update_rssitem_unread_and_enqueued(rss_item* item, const std::string& feedurl) {
	scope_mutex lock(mtx);

	std::string query = prepare_query("SELECT count(*) FROM rss_item WHERE guid = '%q';",item->guid().c_str());
	cb_handler count_cbh;
	GetLogger().log(LOG_DEBUG,"running query: %s", query.c_str());
	int rc = sqlite3_exec(db,query.c_str(),count_callback,&count_cbh,NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	}

	if (count_cbh.count() > 0) {
		std::string update = prepare_query("UPDATE rss_item SET unread = '%d', enqueued = '%d' WHERE guid = '%q'",
			item->unread()?1:0, item->enqueued()?1:0, item->guid().c_str());
		GetLogger().log(LOG_DEBUG,"running query: %s", update.c_str());
		rc = sqlite3_exec(db,update.c_str(),NULL,NULL,NULL);
		if (rc != SQLITE_OK) {
			GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", update.c_str(), rc);
			throw dbexception(db);
		}
	} else {
		std::string insert = prepare_query("INSERT INTO rss_item (guid,title,author,url,feedurl,pubDate,content,unread,enclosure_url,enclosure_type,enqueued,flags) "
										"VALUES ('%q','%q','%q','%q','%q','%u','%q',1,'%q','%q',%d)",
										item->guid().c_str(), item->title_raw().c_str(), item->author_raw().c_str(), 
										item->link().c_str(), feedurl.c_str(), item->pubDate_timestamp(), item->description_raw().c_str(),
										item->enclosure_url().c_str(), item->enclosure_type().c_str(), item->enqueued() ? 1 : 0);
		GetLogger().log(LOG_DEBUG,"running query: %s", insert.c_str());
		rc = sqlite3_exec(db,insert.c_str(),NULL,NULL,NULL);
		if (rc != SQLITE_OK) {
			GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", insert.c_str(), rc);
			throw dbexception(db);
		}
	}
}

/* this function updates the unread and enqueued flags */
void cache::update_rssitem_unread_and_enqueued(std::tr1::shared_ptr<rss_item>& item, const std::string& feedurl) {
	scope_mutex lock(mtx);

	std::string query = prepare_query("SELECT count(*) FROM rss_item WHERE guid = '%q';",item->guid().c_str());
	cb_handler count_cbh;
	GetLogger().log(LOG_DEBUG,"running query: %s", query.c_str());
	int rc = sqlite3_exec(db,query.c_str(),count_callback,&count_cbh,NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	}

	if (count_cbh.count() > 0) {
		std::string update = prepare_query("UPDATE rss_item SET unread = '%d', enqueued = '%d' WHERE guid = '%q'",
			item->unread()?1:0, item->enqueued()?1:0, item->guid().c_str());
		GetLogger().log(LOG_DEBUG,"running query: %s", update.c_str());
		rc = sqlite3_exec(db,update.c_str(),NULL,NULL,NULL);
		if (rc != SQLITE_OK) {
			GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", update.c_str(), rc);
			throw dbexception(db);
		}
	} else {
		std::string insert = prepare_query("INSERT INTO rss_item (guid,title,author,url,feedurl,pubDate,content,unread,enclosure_url,enclosure_type,enqueued,flags) "
										"VALUES ('%q','%q','%q','%q','%q','%u','%q',1,'%q','%q',%d)",
										item->guid().c_str(), item->title_raw().c_str(), item->author_raw().c_str(), 
										item->link().c_str(), feedurl.c_str(), item->pubDate_timestamp(), item->description_raw().c_str(),
										item->enclosure_url().c_str(), item->enclosure_type().c_str(), item->enqueued() ? 1 : 0);
		GetLogger().log(LOG_DEBUG,"running query: %s", insert.c_str());
		rc = sqlite3_exec(db,insert.c_str(),NULL,NULL,NULL);
		if (rc != SQLITE_OK) {
			GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", insert.c_str(), rc);
			throw dbexception(db);
		}
	}
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
	scope_mutex lock(mtx);

	std::string update = prepare_query("UPDATE rss_item SET flags = '%q' WHERE guid = '%q';", item->flags().c_str(), item->guid().c_str());
	GetLogger().log(LOG_DEBUG,"running query: %s", update.c_str());
	int rc = sqlite3_exec(db,update.c_str(), NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL, "query \"%s\" failed: error = %d", update.c_str(), rc);
		throw dbexception(db);
	}
}

void cache::update_rssitem_flags(std::tr1::shared_ptr<rss_item>& item) {
	scope_mutex lock(mtx);

	std::string update = prepare_query("UPDATE rss_item SET flags = '%q' WHERE guid = '%q';", item->flags().c_str(), item->guid().c_str());
	GetLogger().log(LOG_DEBUG,"running query: %s", update.c_str());
	int rc = sqlite3_exec(db,update.c_str(), NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL, "query \"%s\" failed: error = %d", update.c_str(), rc);
		throw dbexception(db);
	}
}

void cache::remove_old_deleted_items(const std::string& rssurl, const std::vector<std::string>& guids) {
	scope_measure m1("cache::remove_old_deleted_items");
	std::string guidset = "(";
	for (std::vector<std::string>::const_iterator it=guids.begin();it!=guids.end();++it) {
		guidset.append(prepare_query("'%q', ", it->c_str()));
	}
	guidset.append("'')");
	std::string query = prepare_query("DELETE FROM rss_item WHERE feedurl = '%q' AND deleted = 1 AND guid NOT IN %s;", rssurl.c_str(), guidset.c_str());
	int rc = sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL, "query \"%s\" failed: error = %d", query.c_str(), rc);
		throw dbexception(db);
	} else {
		GetLogger().log(LOG_DEBUG, "cache::remove_old_deleted_items: executed query successfully: %s", query.c_str());
	}
}

unsigned int cache::get_unread_count() {
	scope_mutex lock(mtx);

	std::string countquery = "SELECT count(id) FROM rss_item WHERE unread = 1;";
	cb_handler count_cbh;
	int rc = sqlite3_exec(db,countquery.c_str(),count_callback,&count_cbh,NULL);
	unsigned int count = static_cast<unsigned int>(count_cbh.count());
	GetLogger().log(LOG_DEBUG, "cache::get_unread_count: rc = %d count = %u", rc, count);
	return count;
}

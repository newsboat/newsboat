#include <cache.h>
#include <sqlite3.h>
#include <cstdlib>
#include <configcontainer.h>

#include <sstream>
#include <fstream>
#include <iostream>
#include <cassert>
#include <rss.h>
#include <logger.h>
#include <config.h>

using namespace newsbeuter;

//-------------------------------------------

struct cb_handler {
		cb_handler() : c(-1) { }
		void set_count(int i) { c = i; }
		int count() { return c; }
	private:
		int c;
};

static int count_callback(void * handler, int argc, char ** argv, char ** azColName) {
	cb_handler * cbh = (cb_handler *)handler;
	// std::cerr << "inside count_callback" << std::endl;
	if (argc>0) {
		std::istringstream is(argv[0]);
		int x;
		is >> x;
		cbh->set_count(x);
	}
	// std::cerr << "count_callback: count = " << cbh->count() << std::endl;
	return 0;
}

static int rssfeed_callback(void * myfeed, int argc, char ** argv, char ** azColName) {
	rss_feed * feed = (rss_feed *)myfeed;
	// normaly, this shouldn't happen, but we keep the assert()s here nevertheless
	assert(argc == 2);
	assert(argv[0] != NULL);
	assert(argv[1] != NULL);
	feed->set_title(argv[0]);
	feed->set_link(argv[1]);
	GetLogger().log(LOG_INFO, "rssfeed_callback: title = %s link = %s",argv[0],argv[1]);
	// std::cerr << "callback: feed->title = " << feed->title() << std::endl;
	return 0;
}

static int rssitem_callback(void * myfeed, int argc, char ** argv, char ** azColName) {
	rss_feed * feed = (rss_feed *)myfeed;
	assert (argc == 11);
	rss_item item(NULL);
	item.set_guid(argv[0]);
	item.set_title(argv[1]);
	item.set_author(argv[2]);
	item.set_link(argv[3]);
	
	std::istringstream is(argv[4]);
	time_t t;
	is >> t;
	item.set_pubDate(t);
	
	item.set_description(argv[5]);
	item.set_unread((std::string("1") == argv[6]));

	item.set_feedurl(argv[7]);

	item.set_enclosure_url(argv[8] ? argv[8] : "");
	item.set_enclosure_type(argv[9] ? argv[9] : "");
	item.set_enqueued((std::string("1") == (argv[10] ? argv[10] : "")));

	feed->items().push_back(item);
	return 0;
}

static int rssitemvector_callback(void * vector, int argc, char ** argv, char ** azColName) {
	std::vector<rss_item> * items = (std::vector<rss_item> *)vector;

	assert (argc == 11);
	rss_item item(NULL);
	item.set_guid(argv[0]);
	item.set_title(argv[1]);
	item.set_author(argv[2]);
	item.set_link(argv[3]);
	
	std::istringstream is(argv[4]);
	time_t t;
	is >> t;
	item.set_pubDate(t);
	
	item.set_description(argv[5]);
	item.set_unread((std::string("1") == argv[6]));

	item.set_feedurl(argv[7]);

	item.set_enclosure_url(argv[8] ? argv[8] : "");
	item.set_enclosure_type(argv[9] ? argv[9] : "");
	item.set_enqueued((std::string("1") == (argv[10] ? argv[10] : "")));

	items->push_back(item);
	return 0;
}

static int search_item_callback(void * myfeed, int argc, char ** argv, char ** azColName) {
	std::vector<rss_item> * items = (std::vector<rss_item> *)myfeed;
	assert (argc == 11);
	rss_item item(NULL);
	item.set_guid(argv[0]);
	item.set_title(argv[1]);
	item.set_author(argv[2]);
	item.set_link(argv[3]);
	
	std::istringstream is(argv[4]);
	time_t t;
	is >> t;
	item.set_pubDate(t);
	
	item.set_description(argv[5]);
	item.set_unread((std::string("1") == argv[6]));
	item.set_feedurl(argv[7]);

	item.set_enclosure_url(argv[8] ? argv[8] : "");
	item.set_enclosure_type(argv[9] ? argv[9] : "");
	item.set_enqueued((std::string("1") == argv[10]));

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
		char buf[1024];
		snprintf(buf, sizeof(buf), _("Error: opening the cache file `%s' failed: %s"), cachefile.c_str(), sqlite3_errmsg(db));
		sqlite3_close(db);
		std::cout << buf << std::endl;
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
	}
	assert(rc == SQLITE_OK);

	// then we disable case-sensitive matching for the LIKE operator in SQLite, for search operations
	rc = sqlite3_exec(db, "PRAGMA case_sensitive_like=OFF;", NULL, NULL, NULL);

	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"setting PRAGMA case_sensitive_like = OFF failed");
	}

	assert(rc == SQLITE_OK);
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
}


// this function writes an rss_feed including all rss_items to the database
void cache::externalize_rssfeed(rss_feed& feed) {
	mtx->lock();
	std::ostringstream query;
	query << "SELECT count(*) FROM rss_feed WHERE rssurl = '" << feed.rssurl() << "';";
	cb_handler count_cbh;
	int rc = sqlite3_exec(db,query.str().c_str(),count_callback,&count_cbh,NULL);
	int count = count_cbh.count();
	GetLogger().log(LOG_DEBUG, "cache::externalize_rss_feed: rss_feeds with rssurl = '%s': found %d",feed.rssurl().c_str(), count);
	if (count > 0) {
		std::string updatequery = prepare_query("UPDATE rss_feed SET title = '%q', url = '%q' WHERE rssurl = '%q';",feed.title_raw().c_str(),feed.link().c_str(), feed.rssurl().c_str());
		rc = sqlite3_exec(db,updatequery.c_str(),NULL,NULL,NULL);
		GetLogger().log(LOG_DEBUG,"ran SQL statement: %s", updatequery.c_str());
	} else {
		std::string insertquery = prepare_query("INSERT INTO rss_feed (rssurl, url, title) VALUES ( '%q', '%q', '%q' );", feed.rssurl().c_str(), feed.link().c_str(), feed.title_raw().c_str());
		rc = sqlite3_exec(db,insertquery.c_str(),NULL,NULL,NULL);
		GetLogger().log(LOG_DEBUG,"ran SQL statement: %s", insertquery.c_str());
	}


	mtx->unlock();
	
	unsigned int max_items = cfg->get_configvalue_as_int("max-items");

	GetLogger().log(LOG_INFO, "cache::externalize_feed: max_items = %u feed.items().size() = %u", max_items, feed.items().size());
	
	if (max_items > 0 && feed.items().size() > max_items) {
		std::vector<rss_item>::iterator it=feed.items().begin();
		for (unsigned int i=0;i<max_items;++i)
			++it;	
		if (it != feed.items().end())
			feed.items().erase(it, feed.items().end()); // delete entries that are too much
	}

	// the reverse iterator is there for the sorting foo below (think about it)
	for (std::vector<rss_item>::reverse_iterator it=feed.items().rbegin(); it != feed.items().rend(); ++it) {
		update_rssitem(*it, feed.rssurl());
	}
}

// this function reads an rss_feed including all of its rss_items.
// the feed parameter needs to have the rssurl member set.
void cache::internalize_rssfeed(rss_feed& feed) {
	mtx->lock();

	/* first, we check whether the feed is there at all */
	std::string query = prepare_query("SELECT count(*) FROM rss_feed WHERE rssurl = '%q';",feed.rssurl().c_str());
	cb_handler count_cbh;
	GetLogger().log(LOG_DEBUG,"running query: %s",query.c_str());
	int rc = sqlite3_exec(db,query.c_str(),count_callback,&count_cbh,NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
	}
	assert(rc == SQLITE_OK);

	if (count_cbh.count() == 0) {
		mtx->unlock();
		return;
	}

	/* then we first read the feed from the database */
	query = prepare_query("SELECT title, url FROM rss_feed WHERE rssurl = '%q';",feed.rssurl().c_str());
	GetLogger().log(LOG_DEBUG,"running query: %s",query.c_str());
	rc = sqlite3_exec(db,query.c_str(),rssfeed_callback,&feed,NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
	}
	assert(rc == SQLITE_OK);

	if (feed.items().size() > 0) {
		feed.items().erase(feed.items().begin(),feed.items().end());
	}

	/* ...and then the associated items */
	query = prepare_query("SELECT guid,title,author,url,pubDate,content,unread,feedurl,enclosure_url,enclosure_type,enqueued FROM rss_item WHERE feedurl = '%q' ORDER BY pubDate DESC, id DESC;",feed.rssurl().c_str());
	GetLogger().log(LOG_DEBUG,"running query: %s",query.c_str());
	rc = sqlite3_exec(db,query.c_str(),rssitem_callback,&feed,NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
	}
	assert(rc == SQLITE_OK);

	for (std::vector<rss_item>::iterator it=feed.items().begin(); it != feed.items().end(); ++it) {
		it->set_cache(this);
		it->set_feedurl(feed.rssurl());
	}
	
	unsigned int max_items = cfg->get_configvalue_as_int("max-items");
	
	if (max_items > 0 && feed.items().size() > max_items) {
		std::vector<rss_item>::iterator it=feed.items().begin();
		for (unsigned int i=0;i<max_items;++i)
			++it;
		for (unsigned int i=max_items;i<feed.items().size();++i) {
			delete_item(feed.items()[i]);	
		}	
		feed.items().erase(it, feed.items().end()); // delete old entries
	}
	mtx->unlock();
}

void cache::get_latest_items(std::vector<rss_item>& items, unsigned int limit) {
	mtx->lock();
	std::string query = prepare_query("SELECT guid,title,author,url,pubDate,content,unread,feedurl,enclosure_url,enclosure_type,enqueued "
									"FROM rss_item ORDER BY pubDate DESC, id DESC LIMIT %d;", limit);
	GetLogger().log(LOG_DEBUG, "running query: %s", query.c_str());
	int rc = sqlite3_exec(db, query.c_str(), rssitemvector_callback, &items, NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
	}
	assert(rc == SQLITE_OK);

	mtx->unlock();
}

rss_feed cache::get_feed_by_url(const std::string& feedurl) {
	rss_feed feed(this);
	std::string query;
	int rc;

	mtx->lock();
	
	query = prepare_query("SELECT title, url FROM rss_feed WHERE rssurl = '%q';",feedurl.c_str());
	GetLogger().log(LOG_DEBUG,"running query: %s",query.c_str());

	rc = sqlite3_exec(db,query.c_str(),rssfeed_callback,&feed,NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
	}
	assert(rc == SQLITE_OK);

	mtx->unlock();

	return feed;
}

std::vector<rss_item> cache::search_for_items(const std::string& querystr, const std::string& feedurl) {
	std::string query;
	std::vector<rss_item> items;
	int rc;

	mtx->lock();

	if (feedurl.length() > 0) {
		query = prepare_query("SELECT guid,title,author,url,pubDate,content,unread,feedurl,enclosure_url,enclosure_type,enqueued FROM rss_item WHERE (title LIKE '%%%q%%' OR content LIKE '%%%q%%') AND feedurl = '%q' ORDER BY pubDate DESC, id DESC;",querystr.c_str(), querystr.c_str(), feedurl.c_str());
	} else {
		query = prepare_query("SELECT guid,title,author,url,pubDate,content,unread,feedurl,enclosure_url,enclosure_type,enqueued FROM rss_item WHERE (title LIKE '%%%q%%' OR content LIKE '%%%q%%') ORDER BY pubDate DESC, id DESC;",querystr.c_str(), querystr.c_str());
	}

	GetLogger().log(LOG_DEBUG,"running query: %s",query.c_str());

	rc = sqlite3_exec(db,query.c_str(),search_item_callback,&items,NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
	}
	assert(rc == SQLITE_OK);

	mtx->unlock();

	return items;
}

void cache::delete_item(const rss_item& item) {
	std::string query = prepare_query("DELETE FROM rss_item WHERE guid = '%q';",item.guid().c_str());
	GetLogger().log(LOG_DEBUG,"running query: %s",query.c_str());
	int rc = sqlite3_exec(db,query.c_str(),NULL,NULL,NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
	}
	assert(rc == SQLITE_OK);
}

void cache::cleanup_cache(std::vector<rss_feed>& feeds) {
	mtx->lock();

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

		for (std::vector<rss_feed>::iterator it=feeds.begin();it!=feeds.end();++it,++i) {
			std::string name = prepare_query("'%q'",it->rssurl().c_str());
			list.append(name);
			if (i < feed_size-1) {
				list.append(", ");
			}
		}
		list.append(")");

		std::string cleanup_rss_feeds_statement = std::string("DELETE FROM rss_feed WHERE rssurl NOT IN ") + list + ";";
		std::string cleanup_rss_items_statement = std::string("DELETE FROM rss_item WHERE feedurl NOT IN ") + list + ";";

		// std::cerr << "statements: " << cleanup_rss_feeds_statement << " " << cleanup_rss_items_statement << std::endl;

		GetLogger().log(LOG_DEBUG,"running query: %s", cleanup_rss_feeds_statement.c_str());
		rc = sqlite3_exec(db,cleanup_rss_feeds_statement.c_str(),NULL,NULL,NULL);
		if (rc != SQLITE_OK) {
			GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", cleanup_rss_feeds_statement.c_str(), rc);
		}
		assert(rc == SQLITE_OK);

		GetLogger().log(LOG_DEBUG,"running query: %s", cleanup_rss_items_statement.c_str());
		rc = sqlite3_exec(db,cleanup_rss_items_statement.c_str(),NULL,NULL,NULL);
		if (rc != SQLITE_OK) {
			GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", cleanup_rss_items_statement.c_str(), rc);
		}
		assert(rc == SQLITE_OK);

		// rc = sqlite3_exec(db,"VACUUM;",NULL,NULL,NULL);
		// assert(rc == SQLITE_OK);

		// WARNING: THE MISSING UNLOCK OPERATION IS MISSING FOR A PURPOSE!
		// It's missing so that no database operation can occur after the cache cleanup!
		// mtx->unlock();
	} else {
		GetLogger().log(LOG_DEBUG,"cache::cleanup_cache: NOT cleaning up cache...");
	}
}

/* this function writes an rss_item to the database, also checking whether this item already exists in the database */
void cache::update_rssitem(rss_item& item, const std::string& feedurl) {
	mtx->lock();
	std::string query = prepare_query("SELECT count(*) FROM rss_item WHERE guid = '%q';",item.guid().c_str());
	cb_handler count_cbh;
	GetLogger().log(LOG_DEBUG,"running query: %s", query.c_str());
	int rc = sqlite3_exec(db,query.c_str(),count_callback,&count_cbh,NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
	}
	assert(rc == SQLITE_OK);
	if (count_cbh.count() > 0) {
		std::string update = prepare_query("UPDATE rss_item SET title = '%q', author = '%q', url = '%q', feedurl = '%q', content = '%q', enclosure_url = '%q', enclosure_type = '%q' WHERE guid = '%q' AND pubDate != '%u'",
			item.title_raw().c_str(), item.author_raw().c_str(), item.link().c_str(), 
			feedurl.c_str(), item.description_raw().c_str(), 
			item.enclosure_url().c_str(), item.enclosure_type().c_str(),
			item.guid().c_str(), item.pubDate_timestamp());
		GetLogger().log(LOG_DEBUG,"running query: %s", update.c_str());
		rc = sqlite3_exec(db,update.c_str(),NULL,NULL,NULL);
		if (rc != SQLITE_OK) {
			GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", update.c_str(), rc);
		}
		assert(rc == SQLITE_OK);
	} else {
		std::string insert = prepare_query("INSERT INTO rss_item (guid,title,author,url,feedurl,pubDate,content,unread,enclosure_url,enclosure_type,enqueued) "
								"VALUES ('%q','%q','%q','%q','%q','%u','%q',1,'%q','%q',%d)",
								item.guid().c_str(), item.title_raw().c_str(), item.author_raw().c_str(), 
								item.link().c_str(), feedurl.c_str(), item.pubDate_timestamp(), item.description_raw().c_str(),
								item.enclosure_url().c_str(), item.enclosure_type().c_str(), item.enqueued() ? 1 : 0);
		GetLogger().log(LOG_DEBUG,"running query: %s", insert.c_str());
		rc = sqlite3_exec(db,insert.c_str(),NULL,NULL,NULL);
		if (rc != SQLITE_OK) {
			GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", insert.c_str(), rc);
		}
		assert(rc == SQLITE_OK);
	}
	mtx->unlock();
}

/* this function marks all rss_items (optionally of a certain feed url) as read */
void cache::catchup_all(const std::string& feedurl) {
	mtx->lock();
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
	}
	assert(rc == SQLITE_OK);
	mtx->unlock();
}

/* this function updates the unread and enqueued flags */
void cache::update_rssitem_unread_and_enqueued(rss_item& item, const std::string& feedurl) {
	mtx->lock();
	std::string query = prepare_query("SELECT count(*) FROM rss_item WHERE guid = '%q';",item.guid().c_str());
	cb_handler count_cbh;
	GetLogger().log(LOG_DEBUG,"running query: %s", query.c_str());
	int rc = sqlite3_exec(db,query.c_str(),count_callback,&count_cbh,NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query.c_str(), rc);
	}
	assert(rc == SQLITE_OK);

	if (count_cbh.count() > 0) {
		std::string update = prepare_query("UPDATE rss_item SET unread = '%d', enqueued = '%d' WHERE guid = '%q'",
			item.unread()?1:0, item.enqueued()?1:0, item.guid().c_str());
		GetLogger().log(LOG_DEBUG,"running query: %s", update.c_str());
		rc = sqlite3_exec(db,update.c_str(),NULL,NULL,NULL);
		if (rc != SQLITE_OK) {
			GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", update.c_str(), rc);
		}
		assert(rc == SQLITE_OK);
	} else {
		std::string insert = prepare_query("INSERT INTO rss_item (guid,title,author,url,feedurl,pubDate,content,unread,enclosure_url,enclosure_type,enqueued) "
										"VALUES ('%q','%q','%q','%q','%q','%u','%q',1,'%q','%q',%d)",
										item.guid().c_str(), item.title_raw().c_str(), item.author_raw().c_str(), 
										item.link().c_str(), feedurl.c_str(), item.pubDate_timestamp(), item.description_raw().c_str(),
										item.enclosure_url().c_str(), item.enclosure_type().c_str(), item.enqueued() ? 1 : 0);
		GetLogger().log(LOG_DEBUG,"running query: %s", insert.c_str());
		rc = sqlite3_exec(db,insert.c_str(),NULL,NULL,NULL);
		if (rc != SQLITE_OK) {
			GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", insert.c_str(), rc);
		}
		assert(rc == SQLITE_OK);
	}
	mtx->unlock();
}

/* helper function to wrap std::string around the sqlite3_*mprintf function */
std::string cache::prepare_query(const char * format, ...) {
	std::string result;
	va_list ap;
	va_start(ap, format);
	char * query = sqlite3_vmprintf(format, ap);
	if (query) {
		result = query;
		free(query);
	}
	return result;
}

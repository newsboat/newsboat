#include <cache.h>
#include <sqlite3.h>
#include <configcontainer.h>

#include <sstream>
#include <fstream>
#include <iostream>
#include <cassert>
#include <rss.h>
#include <logger.h>

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
	assert(argc == 2);
	assert(argv[0] != NULL);
	assert(argv[1] != NULL);
	feed->set_title(argv[0]);
	feed->set_link(argv[1]);
	// std::cerr << "callback: feed->title = " << feed->title() << std::endl;
	return 0;
}

static int rssitem_callback(void * myfeed, int argc, char ** argv, char ** azColName) {
	rss_feed * feed = (rss_feed *)myfeed;
	assert (argc == 7);
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
	// item.wash();
	feed->items().push_back(item);
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
	if (error != SQLITE_OK) {
		// TODO: error message
		sqlite3_close(db);
		GetLogger().log(LOG_ERROR,"couldn't sqlite3_open(%s): error = %d", cachefile.c_str(), error);
		::exit(EXIT_FAILURE);
	}
	// if (!file_exists) {
	populate_tables();
	set_pragmas();
	// }
	mtx = new mutex();
}

cache::~cache() {
	sqlite3_close(db);
	delete mtx;
}

void cache::set_pragmas() {
	int rc;
	
	rc = sqlite3_exec(db, "PRAGMA synchronous = OFF;", NULL, NULL, NULL);
	
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"setting PRAGMA synchronous = OFF failed");
	}
	assert(rc == SQLITE_OK);
	
}

void cache::populate_tables() {
	// TODO: run create table statements
	int rc;

	rc = sqlite3_exec(db,"CREATE TABLE rss_feed ( "
						" rssurl VARCHAR(1024) PRIMARY KEY NOT NULL, "
						" url VARCHAR(1024) NOT NULL, "
						" title VARCHAR(1024) NOT NULL ); " , NULL, NULL, NULL);

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
}


void cache::externalize_rssfeed(rss_feed& feed) {
	mtx->lock();
	std::ostringstream query;
	query << "SELECT count(*) FROM rss_feed WHERE rssurl = '" << feed.rssurl() << "';";
	cb_handler count_cbh;
	int rc = sqlite3_exec(db,query.str().c_str(),count_callback,&count_cbh,NULL);
	// std::cerr << "externalize: count rc = " << rc << std::endl;
	int count = count_cbh.count();
	GetLogger().log(LOG_DEBUG, "cache::externalize_rss_feed: rss_feeds with rssurl = '%s': found %d",feed.rssurl().c_str(), count);
	// std::cerr << "externalize: count = " << count << std::endl;
	if (count > 0) {
		char * updatequery = sqlite3_mprintf("UPDATE rss_feed SET title = '%q', url = '%q' WHERE rssurl = '%q';",feed.title().c_str(),feed.link().c_str(), feed.rssurl().c_str());
		// std::cerr << updatequery << std::endl;
		rc = sqlite3_exec(db,updatequery,NULL,NULL,NULL);
		GetLogger().log(LOG_DEBUG,"ran SQL statement: %s", updatequery);
		free(updatequery);
		// std::cerr << "externalize: update rc = " << rc << " query = " << updatequery << std::endl;
	} else {
		char * insertquery = sqlite3_mprintf("INSERT INTO rss_feed (rssurl, url, title) VALUES ( '%q', '%q', '%q' );", feed.rssurl().c_str(), feed.link().c_str(), feed.title().c_str());
		// std::cerr << insertquery << std::endl;
		rc = sqlite3_exec(db,insertquery,NULL,NULL,NULL);
		GetLogger().log(LOG_DEBUG,"ran SQL statement: %s", insertquery);
		free(insertquery);
		// std::cerr << "externalize: insert rc = " << rc << " query = " << insertquery << std::endl;
	}


	mtx->unlock();
	
	unsigned int max_items = cfg->get_configvalue_as_int("max-items");
	
	if (max_items > 0 && feed.items().size() > max_items) {
		std::vector<rss_item>::iterator it=feed.items().begin();
		for (unsigned int i=0;i<max_items-1;++i)
			++it;	
		feed.items().erase(it, feed.items().end()); // delete entries that are too much
	}

	// the reverse iterator is there for the sorting foo below (think about it)
	for (std::vector<rss_item>::reverse_iterator it=feed.items().rbegin(); it != feed.items().rend(); ++it) {
		update_rssitem(*it, feed.rssurl());
	}
}

void cache::internalize_rssfeed(rss_feed& feed) {
	mtx->lock();
	char * query = sqlite3_mprintf("SELECT count(*) FROM rss_feed WHERE rssurl = '%q';",feed.rssurl().c_str());
	cb_handler count_cbh;
	GetLogger().log(LOG_DEBUG,"running query: %s",query);
	int rc = sqlite3_exec(db,query,count_callback,&count_cbh,NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query, rc);
	}
	assert(rc == SQLITE_OK);
	// std::cerr << "internalize: query = " << query << std::endl;
	free(query);

	if (count_cbh.count() == 0) {
		mtx->unlock();
		return;
	}

	query = sqlite3_mprintf("SELECT title, url FROM rss_feed WHERE rssurl = '%q';",feed.rssurl().c_str());
	GetLogger().log(LOG_DEBUG,"running query: %s",query);
	rc = sqlite3_exec(db,query,rssfeed_callback,&feed,NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query, rc);
	}
	assert(rc == SQLITE_OK);
	// std::cerr << "internalize: query = " << query << std::endl;
	free(query);

	if (feed.items().size() > 0) {
		feed.items().erase(feed.items().begin(),feed.items().end());
	}

	query = sqlite3_mprintf("SELECT guid,title,author,url,pubDate,content,unread FROM rss_item WHERE feedurl = '%q' ORDER BY pubDate DESC, id DESC;",feed.rssurl().c_str());
	GetLogger().log(LOG_DEBUG,"running query: %s",query);
	rc = sqlite3_exec(db,query,rssitem_callback,&feed,NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query, rc);
	}
	assert(rc == SQLITE_OK);
	free(query);
	for (std::vector<rss_item>::iterator it=feed.items().begin(); it != feed.items().end(); ++it) {
		it->set_cache(this);	
		it->set_feedurl(feed.rssurl());
		// std::cerr << feed.rssurl() << ": " << it->pubDate() << std::endl;
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

void cache::delete_item(const rss_item& item) {
	char * query = sqlite3_mprintf("DELETE FROM rss_item WHERE guid = '%q';",item.guid().c_str());
	GetLogger().log(LOG_DEBUG,"running query: %s",query);
	int rc = sqlite3_exec(db,query,NULL,NULL,NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query, rc);
	}
	assert(rc == SQLITE_OK);
	free(query);
}

void cache::cleanup_cache(std::vector<rss_feed>& feeds) {
	mtx->lock();
	std::string list = "(";
	int rc;
	unsigned int i = 0;
	unsigned int feed_size = feeds.size();

	for (std::vector<rss_feed>::iterator it=feeds.begin();it!=feeds.end();++it,++i) {
		char * name = sqlite3_mprintf("'%q'",it->rssurl().c_str());
		list.append(name);
		if (i < feed_size-1) {
			list.append(", ");
		}
		free(name);
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
}

void cache::update_rssitem(rss_item& item, const std::string& feedurl) {
	mtx->lock();
	char * query = sqlite3_mprintf("SELECT count(*) FROM rss_item WHERE guid = '%q';",item.guid().c_str());
	cb_handler count_cbh;
	GetLogger().log(LOG_DEBUG,"running query: %s", query);
	int rc = sqlite3_exec(db,query,count_callback,&count_cbh,NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query, rc);
	}
	assert(rc == SQLITE_OK);
	free(query);
	if (count_cbh.count() > 0) {
		char * update = sqlite3_mprintf("UPDATE rss_item SET title = '%q', author = '%q', url = '%q', feedurl = '%q', content = '%q' WHERE guid = '%q'",
			item.title().c_str(), item.author().c_str(), item.link().c_str(), 
			feedurl.c_str(), item.description().c_str(), item.guid().c_str());
		GetLogger().log(LOG_DEBUG,"running query: %s", update);
		rc = sqlite3_exec(db,update,NULL,NULL,NULL);
		if (rc != SQLITE_OK) {
			GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", update, rc);
		}
		assert(rc == SQLITE_OK);
		free(update);
	} else {
		char * insert = sqlite3_mprintf("INSERT INTO rss_item (guid,title,author,url,feedurl,pubDate,content,unread) "
										"VALUES ('%q','%q','%q','%q','%q','%u','%q',1)",
										item.guid().c_str(), item.title().c_str(), item.author().c_str(), 
										item.link().c_str(), feedurl.c_str(), item.pubDate_timestamp(), item.description().c_str());
		GetLogger().log(LOG_DEBUG,"running query: %s", insert);
		rc = sqlite3_exec(db,insert,NULL,NULL,NULL);
		if (rc != SQLITE_OK) {
			GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", insert, rc);
		}
		assert(rc == SQLITE_OK);
		free(insert);
	}
	mtx->unlock();
}

void cache::update_rssitem_unread(rss_item& item, const std::string& feedurl) {
	mtx->lock();
	char * query = sqlite3_mprintf("SELECT count(*) FROM rss_item WHERE guid = '%q';",item.guid().c_str());
	cb_handler count_cbh;
	GetLogger().log(LOG_DEBUG,"running query: %s", query);
	int rc = sqlite3_exec(db,query,count_callback,&count_cbh,NULL);
	if (rc != SQLITE_OK) {
		GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", query, rc);
	}
	assert(rc == SQLITE_OK);
	free(query);
	if (count_cbh.count() > 0) {
		char * update = sqlite3_mprintf("UPDATE rss_item SET unread = '%d' WHERE guid = '%q'",
			item.unread()?1:0, item.guid().c_str());
		// std::cerr << update << std::endl;
		GetLogger().log(LOG_DEBUG,"running query: %s", update);
		rc = sqlite3_exec(db,update,NULL,NULL,NULL);
		if (rc != SQLITE_OK) {
			GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", update, rc);
		}
		assert(rc == SQLITE_OK);
		// std::cerr << "item update query:" << update << " |" << std::endl;
		free(update);
	} else {
		char * insert = sqlite3_mprintf("INSERT INTO rss_item (guid,title,author,url,feedurl,pubDate,content,unread) "
										"VALUES ('%q','%q','%q','%q','%q','%u','%q',1)",
										item.guid().c_str(), item.title().c_str(), item.author().c_str(), 
										item.link().c_str(), feedurl.c_str(), item.pubDate_timestamp(), item.description().c_str());
		// std::cerr << insert << std::endl;
		GetLogger().log(LOG_DEBUG,"running query: %s", insert);
		rc = sqlite3_exec(db,insert,NULL,NULL,NULL);
		if (rc != SQLITE_OK) {
			GetLogger().log(LOG_CRITICAL,"query \"%s\" failed: error = %d", insert, rc);
		}
		assert(rc == SQLITE_OK);
		free(insert);
		// std::cerr << "item insert" << std::endl;
	}
	mtx->unlock();
}

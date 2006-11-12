#include <cache.h>
#include <sqlite3.h>

#include <sstream>
#include <fstream>
#include <iostream>
#include <cassert>
#include <rss.h>

using namespace noos;

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
	std::cerr << "inside count_callback" << std::endl;
	if (argc>0) {
		std::istringstream is(argv[0]);
		int x;
		is >> x;
		cbh->set_count(x);
	}
	std::cerr << "count_callback: count = " << cbh->count() << std::endl;
	return 0;
}

static int rssfeed_callback(void * myfeed, int argc, char ** argv, char ** azColName) {
	rss_feed * feed = (rss_feed *)myfeed;
	assert(argc == 2);
	assert(argv[0] != NULL);
	assert(argv[1] != NULL);
	feed->title() = argv[0];
	feed->link() = argv[1];
	std::cerr << "callback: feed->title = " << feed->title() << std::endl;
	return 0;
}

static int rssitem_callback(void * myfeed, int argc, char ** argv, char ** azColName) {
	rss_feed * feed = (rss_feed *)myfeed;
	assert (argc == 7);
	rss_item item;
	item.guid() = argv[0];
	item.title() = argv[1];
	item.author() = argv[2];
	item.link() = argv[3];
	item.pubDate() = argv[4];
	item.description() = argv[5];
	item.unread() = (std::string("1") == argv[6]);
	feed->items().push_back(item);
	return 0;
}

//-------------------------------------------




cache::cache(const std::string& cachefile) : db(0) {
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
		::exit(1);
	}
	// if (!file_exists) {
	populate_tables();
	// }
}

cache::~cache() {
	sqlite3_close(db);
}

void cache::populate_tables() {
	// TODO: run create table statements
	int rc;

	rc = sqlite3_exec(db,"CREATE TABLE rss_feed ( "
						" rssurl VARCHAR(1024) PRIMARY KEY NOT NULL, "
						" url VARCHAR(1024) NOT NULL, "
						" title VARCHAR(1024) NOT NULL ); " , NULL, NULL, NULL);

	rc = sqlite3_exec(db,"CREATE TABLE rss_item ( "
						" guid VARCHAR(64) PRIMARY KEY NOT NULL, "
						" title VARCHAR(1024) NOT NULL, "
						" author VARCHAR(1024) NOT NULL, "
						" url VARCHAR(1024) NOT NULL, "
						" feedurl VARCHAR(1024) NOT NULL, "
						" pubDate DATETIME NOT NULL, "
						" content VARCHAR(65535) NOT NULL,"
						" unread INT(1) NOT NULL );", NULL, NULL, NULL);
}


void cache::externalize_rssfeed(rss_feed& feed) {
	std::ostringstream query;
	query << "SELECT count(*) FROM rss_feed WHERE rssurl = '" << feed.rssurl() << "';";
	cb_handler count_cbh;
	int rc = sqlite3_exec(db,query.str().c_str(),count_callback,&count_cbh,NULL);
	// std::cerr << "externalize: count rc = " << rc << std::endl;
	int count = count_cbh.count();
	// std::cerr << "externalize: count = " << count << std::endl;
	if (count > 0) {
		char * updatequery = sqlite3_mprintf("UPDATE rss_feed SET title = '%q', url = '%q' WHERE rssurl = '%q';",feed.title().c_str(),feed.link().c_str(), feed.rssurl().c_str());
		rc = sqlite3_exec(db,updatequery,NULL,NULL,NULL);
		free(updatequery);
		// std::cerr << "externalize: update rc = " << rc << " query = " << updatequery << std::endl;
	} else {
		char * insertquery = sqlite3_mprintf("INSERT INTO rss_feed (rssurl, url, title) VALUES ( '%q', '%q', '%q' );", feed.rssurl().c_str(), feed.link().c_str(), feed.title().c_str());
		rc = sqlite3_exec(db,insertquery,NULL,NULL,NULL);
		free(insertquery);
		// std::cerr << "externalize: insert rc = " << rc << " query = " << insertquery << std::endl;
	}

	for (std::vector<rss_item>::iterator it=feed.items().begin(); it != feed.items().end(); ++it) {
		char * query = sqlite3_mprintf("SELECT count(*) FROM rss_item WHERE guid = '%q';",it->guid().c_str());
		cb_handler count_cbh;
		int rc = sqlite3_exec(db,query,count_callback,&count_cbh,NULL);
		assert(rc == SQLITE_OK);
		free(query);
		if (count_cbh.count() > 0) {
			char * update;
			
			if (!it->unread()) {
				update = sqlite3_mprintf("UPDATE rss_item SET title = '%q', author = '%q', url = '%q', feedurl = '%q', pubDate = '%q', content = '%q', unread = '0' WHERE guid = '%q'",
					it->title().c_str(), it->author().c_str(), it->link().c_str(), 
					feed.rssurl().c_str(), it->pubDate().c_str(), it->description().c_str(), it->guid().c_str());
			} else {
				update = sqlite3_mprintf("UPDATE rss_item SET title = '%q', author = '%q', url = '%q', feedurl = '%q', pubDate = '%q', content = '%q' WHERE guid = '%q'",
					it->title().c_str(), it->author().c_str(), it->link().c_str(), 
					feed.rssurl().c_str(), it->pubDate().c_str(), it->description().c_str(), it->guid().c_str());
			}
			rc = sqlite3_exec(db,update,NULL,NULL,NULL);
			assert(rc == SQLITE_OK);
			std::cerr << "item update query:" << update << " |" << std::endl;
			free(update);
		} else {
			char * insert = sqlite3_mprintf("INSERT INTO rss_item (guid,title,author,url,feedurl,pubDate,content,unread) "
											"VALUES ('%q','%q','%q','%q','%q','%q','%q',1)",
											it->guid().c_str(), it->title().c_str(), it->author().c_str(), 
											it->link().c_str(), feed.rssurl().c_str(), it->pubDate().c_str(), it->description().c_str());
			rc = sqlite3_exec(db,insert,NULL,NULL,NULL);
			assert(rc == SQLITE_OK);
			free(insert);
			std::cerr << "item insert" << std::endl;
		}
	}
}

void cache::internalize_rssfeed(rss_feed& feed) {
	char * query = sqlite3_mprintf("SELECT count(*) FROM rss_feed WHERE rssurl = '%q';",feed.rssurl().c_str());
	cb_handler count_cbh;
	int rc = sqlite3_exec(db,query,count_callback,&count_cbh,NULL);
	assert(rc == SQLITE_OK);
	std::cerr << "internalize: query = " << query << std::endl;
	free(query);

	if (count_cbh.count() == 0)
		return;

	query = sqlite3_mprintf("SELECT title, url FROM rss_feed WHERE rssurl = '%q';",feed.rssurl().c_str());
	rc = sqlite3_exec(db,query,rssfeed_callback,&feed,NULL);
	assert(rc == SQLITE_OK);
	std::cerr << "internalize: query = " << query << std::endl;
	free(query);

	if (feed.items().size() > 0) {
		feed.items().erase(feed.items().begin(),feed.items().end());
	}

	query = sqlite3_mprintf("SELECT guid,title,author,url,pubDate,content,unread FROM rss_item WHERE feedurl = '%q';",feed.rssurl().c_str());
	rc = sqlite3_exec(db,query,rssitem_callback,&feed,NULL);
	assert(rc == SQLITE_OK);
	free(query);
}

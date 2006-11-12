#include <cache.h>
#include <sqlite3.h>

#include <sstream>
#include <fstream>
#include <iostream>

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
	if (argc>0) {
		std::istringstream is(argv[0]);
		int x;
		is >> x;
		cbh->set_count(x);
	}
	return 0;
}

//-------------------------------------------



using namespace noos;

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
						" url VARCHAR(1024) PRIMARY KEY NOT NULL, "
						" title VARCHAR(1024) NOT NULL ); " , NULL, NULL, NULL);

	rc = sqlite3_exec(db,"CREATE TABLE rss_item ( "
						" guid VARCHAR(64) PRIMARY KEY NOT NULL, "
						" title VARCHAR(1024) NOT NULL, "
						" author VARCHAR(1024) NOT NULL, "
						" url VARCHAR(1024) NOT NULL, "
						" feedurl VARCHAR(1024) NOT NULL, "
						" pubDate DATETIME NOT NULL, "
						" content VARCHAR(65535) NOT NULL );", NULL, NULL, NULL);
}


void cache::externalize_rssfeed(rss_feed& feed) {
	// XXX: protect from SQL injection!!!!
	std::ostringstream query;
	query << "SELECT count(*) FROM rss_feed WHERE url = '" << feed.link() << "';";
	cb_handler count_cbh;
	int rc = sqlite3_exec(db,query.str().c_str(),count_callback,&count_cbh,NULL);
	std::cerr << "externalize: count rc = " << rc << std::endl;
	int count = count_cbh.count();
	std::cerr << "externalize: count = " << count << std::endl;
	if (count > 0) {
		char * updatequery = sqlite3_mprintf("UPDATE rss_feed SET title = '%q' WHERE url = '%q';",feed.title().c_str(),feed.link().c_str());
		rc = sqlite3_exec(db,updatequery,NULL,NULL,NULL);
		free(updatequery);
		std::cerr << "externalize: update rc = " << rc << " query = " << updatequery << std::endl;
	} else {
		char * insertquery = sqlite3_mprintf("INSERT INTO rss_feed (url, title) VALUES ( '%q', '%q' );", feed.link().c_str(), feed.title().c_str());
		rc = sqlite3_exec(db,insertquery,NULL,NULL,NULL);
		free(insertquery);
		std::cerr << "externalize: insert rc = " << rc << " query = " << insertquery << std::endl;
	}
}

void cache::internalize_rssfeed(rss_feed& feed) {
	// TODO
}

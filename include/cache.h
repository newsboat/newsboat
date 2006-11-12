#ifndef NOOS_CACHE__H
#define NOOS_CACHE__H

#include <sqlite3.h>
#include <rss.h>

namespace noos {

class cache {
	public:
		cache(const std::string& cachefile);
		~cache();
		void externalize_rssfeed(rss_feed& feed);
		void internalize_rssfeed(rss_feed& feed);
	private:
		void populate_tables();
		sqlite3 * db;
};


}


#endif

#ifndef NOOS_CACHE__H
#define NOOS_CACHE__H

#include <sqlite3.h>
#include <rss.h>
#include <configcontainer.h>
#include <mutex.h>

namespace noos {

class cache {
	public:
		cache(const std::string& cachefile, configcontainer * c);
		~cache();
		void externalize_rssfeed(rss_feed& feed);
		void internalize_rssfeed(rss_feed& feed);
		void update_rssitem(rss_item& item, const std::string& feedurl);
		void update_rssitem_unread(rss_item& item, const std::string& feedurl);
		void cleanup_cache(std::vector<rss_feed>& feeds);
	private:
		void populate_tables();
		void set_pragmas();
		void delete_item(const rss_item& item);
		sqlite3 * db;
		configcontainer * cfg;
		mutex * mtx;
};


}


#endif

#ifndef NEWSBEUTER_CACHE__H
#define NEWSBEUTER_CACHE__H

#include <sqlite3.h>
#include <rss.h>
#include <configcontainer.h>
#include <mutex.h>

namespace newsbeuter {

class cache {
	public:
		cache(const std::string& cachefile, configcontainer * c);
		~cache();
		void externalize_rssfeed(refcnt_ptr<rss_feed>& feed);
		void internalize_rssfeed(refcnt_ptr<rss_feed>& feed);
		void update_rssitem(rss_item& item, const std::string& feedurl);
		void update_rssitem_unread_and_enqueued(rss_item& item, const std::string& feedurl);
		void cleanup_cache(std::vector<refcnt_ptr<rss_feed> >& feeds);
		void do_vacuum();
		void get_latest_items(std::vector<refcnt_ptr<rss_item> >& items, unsigned int limit);
		std::vector<refcnt_ptr<rss_item> > search_for_items(const std::string& querystr, const std::string& feedurl);
		refcnt_ptr<rss_feed> get_feed_by_url(const std::string& feedurl);
		void catchup_all(const std::string& feedurl = "");
		void catchup_all(refcnt_ptr<rss_feed>& feed);
		std::vector<std::string> get_feed_urls();
	private:
		void populate_tables();
		void set_pragmas();
		void delete_item(refcnt_ptr<rss_item>& item);

		std::string prepare_query(const char * format, ...);
			
		sqlite3 * db;
		configcontainer * cfg;
		mutex * mtx;
};


}


#endif

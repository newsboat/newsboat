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
		void externalize_rssfeed(std::tr1::shared_ptr<rss_feed> feed, bool reset_unread);
		void internalize_rssfeed(std::tr1::shared_ptr<rss_feed> feed);
		void update_rssitem(std::tr1::shared_ptr<rss_item>& item, const std::string& feedurl, bool reset_unread);
		void update_rssitem_unread_and_enqueued(std::tr1::shared_ptr<rss_item>& item, const std::string& feedurl);
		void update_rssitem_unread_and_enqueued(rss_item* item, const std::string& feedurl);
		void cleanup_cache(std::vector<std::tr1::shared_ptr<rss_feed> >& feeds);
		void do_vacuum();
		void get_latest_items(std::vector<std::tr1::shared_ptr<rss_item> >& items, unsigned int limit);
		std::vector<std::tr1::shared_ptr<rss_item> > search_for_items(const std::string& querystr, const std::string& feedurl);
		std::tr1::shared_ptr<rss_feed> get_feed_by_url(const std::string& feedurl);
		void catchup_all(const std::string& feedurl = "");
		void catchup_all(std::tr1::shared_ptr<rss_feed> feed);
		void update_rssitem_flags(std::tr1::shared_ptr<rss_item>& item);
		void update_rssitem_flags(rss_item* item);
		std::vector<std::string> get_feed_urls();
		time_t get_lastmodified(const std::string& feedurl);
		void set_lastmodified(const std::string& feedurl, time_t lastmod);
		unsigned int get_unread_count();
		void mark_item_deleted(const std::string& guid, bool b);
		void remove_old_deleted_items(const std::string& rssurl, const std::vector<std::string>& guids);
	private:
		void populate_tables();
		void set_pragmas();
		void delete_item(const std::tr1::shared_ptr<rss_item>& item);

		std::string prepare_query(const char * format, ...);
			
		sqlite3 * db;
		configcontainer * cfg;
		mutex * mtx;
};


}


#endif

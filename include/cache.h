#ifndef NEWSBEUTER_CACHE__H
#define NEWSBEUTER_CACHE__H

#include <sqlite3.h>
#include <rss.h>
#include <configcontainer.h>
#include <mutex>

namespace newsbeuter {

typedef std::vector<std::string> schema_queries_list;

class cache {
	public:
		cache(const std::string& cachefile, configcontainer * c);
		~cache();
		void externalize_rssfeed(std::shared_ptr<rss_feed> feed, bool reset_unread);
		std::shared_ptr<rss_feed> internalize_rssfeed(std::string rssurl, rss_ignores * ign);
		void update_rssitem_unread_and_enqueued(std::shared_ptr<rss_item> item, const std::string& feedurl);
		void update_rssitem_unread_and_enqueued(rss_item* item, const std::string& feedurl);
		void cleanup_cache(std::vector<std::shared_ptr<rss_feed>>& feeds);
		void do_vacuum();
		std::vector<std::shared_ptr<rss_item>> search_for_items(const std::string& querystr, const std::string& feedurl);
		void catchup_all(const std::string& feedurl = "");
		void catchup_all(std::shared_ptr<rss_feed> feed);
		void update_rssitem_flags(rss_item* item);
		std::vector<std::string> get_feed_urls();
		void fetch_lastmodified(const std::string& uri, time_t& t, std::string& etag);
		void update_lastmodified(const std::string& uri, time_t t, const std::string& etag);
		unsigned int get_unread_count();
		void mark_item_deleted(const std::string& guid, bool b);
		void remove_old_deleted_items(const std::string& rssurl, const std::vector<std::string>& guids);
		void mark_items_read_by_guid(const std::vector<std::string>& guids);
		std::vector<std::string> get_read_item_guids();
		void fetch_descriptions(rss_feed * feed);
	private:
		void populate_tables();
		void set_pragmas();
		void delete_item(const std::shared_ptr<rss_item> item);
		void clean_old_articles();
		void update_rssitem_unlocked(std::shared_ptr<rss_item> item, const std::string& feedurl, bool reset_unread);

		std::string prepare_query(const char * format, ...);

		sqlite3 * db;
		configcontainer * cfg;
		std::mutex mtx;
};

}


#endif

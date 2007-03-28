#ifndef NEWSBEUTER_CONTROLLER__H
#define NEWSBEUTER_CONTROLLER__H

#include <urlreader.h>
#include <rss.h>
#include <cache.h>
#include <nxml.h>
#include <configcontainer.h>

namespace newsbeuter {

	class view;

	class controller {
		public:
			controller();
			~controller();
			void set_view(view * vv);
			void run(int argc = 0, char * argv[] = NULL);
			bool open_feed(unsigned int pos, bool auto_open);
			bool open_item(rss_feed& feed, std::string guid);
			void reload(unsigned int pos, unsigned int max = 0);
			void reload_all();
			void start_reload_all_thread();
			rss_feed * get_feed(unsigned int pos);
			rss_feed * get_feed_by_url(const std::string& feedurl);
			std::vector<rss_item> search_for_items(const std::string& query, const std::string& feedurl);
			inline unsigned int get_feedcount() { return feeds.size(); }
			inline void unlock_reload_mutex() { reload_mutex->unlock(); }
			void update_feedlist();
			void mark_all_read(unsigned int pos);
			void catchup_all();
			inline bool get_refresh_on_start() { return refresh_on_start; }
			bool is_valid_podcast_type(const std::string& mimetype);
			void enqueue_url(const std::string& url);
			void set_itemlist_feed(unsigned int pos);

		private:
			void usage(char * argv0);
			void import_opml(const char * filename);
			void export_opml();
			void rec_find_rss_outlines(nxml_data_t * node, std::string tag);

			view * v;
			urlreader urlcfg;
			cache * rsscache;
			std::vector<rss_feed> feeds;
			std::string config_dir;
			std::string url_file;
			std::string cache_file;
			std::string config_file;
			std::string queue_file;
			bool refresh_on_start;
			configcontainer * cfg;

			mutex * reload_mutex;
	};

}


#endif

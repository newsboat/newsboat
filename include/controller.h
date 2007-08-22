#ifndef NEWSBEUTER_CONTROLLER__H
#define NEWSBEUTER_CONTROLLER__H

#include <urlreader.h>
#include <rss.h>
#include <cache.h>
#include <nxml.h>
#include <configcontainer.h>
#include <filtercontainer.h>

namespace newsbeuter {

	class view;

	class controller {
		public:
			controller();
			~controller();
			void set_view(view * vv);
			void run(int argc = 0, char * argv[] = NULL);

			void reload(unsigned int pos, unsigned int max = 0);
			void reload_all();
			void start_reload_all_thread();

			refcnt_ptr<rss_feed> get_feed(unsigned int pos);
			refcnt_ptr<rss_feed> get_feed_by_url(const std::string& feedurl);
			std::vector<refcnt_ptr<rss_item> > search_for_items(const std::string& query, const std::string& feedurl);
			inline unsigned int get_feedcount() { return feeds.size(); }

			inline void unlock_reload_mutex() { reload_mutex->unlock(); }
			bool trylock_reload_mutex();

			void update_feedlist();
			void mark_all_read(unsigned int pos);
			void catchup_all();
			inline bool get_refresh_on_start() { return refresh_on_start; }
			bool is_valid_podcast_type(const std::string& mimetype);
			void enqueue_url(const std::string& url);
			void set_itemlist_feed(unsigned int pos);
			void notify(const std::string& msg);

			void reload_urls_file();

			inline std::vector<refcnt_ptr<rss_feed> >& get_all_feeds() { return feeds; }

			// TODO: move somewhere else...
			void set_feedptrs(refcnt_ptr<rss_feed> feed);

			inline filtercontainer& get_filters() { return filters; }

		private:
			void usage(char * argv0);
			void version_information();
			void import_opml(const char * filename);
			void export_opml();
			void rec_find_rss_outlines(nxml_data_t * node, std::string tag);
			void compute_unread_numbers(unsigned int&, unsigned int& );

			view * v;
			urlreader * urlcfg;
			cache * rsscache;
			std::vector<refcnt_ptr<rss_feed> > feeds;
			std::string config_dir;
			std::string url_file;
			std::string cache_file;
			std::string config_file;
			std::string queue_file;
			bool refresh_on_start;
			configcontainer * cfg;
			rss_ignores ign;
			filtercontainer filters;

			mutex * reload_mutex;
	};

}


#endif

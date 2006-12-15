#ifndef NOOS_CONTROLLER__H
#define NOOS_CONTROLLER__H

#include <urlreader.h>
#include <rss.h>
#include <cache.h>
#include <nxml.h>

namespace noos {

	class view;

	class controller {
		public:
			controller();
			~controller();
			void set_view(view * vv);
			void run(int argc = 0, char * argv[] = NULL);
			void open_feed(unsigned int pos);
			bool open_item(rss_item& item);
			void reload(unsigned int pos, unsigned int max = 0);
			void reload_all();
			void start_reload_all_thread();
			inline void unlock_reload_mutex() { reload_mutex->unlock(); }
			void update_feedlist();
			void mark_all_read(unsigned int pos);
			void catchup_all();
		private:
			void usage(char * argv0);
			void import_opml(const char * filename);
			void export_opml();
			void rec_find_rss_outlines(nxml_data_t * node);

			view * v;
			urlreader urlcfg;
			cache * rsscache;
			std::vector<rss_feed> feeds;
			std::string config_dir;
			std::string url_file;
			std::string cache_file;
			std::string config_file;

			mutex * reload_mutex;
	};

}


#endif

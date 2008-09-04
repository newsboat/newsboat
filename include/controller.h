#ifndef NEWSBEUTER_CONTROLLER__H
#define NEWSBEUTER_CONTROLLER__H

#include <urlreader.h>
#include <rss.h>
#include <cache.h>
#include <_nxml.h>
#include <configcontainer.h>
#include <filtercontainer.h>

namespace newsbeuter {

	class view;

	class controller {
		public:
			controller();
			~controller();
			void set_view(view * vv);
			view * get_view() { return v; }
			void run(int argc = 0, char * argv[] = NULL);

			void reload(unsigned int pos, unsigned int max = 0, bool unattended = false);

			void reload_all(bool unattended = false);
			void reload_indexes(const std::vector<int>& indexes, bool unattended = false);
			void start_reload_all_thread(std::vector<int> * indexes = 0);

			std::tr1::shared_ptr<rss_feed> get_feed(unsigned int pos);
			std::tr1::shared_ptr<rss_feed> get_feed_by_url(const std::string& feedurl);
			std::vector<std::tr1::shared_ptr<rss_item> > search_for_items(const std::string& query, const std::string& feedurl);
			inline unsigned int get_feedcount() { return feeds.size(); }

			inline void unlock_reload_mutex() { reload_mutex->unlock(); }
			bool trylock_reload_mutex();

			void update_feedlist();
			void update_visible_feeds();
			void mark_all_read(unsigned int pos);
			void catchup_all();
			inline void catchup_all(std::tr1::shared_ptr<rss_feed> feed) { rsscache->catchup_all(feed); }
			inline bool get_refresh_on_start() { return refresh_on_start; }
			bool is_valid_podcast_type(const std::string& mimetype);
			void enqueue_url(const std::string& url);
			void notify(const std::string& msg);

			void reload_urls_file();

			inline std::vector<std::tr1::shared_ptr<rss_feed> >& get_all_feeds() { return feeds; }

			void set_feedptrs(std::tr1::shared_ptr<rss_feed> feed);

			inline filtercontainer& get_filters() { return filters; }

			std::string bookmark(const std::string& url, const std::string& title, const std::string& description);

			inline cache * get_cache() { return rsscache; }

			inline configcontainer * get_cfg() const { return cfg; }

			void write_item(const rss_item& item, const std::string& filename);

			void mark_deleted(const std::string& guid, bool b);

		private:
			void usage(char * argv0);
			void version_information();
			void import_opml(const char * filename);
			void export_opml();
			void rec_find_rss_outlines(nxml_data_t * node, std::string tag);
			void compute_unread_numbers(unsigned int&, unsigned int& );
			void execute_commands(char ** argv, unsigned int i);

			std::string prepare_message(unsigned int pos, unsigned int max);
			void save_feed(std::tr1::shared_ptr<rss_feed> feed, unsigned int pos);
			void enqueue_items(std::tr1::shared_ptr<rss_feed> feed);

			view * v;
			urlreader * urlcfg;
			cache * rsscache;
			std::vector<std::tr1::shared_ptr<rss_feed> > feeds;
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

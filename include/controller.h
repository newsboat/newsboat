#ifndef NEWSBEUTER_CONTROLLER__H
#define NEWSBEUTER_CONTROLLER__H

#include <urlreader.h>
#include <rss.h>
#include <cache.h>
#include <configcontainer.h>
#include <filtercontainer.h>
#include <colormanager.h>
#include <regexmanager.h>
#include <remote_api.h>
#include <libxml/tree.h>

namespace newsbeuter {

	extern int ctrl_c_hit;
	extern std::string lock_file;

	class view;

	class curl_handle;

	class controller {
		public:
			controller();
			~controller();
			void set_view(view * vv);
			view * get_view() { return v; }
			void run(int argc = 0, char * argv[] = NULL);

			void reload(unsigned int pos, unsigned int max = 0, bool unattended = false, curl_handle *easyhandle = 0);

			void reload_all(bool unattended = false);
			void reload_indexes(const std::vector<int>& indexes, bool unattended = false);
			void reload_range(unsigned int start, unsigned int end, unsigned int size, bool unattended = false);
			void start_reload_all_thread(std::vector<int> * indexes = 0);

			std::tr1::shared_ptr<rss_feed> get_feed(unsigned int pos);
			std::tr1::shared_ptr<rss_feed> get_feed_by_url(const std::string& feedurl);
			std::vector<std::tr1::shared_ptr<rss_item> > search_for_items(const std::string& query, const std::string& feedurl);
			inline unsigned int get_feedcount() { return feeds.size(); }

			inline void unlock_reload_mutex() { reload_mutex.unlock(); }
			bool trylock_reload_mutex();

			void update_feedlist();
			void update_visible_feeds();
			void mark_all_read(unsigned int pos);
			void mark_article_read(const std::string& guid, bool read);
			void record_google_replay(const std::string& guid, bool read);
			void catchup_all();
			inline void catchup_all(std::tr1::shared_ptr<rss_feed> feed) { rsscache->catchup_all(feed); }
			inline bool get_refresh_on_start() const { return refresh_on_start; }
			bool is_valid_podcast_type(const std::string& mimetype);
			void enqueue_url(const std::string& url, std::tr1::shared_ptr<rss_feed> feed);
			void notify(const std::string& msg);
			unsigned int get_pos_of_next_unread(unsigned int pos);

			void reload_urls_file();
			void edit_urls_file();

			std::vector<std::tr1::shared_ptr<rss_feed> > get_all_feeds();
			std::vector<std::tr1::shared_ptr<rss_feed> > get_all_feeds_unlocked();

			inline filtercontainer& get_filters() { return filters; }

			std::string bookmark(const std::string& url, const std::string& title, const std::string& description);

			inline cache * get_cache() { return rsscache; }

			inline configcontainer * get_cfg() { return &cfg; }

			void write_item(std::tr1::shared_ptr<rss_item> item, const std::string& filename);
			void write_item(std::tr1::shared_ptr<rss_item> item, std::ostream& ostr);
			std::string write_temporary_item(std::tr1::shared_ptr<rss_item> item);

			void mark_deleted(const std::string& guid, bool b);

			void update_config();

			void load_configfile(const std::string& filename);

			void dump_config(const std::string& filename);

			void sort_feeds();

			void update_flags(std::tr1::shared_ptr<rss_item> item);

			unsigned int get_feed_count_per_tag(const std::string& tag);
		private:
			void usage(char * argv0);
			bool setup_dirs_xdg(const char *env_home, bool silent);
			void setup_dirs(bool silent);
			void version_information(const char * argv0, unsigned int level);
			void import_opml(const char * filename);
			void export_opml();
			void rec_find_rss_outlines(xmlNode * node, std::string tag);
			void compute_unread_numbers(unsigned int&, unsigned int& );
			void execute_commands(char ** argv, unsigned int i);

			std::string prepare_message(unsigned int pos, unsigned int max);
			void save_feed(std::tr1::shared_ptr<rss_feed> feed, unsigned int pos);
			void enqueue_items(std::tr1::shared_ptr<rss_feed> feed);

			std::string generate_enqueue_filename(const std::string& url, std::tr1::shared_ptr<rss_feed> feed);
			std::string get_hostname_from_url(const std::string& url);

			void import_read_information(const std::string& readinfofile);
			void export_read_information(const std::string& readinfofile);

			view * v;
			urlreader * urlcfg;
			cache * rsscache;
			std::vector<std::tr1::shared_ptr<rss_feed> > feeds;
			std::string config_dir;
			std::string url_file;
			std::string cache_file;
			std::string config_file;
			std::string queue_file;
			std::string searchfile;
			std::string cmdlinefile;
			bool refresh_on_start;
			configcontainer cfg;
			rss_ignores ign;
			filtercontainer filters;

			mutex reload_mutex;
			configparser cfgparser;
			colormanager colorman;
			regexmanager rxman;
			remote_api * api;
			mutex feeds_mutex;
			bool offline_mode;
	};

}


#endif

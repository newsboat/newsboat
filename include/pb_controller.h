#ifndef PODBEUTER_CONTROLLER__H
#define PODBEUTER_CONTROLLER__H

#include <string>
#include <configcontainer.h>
#include <download.h>
#include <queueloader.h>
#include <vector>

namespace podbeuter {

class pb_view;

class queueloader;

class pb_controller {
	public:
		pb_controller();
		~pb_controller();
		inline void set_view(pb_view * vv) {
			v = vv;
		}
		void run(int argc, char * argv[] = 0);

		inline bool view_update_necessary() const {
			return view_update_;
		}
		inline void set_view_update_necessary(bool b) {
			view_update_ = b;
		}
		std::vector<download>& downloads() {
			return downloads_;
		}

		void usage(const char * argv0);

		std::string get_dlpath();

		unsigned int downloads_in_progress();
		void reload_queue(bool remove_unplayed = false);

		unsigned int get_maxdownloads();
		void start_downloads();

		void increase_parallel_downloads();
		void decrease_parallel_downloads();

		double get_total_kbps();

		void play_file(const std::string& str);

		inline newsbeuter::configcontainer * get_cfgcont() {
			return cfg;
		}

	private:
		bool setup_dirs_xdg(const char *env_home);
		void setup_dirs(const char *custom_home);

		pb_view * v;
		std::string config_file;
		std::string queue_file;
		newsbeuter::configcontainer * cfg;
		bool view_update_;
		std::vector<download> downloads_;

		std::string config_dir;
		std::string url_file;
		std::string cache_file;
		std::string searchfile;
		std::string cmdlinefile;

		unsigned int max_dls;

		queueloader * ql;
};

}

#endif

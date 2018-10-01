#ifndef PODBOAT_CONTROLLER_H_
#define PODBOAT_CONTROLLER_H_

#include <memory>
#include <string>
#include <vector>

#include "configcontainer.h"
#include "download.h"
#include "fslock.h"
#include "queueloader.h"

namespace podboat {

class pb_view;

class queueloader;

class pb_controller {
public:
	pb_controller();
	~pb_controller();
	void set_view(pb_view* vv)
	{
		v = vv;
	}
	int run(int argc, char* argv[] = 0);

	bool view_update_necessary() const
	{
		return view_update_;
	}
	void set_view_update_necessary(bool b)
	{
		view_update_ = b;
	}
	std::vector<download>& downloads()
	{
		return downloads_;
	}

	std::string get_dlpath();
	std::string get_formatstr();

	unsigned int downloads_in_progress();
	void reload_queue(bool remove_unplayed = false);

	unsigned int get_maxdownloads();
	void start_downloads();

	void increase_parallel_downloads();
	void decrease_parallel_downloads();

	double get_total_kbps();

	void play_file(const std::string& str);

	newsboat::configcontainer* get_cfgcont()
	{
		return cfg;
	}

private:
	void print_usage(const char* argv0);
	bool setup_dirs_xdg(const char* env_home);

	pb_view* v;
	std::string config_file;
	std::string queue_file;
	newsboat::configcontainer* cfg;
	bool view_update_;
	std::vector<download> downloads_;

	std::string config_dir;
	std::string url_file;
	std::string cache_file;
	std::string searchfile;
	std::string cmdlinefile;

	unsigned int max_dls;

	queueloader* ql;

	std::string lock_file;
	std::unique_ptr<newsboat::FSLock> fslock;
};

} // namespace podboat

#endif /* PODBOAT_CONTROLLER_H_ */

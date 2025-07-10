#ifndef PODBOAT_CONTROLLER_H_
#define PODBOAT_CONTROLLER_H_

#include <memory>
#include <string>
#include <vector>

#include "colormanager.h"
#include "configcontainer.h"
#include "download.h"
#include "fslock.h"
#include "keymap.h"
#include "queueloader.h"

namespace Podboat {

class PbView;

class PbController {
public:
	PbController();
	~PbController() = default;

	void initialize(int argc, char* argv[]);
	int run(PbView& v);

	Newsboat::KeyMap& get_keymap();

	std::vector<Download>& downloads()
	{
		return downloads_;
	}

	unsigned int downloads_in_progress();
	void purge_queue();

	unsigned int get_maxdownloads();
	void start_downloads();
	void start_download(Download& item);

	void increase_parallel_downloads();
	void decrease_parallel_downloads();

	double get_total_kbps();

	void play_file(const std::string& str);

	Newsboat::ConfigContainer* get_cfgcont()
	{
		return &cfg;
	}

	const Newsboat::ColorManager& get_colormanager()
	{
		return colorman;
	}

private:
	void print_usage(const char* argv0);
	bool setup_dirs_xdg(const char* env_home);

	std::string config_file;
	std::string queue_file;
	Newsboat::ConfigContainer cfg;
	std::vector<Download> downloads_;

	std::string config_dir;

	unsigned int max_dls;

	std::unique_ptr<QueueLoader> ql;

	std::string lock_file;
	std::unique_ptr<Newsboat::FsLock> fslock;

	bool automatic_dl = false;
	Newsboat::ColorManager colorman;
	Newsboat::KeyMap keys;
};

} // namespace Podboat

#endif /* PODBOAT_CONTROLLER_H_ */

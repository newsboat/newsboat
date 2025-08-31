#ifndef PODBOAT_CONTROLLER_H_
#define PODBOAT_CONTROLLER_H_

#include <memory>
#include <string>
#include <vector>

#include "colormanager.h"
#include "configcontainer.h"
#include "download.h"
#include "filepath.h"
#include "fslock.h"
#include "keymap.h"
#include "queueloader.h"

namespace podboat {

class PbView;

class PbController {
public:
	PbController();
	~PbController() = default;

	void initialize(int argc, char* argv[]);
	int run(PbView& v);

	newsboat::KeyMap& get_keymap();

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

	void play_file(const newsboat::Filepath& str);

	newsboat::ConfigContainer* get_cfgcont()
	{
		return &cfg;
	}

	const newsboat::ColorManager& get_colormanager()
	{
		return colorman;
	}

private:
	void print_usage(const char* argv0);
	bool setup_dirs_xdg(const newsboat::Filepath& home);

	newsboat::Filepath config_file;
	newsboat::Filepath queue_file;
	newsboat::ConfigContainer cfg;
	std::vector<Download> downloads_;

	newsboat::Filepath config_dir;

	unsigned int max_dls;

	std::unique_ptr<QueueLoader> ql;

	newsboat::Filepath lock_file;
	std::unique_ptr<newsboat::FsLock> fslock;

	bool automatic_dl = false;
	newsboat::ColorManager colorman;
	newsboat::KeyMap keys;
};

} // namespace podboat

#endif /* PODBOAT_CONTROLLER_H_ */

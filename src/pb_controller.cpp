#include <pb_controller.h>
#include <pb_view.h>
#include <poddlthread.h>
#include <config.h>
#include <utils.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>

#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <cstdlib>
#include <signal.h>
#include <unistd.h>

#include <keymap.h>
#include <configcontainer.h>
#include <colormanager.h>
#include <exceptions.h>
#include <queueloader.h>
#include <logger.h>

using namespace newsbeuter;

static std::string lock_file = "pb-lock.pid";

static void ctrl_c_action(int sig) {
	LOG(LOG_DEBUG,"caugh signal %d",sig);
	stfl::reset();
	utils::remove_fs_lock(lock_file);
	::exit(EXIT_FAILURE);
}

namespace podbeuter {

#define LOCK_SUFFIX ".lock"

/**
 * \brief Try to setup XDG style dirs.
 *
 * returns false, if that fails
 */
bool pb_controller::setup_dirs_xdg(const char *env_home) {
        const char *env_xdg_config;
        const char *env_xdg_data;
        std::string xdg_config_dir;
        std::string xdg_data_dir;

        env_xdg_config = ::getenv("XDG_CONFIG_HOME");
        if (env_xdg_config) {
                xdg_config_dir = env_xdg_config;
        } else {
                xdg_config_dir = env_home;
                xdg_config_dir.append(NEWSBEUTER_PATH_SEP);
                xdg_config_dir.append(".config");
        }

        env_xdg_data = ::getenv("XDG_DATA_HOME");
        if (env_xdg_data) {
                xdg_data_dir = env_xdg_data;
        } else {
                xdg_data_dir = env_home;
                xdg_data_dir.append(NEWSBEUTER_PATH_SEP);
                xdg_data_dir.append(".local");
                xdg_data_dir.append(NEWSBEUTER_PATH_SEP);
                xdg_data_dir.append("share");
        }

        xdg_config_dir.append(NEWSBEUTER_PATH_SEP);
        xdg_config_dir.append(NEWSBEUTER_SUBDIR_XDG);

        xdg_data_dir.append(NEWSBEUTER_PATH_SEP);
        xdg_data_dir.append(NEWSBEUTER_SUBDIR_XDG);

        if (access(xdg_config_dir.c_str(), R_OK | X_OK) != 0)
        {
                std::cout << utils::strprintf(_("XDG: configuration directory '%s' not accessible, using '%s' instead."), xdg_config_dir.c_str(), config_dir.c_str()) << std::endl;
                return false;
        }
        if (access(xdg_data_dir.c_str(), R_OK | X_OK | W_OK) != 0)
        {
                std::cout << utils::strprintf(_("XDG: data directory '%s' not accessible, using '%s' instead."), xdg_data_dir.c_str(), config_dir.c_str()) << std::endl;
                return false;
        }

        config_dir = xdg_config_dir;

        /* in config */
        url_file = config_dir + std::string(NEWSBEUTER_PATH_SEP) + url_file;
        config_file = config_dir + std::string(NEWSBEUTER_PATH_SEP) + config_file;

        /* in data */
        cache_file = xdg_data_dir + std::string(NEWSBEUTER_PATH_SEP) + cache_file;
        lock_file = cache_file + LOCK_SUFFIX;
        queue_file = xdg_data_dir + std::string(NEWSBEUTER_PATH_SEP) + queue_file;
        searchfile = utils::strprintf("%s%shistory.search", xdg_data_dir.c_str(), NEWSBEUTER_PATH_SEP);
        cmdlinefile = utils::strprintf("%s%shistory.cmdline", xdg_data_dir.c_str(), NEWSBEUTER_PATH_SEP);

        return true;
}

pb_controller::pb_controller() : v(0), config_file("config"), queue_file("queue"), cfg(0), view_update_(true),  max_dls(1), ql(0) { 
	char * cfgdir;
	if (!(cfgdir = ::getenv("HOME"))) {
		struct passwd * spw = ::getpwuid(::getuid());
		if (spw) {
			cfgdir = spw->pw_dir;
		} else {
			std::cout << _("Fatal error: couldn't determine home directory!") << std::endl;
			std::cout << utils::strprintf(_("Please set the HOME environment variable or add a valid user for UID %u!"), ::getuid()) << std::endl;
			::exit(EXIT_FAILURE);
		}
	}
	config_dir = cfgdir;

	if (setup_dirs_xdg(cfgdir))
		return;

	config_dir.append(NEWSBEUTER_PATH_SEP);
	config_dir.append(NEWSBEUTER_CONFIG_SUBDIR);
	::mkdir(config_dir.c_str(),0700); // create configuration directory if it doesn't exist

	config_file = config_dir + std::string(NEWSBEUTER_PATH_SEP) + config_file;
	queue_file = config_dir + std::string(NEWSBEUTER_PATH_SEP) + queue_file;
	lock_file = config_dir + std::string(NEWSBEUTER_PATH_SEP) + lock_file;
}

pb_controller::~pb_controller() { 
	delete cfg;
}

void pb_controller::run(int argc, char * argv[]) {
	int c;
	bool automatic_dl = false;

	::signal(SIGINT, ctrl_c_action);

	do {
		if ((c = ::getopt(argc, argv, "C:q:d:l:ha")) < 0)
			continue;

		switch (c) {
			case ':':
			case '?':
				usage(argv[0]);
				break;
			case 'C':
				config_file = optarg;
				break;
			case 'q':
				queue_file = optarg;
				break;
			case 'a':
				automatic_dl = true;
				break;
			case 'd': // this is an undocumented debug commandline option!
				logger::getInstance().set_logfile(optarg);
				break;
			case 'l': // this is an undocumented debug commandline option!
				{
					loglevel level = static_cast<loglevel>(atoi(optarg));
					if (level > LOG_NONE && level <= LOG_DEBUG)
						logger::getInstance().set_loglevel(level);
				}
				break;
			case 'h':
				usage(argv[0]);
				break;
			default:
				std::cout << utils::strprintf(_("%s: unknown option - %c"), argv[0], static_cast<char>(c)) << std::endl;
				usage(argv[0]);
				break;
		}
	} while (c != -1);

	std::cout << utils::strprintf(_("Starting %s %s..."), "podbeuter", PROGRAM_VERSION) << std::endl;

	pid_t pid;
	if (!utils::try_fs_lock(lock_file, pid)) {
		std::cout << utils::strprintf(_("Error: an instance of %s is already running (PID: %u)"), "podbeuter", pid) << std::endl;
		return;
	}

	std::cout << _("Loading configuration...");
	std::cout.flush();

	configparser cfgparser;
	cfg = new configcontainer();
	cfg->register_commands(cfgparser);
	colormanager * colorman = new colormanager();
	colorman->register_commands(cfgparser);

	keymap keys(KM_PODBEUTER);
	cfgparser.register_handler("bind-key", &keys);
	cfgparser.register_handler("unbind-key", &keys);

	null_config_action_handler null_cah;
	cfgparser.register_handler("macro", &null_cah);
	cfgparser.register_handler("ignore-article", &null_cah);
	cfgparser.register_handler("always-download", &null_cah);
	cfgparser.register_handler("define-filter", &null_cah);
	cfgparser.register_handler("highlight", &null_cah);
	cfgparser.register_handler("highlight-article", &null_cah);
	cfgparser.register_handler("reset-unread-on-update", &null_cah);

	try {
		cfgparser.parse("/etc/newsbeuter/config");
		cfgparser.parse(config_file);
	} catch (const configexception& ex) {
		std::cout << ex.what() << std::endl;
		delete colorman;
		return;	
	}

	if (colorman->colors_loaded())
		colorman->set_pb_colors(v);
	delete colorman;

	max_dls = cfg->get_configvalue_as_int("max-downloads");

	std::cout << _("done.") << std::endl;

	ql = new queueloader(queue_file, this);
	ql->reload(downloads_);

	v->set_keymap(&keys);
	
	v->run(automatic_dl);

	stfl::reset();

	std::cout <<  _("Cleaning up queue...");
	std::cout.flush();
	
	ql->reload(downloads_);
	delete ql;
	
	std::cout << _("done.") << std::endl;

	utils::remove_fs_lock(lock_file);
}

void pb_controller::usage(const char * argv0) {
	std::cout << utils::strprintf(_("%s %s\nusage %s [-C <file>] [-q <file>] [-h]\n"
				"-C <configfile> read configuration from <configfile>\n"
				"-q <queuefile>  use <queuefile> as queue file\n"
				"-a              start download on startup\n"
				"-h              this help\n"), "podbeuter", PROGRAM_VERSION, argv0);
	::exit(EXIT_FAILURE);
}

std::string pb_controller::get_dlpath() {
	return cfg->get_configvalue("download-path");
}

unsigned int pb_controller::downloads_in_progress() {
	unsigned int count = 0;
	if (downloads_.size() > 0) {
		for (std::vector<download>::iterator it=downloads_.begin();it!=downloads_.end();++it) {
			if (it->status() == DL_DOWNLOADING)
				++count;
		}
	}
	return count;
}

unsigned int pb_controller::get_maxdownloads() {
	return max_dls;
}

void pb_controller::reload_queue(bool remove_unplayed) {
	if (ql) {
		ql->reload(downloads_, remove_unplayed);
	}
}

double pb_controller::get_total_kbps() {
	double result = 0.0;
	if (downloads_.size() > 0) {
		for (std::vector<download>::iterator it=downloads_.begin();it!=downloads_.end();++it) {
			if (it->status() == DL_DOWNLOADING) {
				result += it->kbps();
			}
		}
	}
	return result;
}

void pb_controller::start_downloads() {
	int dl2start = get_maxdownloads() - downloads_in_progress();
	for (std::vector<download>::iterator it=downloads_.begin();dl2start > 0 && it!=downloads_.end();++it) {
		if (it->status() == DL_QUEUED) {
			poddlthread * thread = new poddlthread(&(*it), cfg);
			thread->start();
			--dl2start;
		}
	}
}

void pb_controller::increase_parallel_downloads() {
	++max_dls;
}

void pb_controller::decrease_parallel_downloads() {
	if (max_dls > 1)
		--max_dls;
}

void pb_controller::play_file(const std::string& file) {
	std::string cmdline;
	std::string player = cfg->get_configvalue("player");
	if (player == "")
		return;
	cmdline.append(player);
	cmdline.append(" \"");
	cmdline.append(utils::replace_all(file,"\"", "\\\""));
	cmdline.append("\"");
	stfl::reset();
	LOG(LOG_DEBUG, "pb_controller::play_file: running `%s'", cmdline.c_str());
	::system(cmdline.c_str());
}


} // namespace

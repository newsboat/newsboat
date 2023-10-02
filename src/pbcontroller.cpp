#include "pbcontroller.h"

#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <iostream>
#include <optional>
#include <pwd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <memory>
#include <thread>
#include <unistd.h>

#include "config.h"
#include "configcontainer.h"
#include "configexception.h"
#include "configparser.h"
#include "logger.h"
#include "nullconfigactionhandler.h"
#include "pbview.h"
#include "poddlthread.h"
#include "queueloader.h"
#include "strprintf.h"
#include "utils.h"

using namespace newsboat;

static void ctrl_c_action(int sig)
{
	LOG(Level::DEBUG, "caugh signal %d", sig);
	Stfl::reset();
	::exit(EXIT_FAILURE);
}

namespace podboat {

/**
 * \brief Try to setup XDG style dirs.
 *
 * returns false, if that fails
 */
bool PbController::setup_dirs_xdg(const char* env_home)
{
	const char* env_xdg_config;
	const char* env_xdg_data;
	newsboat::Filepath xdg_config_dir;
	newsboat::Filepath xdg_data_dir;

	env_xdg_config = ::getenv("XDG_CONFIG_HOME");
	if (env_xdg_config) {
		xdg_config_dir = env_xdg_config;
	} else {
		xdg_config_dir = env_home;
		xdg_config_dir.push(".config");
	}

	env_xdg_data = ::getenv("XDG_DATA_HOME");
	if (env_xdg_data) {
		xdg_data_dir = env_xdg_data;
	} else {
		xdg_data_dir = env_home;
		xdg_data_dir.push(".local");
		xdg_data_dir.push("share");
	}

	xdg_config_dir.push(NEWSBOAT_SUBDIR_XDG);

	xdg_data_dir.push(NEWSBOAT_SUBDIR_XDG);

	bool config_dir_exists =
		0 == access(xdg_config_dir.to_locale_string().c_str(), R_OK | X_OK);

	if (!config_dir_exists) {
		std::cerr << strprintf::fmt(
				_("XDG: configuration directory '%s' not "
					"accessible, "
					"using '%s' instead."),
				xdg_config_dir,
				config_dir)
			<< std::endl;

		return false;
	}

	/* Invariant: config dir exists.
	 *
	 * At this point, we're confident we'll be using XDG. We don't check if
	 * data dir exists, because if it doesn't we'll create it. */

	config_dir = xdg_config_dir.clone();

	// create data directory if it doesn't exist
	int ret = utils::mkdir_parents(xdg_data_dir, 0700);
	if (ret == -1) {
		LOG(Level::CRITICAL,
			"Couldn't create `%s'",
			xdg_data_dir);
		::exit(EXIT_FAILURE);
	}

	/* in config */
	config_file =
		config_dir.join(config_file);

	/* in data */
	const std::string LOCK_SUFFIX(".lock");
	lock_file = xdg_data_dir.join(LOCK_SUFFIX);
	queue_file =
		xdg_data_dir.join(queue_file);

	return true;
}

PbController::PbController()
	: config_file("config")
	, queue_file("queue")
	, max_dls(1)
	, lock_file("pb-lock.pid")
	, keys(KM_PODBOAT)
{
	char* cfgdir;
	if (!(cfgdir = ::getenv("HOME"))) {
		struct passwd* spw = ::getpwuid(::getuid());
		if (spw) {
			cfgdir = spw->pw_dir;
		} else {
			std::cout << _("Fatal error: couldn't determine home "
					"directory!")
				<< std::endl;
			std::cout << strprintf::fmt(
					_("Please set the HOME "
						"environment variable or add a "
						"valid user for UID %u!"),
					::getuid())
				<< std::endl;
			::exit(EXIT_FAILURE);
		}
	}
	config_dir = cfgdir;

	if (setup_dirs_xdg(cfgdir)) {
		return;
	}

	config_dir.push(NEWSBOAT_CONFIG_SUBDIR);

	// create configuration directory if it doesn't exist
	int ret = ::mkdir(config_dir.to_locale_string().c_str(), 0700);
	if (ret && errno != EEXIST) {
		std::cerr << strprintf::fmt(
				_("Fatal error: couldn't create "
					"configuration directory `%s': (%i) %s"),
				config_dir,
				errno,
				std::strerror(errno))
			<< std::endl;
		::exit(EXIT_FAILURE);
	}

	config_file = config_dir.join(config_file);
	queue_file = config_dir.join(queue_file);
	lock_file = config_dir.join(lock_file);
}

void PbController::initialize(int argc, char* argv[])
{
	int c;

	::signal(SIGINT, ctrl_c_action);

	static const char getopt_str[] = "C:q:d:l:havV";
	static const struct option longopts[] = {
		{"config-file", required_argument, 0, 'C'},
		{"queue-file", required_argument, 0, 'q'},
		{"lock-file", required_argument, 0, 256},
		{"log-file", required_argument, 0, 'd'},
		{"log-level", required_argument, 0, 'l'},
		{"help", no_argument, 0, 'h'},
		{"autodownload", no_argument, 0, 'a'},
		{"version", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};
	std::optional<Level> log_level;
	std::optional<std::string> log_file;

	while ((c = ::getopt_long(argc, argv, getopt_str, longopts, nullptr)) !=
		-1) {
		switch (c) {
		case ':':
		case '?':
			print_usage(argv[0]);
			exit(EXIT_FAILURE);
		case 'C':
			config_file = optarg;
			break;
		case 'q':
			queue_file = optarg;
			break;
		case 256:
			lock_file = optarg;
			break;
		case 'a':
			automatic_dl = true;
			break;
		case 'd':
			log_file = optarg;
			break;
		case 'l': {
			log_level = static_cast<Level>(atoi(optarg));
			if (log_level < Level::USERERROR || log_level > Level::DEBUG) {
				std::cerr << strprintf::fmt(_("%s: %d: invalid "
							"loglevel value"),
						argv[0],
						static_cast<int>(log_level.value()))
					<< std::endl;
				exit(EXIT_FAILURE);
			}
		}
		break;
		case 'h':
			print_usage(argv[0]);
			exit(EXIT_SUCCESS);
		}
	};

	if (log_level.has_value()) {
		logger::set_loglevel(log_level.value());
	}

	if (log_file.has_value()) {
		logger::set_logfile(log_file.value());
	}

	if (!log_file.has_value() && log_level.has_value()) {
		const std::string date_time_string = utils::mt_strf_localtime("%Y-%m-%d_%H.%M.%S",
				std::time(nullptr));
		const std::string filename = "podboat_" + date_time_string + ".log";
		logger::set_logfile(filename);
	}

	std::cout << strprintf::fmt(
			_("Starting %s %s..."), "Podboat", utils::program_version())
		<< std::endl;

	fslock = std::make_unique<FsLock>();
	pid_t pid;
	std::string error_message;
	if (!fslock->try_lock(lock_file, pid, error_message)) {
		if (pid != 0) {
			std::cout << strprintf::fmt(
					_("Error: an instance of %s is already "
						"running (PID: %s)"),
					"Podboat",
					std::to_string(pid))
				<< std::endl;
		} else {
			std::cout << _("Error: ") << error_message << std::endl;
		}
		exit(EXIT_FAILURE);
	}

	std::cout << _("Loading configuration...");
	std::cout.flush();

	ConfigParser cfgparser;
	cfg.register_commands(cfgparser);
	colorman.register_commands(cfgparser);

	cfgparser.register_handler("bind", keys);
	cfgparser.register_handler("bind-key", keys);
	cfgparser.register_handler("unbind-key", keys);

	NullConfigActionHandler null_cah;
	cfgparser.register_handler("macro", null_cah);
	cfgparser.register_handler("ignore-article", null_cah);
	cfgparser.register_handler("always-download", null_cah);
	cfgparser.register_handler("define-filter", null_cah);
	cfgparser.register_handler("highlight", null_cah);
	cfgparser.register_handler("highlight-article", null_cah);
	cfgparser.register_handler("highlight-feed", null_cah);
	cfgparser.register_handler("reset-unread-on-update", null_cah);
	cfgparser.register_handler("run-on-startup", null_cah);

	try {
		cfgparser.parse_file("/etc/newsboat/config");
		cfgparser.parse_file(config_file);
	} catch (const ConfigException& ex) {
		std::cout << ex.what() << std::endl;
		exit(EXIT_FAILURE);
	}
}

int PbController::run(PbView& v)
{
	v.apply_colors_to_all_forms();

	max_dls = cfg.get_configvalue_as_int("max-downloads");

	std::cout << _("done.") << std::endl;

	ql = std::make_unique<QueueLoader>(queue_file, cfg, [&]() {
		v.set_view_update_necessary();
	});
	ql->reload(downloads_);

	v.run(automatic_dl, cfg.get_configvalue_as_bool("wrap-scroll"));

	Stfl::reset();

	std::cout << _("Cleaning up queue...");
	std::cout.flush();

	ql->reload(downloads_);

	std::cout << _("done.") << std::endl;

	return EXIT_SUCCESS;
}

newsboat::KeyMap& PbController::get_keymap()
{
	return keys;
}

void PbController::print_usage(const char* argv0)
{
	auto msg = strprintf::fmt(
			_("%s %s\nusage %s [-C <file>] [-q <file>] [-h]\n"),
			"Podboat",
			utils::program_version(),
			argv0);
	std::cout << msg;

	struct Arg {
		const char name;
		const std::string longname;
		const std::string params;
		const std::string desc;
	};

	static const std::vector<Arg> args = {
		{
			'C',
			"config-file",
			_s("<file>"),
			_s("read configuration from <file>")
		},
		{
			'q',
			"queue-file",
			_s("<file>"),
			_s("use <file> as podcast queue file")
		},
		{
			'-',
			"lock-file",
			_s("<file>"),
			_s("use <file> as lock file")
		},
		{'a', "autodownload", "", _s("start download on startup")},
		{
			'l',
			"log-level",
			_s("<loglevel>"),
			_s("write a log with a certain log level (valid values: 1 to 6,"
				" for user error, critical, error, warning, info, and debug respectively)")
		},
		{
			'd',
			"log-file",
			_s("<file>"),
			_s("use <file> as output log file")
		},
		{'h', "help", "", _s("this help")}
	};

	std::vector<std::pair<std::string, std::string>> helpLines;
	std::size_t maxLength = 0;
	for (const auto& a : args) {
		std::string longcolumn;
		if (a.name != '-') {
			longcolumn += "-";
			longcolumn += a.name;
			longcolumn += ", ";
		} else {
			longcolumn += "    ";
		}
		longcolumn += "--" + a.longname;
		longcolumn += a.params.size() > 0 ? "=" + a.params : "";

		maxLength = std::max(maxLength, longcolumn.length());
		helpLines.push_back({longcolumn, a.desc});
	}
	for (const auto& helpLine : helpLines) {
		std::cout << std::string(8, ' ') << helpLine.first;
		const auto padding = maxLength - helpLine.first.length();
		std::cout << std::string(2 + padding, ' ') << helpLine.second;
		std::cout << std::endl;
	}
	std::cout << std::endl;

	std::cout << _("Support at #newsboat at https://libera.chat or on our mailing "
			"list https://groups.google.com/g/newsboat")
		<< std::endl
		<< _("For more information, check out https://newsboat.org/")
		<< std::endl;
}

unsigned int PbController::downloads_in_progress()
{
	unsigned int count = 0;
	for (const auto& dl : downloads_) {
		if (dl.status() == DlStatus::DOWNLOADING) {
			++count;
		}
	}
	return count;
}

unsigned int PbController::get_maxdownloads()
{
	return max_dls;
}

void PbController::purge_queue()
{
	if (ql != nullptr) {
		ql->reload(downloads_, true);
	}
}

double PbController::get_total_kbps()
{
	double result = 0.0;
	for (const auto& dl : downloads_) {
		if (dl.status() == DlStatus::DOWNLOADING) {
			result += dl.kbps();
		}
	}
	return result;
}

void PbController::start_downloads()
{
	int dl2start = get_maxdownloads() - downloads_in_progress();
	for (auto& download : downloads_) {
		if (dl2start == 0) {
			break;
		}

		if (download.status() == DlStatus::QUEUED) {
			start_download(download);
			--dl2start;
		}
	}
}

void PbController::start_download(Download& item)
{
	std::thread t{PodDlThread(&item, cfg)};
	t.detach();
}

void PbController::increase_parallel_downloads()
{
	++max_dls;
}

void PbController::decrease_parallel_downloads()
{
	if (max_dls > 1) {
		--max_dls;
	}
}

void PbController::play_file(const newsboat::Filepath& file)
{
	std::string cmdline;
	std::string player = cfg.get_configvalue("player");
	if (player == "") {
		return;
	}
	cmdline.append(player);
	cmdline.append(" '");
	cmdline.append(utils::replace_all(file, "'", "'\\''"));
	cmdline.append("'");
	Stfl::reset();
	utils::run_interactively(cmdline, "PbController::play_file");
}

} // namespace podboat

#include <pb_controller.h>
#include <pb_view.h>
#include <poddlthread.h>
#include <config.h>
#include <iostream>
#include <sstream>
#include <cstdio>

#include <sys/types.h>
#include <pwd.h>
#include <signal.h>

#include <keymap.h>
#include <configcontainer.h>
#include <colormanager.h>
#include <exceptions.h>
#include <queueloader.h>
#include <logger.h>

using namespace newsbeuter;

static std::string lock_file = "pb-lock.pid";

void ctrl_c_action(int sig) {
	GetLogger().log(LOG_DEBUG,"caugh signal %d",sig);
	stfl_reset();
	::unlink(lock_file.c_str());
	if (SIGSEGV == sig) {
		fprintf(stderr,"%s\n", _("Segmentation fault."));
	}
	::exit(EXIT_FAILURE);
}

namespace podbeuter {

pb_controller::pb_controller() : v(0), config_file("config"), queue_file("queue"), cfg(0), view_update_(true),  max_dls(1), ql(0) { 
	std::ostringstream cfgfile;

	char * cfgdir;
	if (!(cfgdir = ::getenv("HOME"))) {
		struct passwd * spw = ::getpwuid(::getuid());
		if (spw) {
			cfgdir = spw->pw_dir;
		} else {
			std::cout << _("Fatal error: couldn't determine home directory!") << std::endl;
			char buf[1024];
			snprintf(buf, sizeof(buf), _("Please set the HOME environment variable or add a valid user for UID %u!"), ::getuid());
			std::cout << buf << std::endl;
			::exit(EXIT_FAILURE);
		}
	}
	config_dir = cfgdir;


	config_dir.append(NEWSBEUTER_PATH_SEP);
	config_dir.append(NEWSBEUTER_CONFIG_SUBDIR);
	mkdir(config_dir.c_str(),0700); // create configuration directory if it doesn't exist

	config_file = config_dir + std::string(NEWSBEUTER_PATH_SEP) + config_file;
	queue_file = config_dir + std::string(NEWSBEUTER_PATH_SEP) + queue_file;
	lock_file = config_dir + std::string(NEWSBEUTER_PATH_SEP) + lock_file;
}

pb_controller::~pb_controller() { 
	delete cfg;
}

void pb_controller::run(int argc, char * argv[]) {
	int c;
	char msgbuf[1024];

	::signal(SIGINT, ctrl_c_action);
	::signal(SIGSEGV, ctrl_c_action);

	do {
		if ((c = ::getopt(argc, argv, "C:q:d:l:h")) < 0)
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
			case 'd': // this is an undocumented debug commandline option!
				GetLogger().set_logfile(optarg);
				break;
			case 'l': // this is an undocumented debug commandline option!
				{
					loglevel level = static_cast<loglevel>(atoi(optarg));
					if (level > LOG_NONE && level <= LOG_DEBUG)
						GetLogger().set_loglevel(level);
				}
				break;
			case 'h':
				usage(argv[0]);
				break;
			default:
				snprintf(msgbuf, sizeof(msgbuf), _("%s: unknown option - %c"), argv[0], static_cast<char>(c));
				std::cout << msgbuf << std::endl;
				usage(argv[0]);
				break;
		}
	} while (c != -1);

	snprintf(msgbuf, sizeof(msgbuf), _("Starting %s %s..."), "podbeuter", PROGRAM_VERSION);
	std::cout << msgbuf << std::endl;

	pid_t pid;
	if (!try_fs_lock(pid)) {
		snprintf(msgbuf, sizeof(msgbuf), _("Error: an instance of %s is already running (PID: %u)"), "podbeuter", pid);
		std::cout << msgbuf << std::endl;
		return;
	}

	std::cout << _("Loading configuration...");
	std::cout.flush();

	configparser cfgparser(config_file.c_str());
	cfg = new configcontainer();
	cfg->register_commands(cfgparser);
	colormanager * colorman = new colormanager();
	colorman->register_commands(cfgparser);

	keymap keys;
	cfgparser.register_handler("bind-key", &keys);
	cfgparser.register_handler("unbind-key", &keys);

	v->set_keymap(&keys);

	try {
		cfgparser.parse();
	} catch (const configexception& ex) {
		std::cout << ex.what() << std::endl;
		return;	
	}

	max_dls = cfg->get_configvalue_as_int("max-downloads");

	std::cout << _("done.") << std::endl;

	ql = new queueloader(queue_file, this);
	ql->reload(downloads_);
	
	v->run();

	std::cout <<  _("Cleaning up queue...");
	std::cout.flush();
	
	ql->reload(downloads_);
	delete ql;
	
	std::cout << _("done.") << std::endl;

	remove_fs_lock();
}

void pb_controller::usage(const char * argv0) {
	// TODO
	::exit(EXIT_FAILURE);
}

std::string pb_controller::get_dlpath() {
	return cfg->get_configvalue("download-path");
}

bool pb_controller::try_fs_lock(pid_t & pid) {
	int fd;
	if ((fd = ::open(lock_file.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600)) >= 0) {
		pid = ::getpid();
		::write(fd,&pid,sizeof(pid));
		::close(fd);
		GetLogger().log(LOG_DEBUG,"wrote lock file with pid = %u",pid);
		return true;
	} else {
		pid = 0;
		if ((fd = ::open(lock_file.c_str(), O_RDONLY)) >=0) {
			::read(fd,&pid,sizeof(pid));
			::close(fd);
			GetLogger().log(LOG_DEBUG,"found lock file");
		} else {
			GetLogger().log(LOG_DEBUG,"found lock file, but couldn't open it for reading from it");
		}
		return false;
	}
}

void pb_controller::remove_fs_lock() {
	::unlink(lock_file.c_str());
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

void pb_controller::reload_queue() {
	if (ql) {
		ql->reload(downloads_);
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
			poddlthread * thread = new poddlthread(&(*it));
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


} // namespace

#include <pb_controller.h>
#include <pb_view.h>
#include <config.h>
#include <iostream>
#include <sstream>
#include <cstdio>

#include <sys/types.h>
#include <pwd.h>

#include <keymap.h>
#include <configcontainer.h>
#include <colormanager.h>
#include <exceptions.h>
#include <queueloader.h>

using namespace newsbeuter;

namespace podbeuter {

pb_controller::pb_controller() : v(0), config_file("config"), queue_file("queue"), cfg(0), view_update_(true) { 
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
}

pb_controller::~pb_controller() { 
	delete cfg;
}

void pb_controller::run(int argc, char * argv[]) {
	int c;
	char msgbuf[1024];

	do {
		if ((c = ::getopt(argc, argv, "C:q:h")) < 0)
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

	std::cout << _("done.") << std::endl;

	queueloader ql(queue_file);
	ql.load(downloads_);
	
	v->run();

	std::cout <<  _("Cleaning up queue...");
	std::cout.flush();
	// TODO: clean up queue, if this wasn't done before already
	
	std::cout << _("done.") << std::endl;
}

void pb_controller::usage(const char * argv0) {
	// TODO
	::exit(EXIT_FAILURE);
}

}

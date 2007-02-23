#include <pb_controller.h>
#include <pb_view.h>
#include <config.h>
#include <iostream>
#include <cstdio>

#include <keymap.h>
#include <configcontainer.h>
#include <colormanager.h>
#include <exceptions.h>

using namespace newsbeuter;

namespace podbeuter {

pb_controller::pb_controller() : v(0), cfg(0) { 
}

pb_controller::~pb_controller() { 
	delete cfg;
}

void pb_controller::run(int argc, char * argv[]) {
	char msgbuf[1024];

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

	// TODO: load URLs from queue file
	
	// TODO: start UI
	//
	v->run();

	std::cout <<  _("Cleaning up queue...");
	std::cout.flush();
	// TODO: clean up queue, if this wasn't done before already
	
	std::cout << _("done.") << std::endl;
}

}

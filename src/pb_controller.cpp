#include <pb_controller.h>
#include <pb_view.h>
#include <config.h>
#include <iostream>
#include <cstdio>

namespace podbeuter {

pb_controller::pb_controller() : v(0) { }
pb_controller::~pb_controller() { }

void pb_controller::run(int argc, char * argv[]) {
	char msgbuf[1024];

	snprintf(msgbuf, sizeof(msgbuf), _("Starting %s %s..."), "podbeuter", PROGRAM_VERSION);
	std::cout << msgbuf << std::endl;


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

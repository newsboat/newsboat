#include <iostream>

#include <config.h>
#include <pb_controller.h>
#include <cstring>
#include <pb_view.h>
#include <errno.h>
#include <utils.h>

using namespace podbeuter;

int main(int argc, char * argv[]) {
	utils::initialize_ssl_implementation();

	if (!setlocale(LC_CTYPE,"") || !setlocale(LC_MESSAGES,"")) {
		std::cerr << "setlocale failed: " << strerror(errno) << std::endl;
		return 1;
	}
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	pb_controller c;
	podbeuter::pb_view v(&c);
	c.set_view(&v);

	return c.run(argc, argv);
}

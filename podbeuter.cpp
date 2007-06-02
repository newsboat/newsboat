#include <iostream>

#include <config.h>
#include <pb_controller.h>
#include <pb_view.h>
#include <errno.h>

using namespace podbeuter;

int main(int argc, char * argv[]) {

	if (!setlocale(LC_CTYPE,"") || !setlocale(LC_MESSAGES,"")) {
		std::cerr << "setlocale failed: " << strerror(errno) << std::endl;
		return 1;	
	}
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	pb_controller c;
	podbeuter::pb_view v(&c);
	c.set_view(&v);

	c.run(argc, argv);

	return 0;
}

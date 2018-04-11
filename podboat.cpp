#include <iostream>

#include <config.h>
#include <pb_controller.h>
#include <cstring>
#include <pb_view.h>
#include <errno.h>
#include <utils.h>

using namespace podboat;

int main(int argc, char * argv[]) {
	utils::initialize_ssl_implementation();

	setlocale(LC_CTYPE,"");
	setlocale(LC_MESSAGES,"");

	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	pb_controller c;
	podboat::pb_view v(&c);
	c.set_view(&v);

	return c.run(argc, argv);
}

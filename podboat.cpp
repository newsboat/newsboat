#include <cstring>
#include <errno.h>
#include <iostream>

#include "config.h"
#include "exception.h"
#include "pb_controller.h"
#include "pb_view.h"
#include "utils.h"

using namespace podboat;

int main(int argc, char* argv[])
{
	utils::initialize_ssl_implementation();

	setlocale(LC_CTYPE, "");
	setlocale(LC_MESSAGES, "");

	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	pb_controller c;

	int ret;
	try {
		podboat::pb_view v(&c);
		c.set_view(&v);

		ret = c.run(argc, argv);
	} catch (const newsboat::exception& e) {
		std::cerr << strprintf::fmt(_("Caught newsboat::exception with "
					      "message: %s"),
				     e.what())
			  << std::endl;
		::exit(EXIT_FAILURE);
	}

	return ret;
}

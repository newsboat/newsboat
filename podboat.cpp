#include <cstring>
#include <errno.h>
#include <iostream>

#include "config.h"
#include "exception.h"
#include "pbcontroller.h"
#include "pbview.h"
#include "utils.h"

extern "C" {
	void rs_setup_human_panic(void);
}

using namespace podboat;

int main(int argc, char* argv[])
{
	rs_setup_human_panic();
	utils::initialize_ssl_implementation();

	setlocale(LC_CTYPE, "");
	setlocale(LC_MESSAGES, "");

	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	PbController c;

	int ret;
	try {
		podboat::PbView v(&c);
		c.set_view(&v);

		ret = c.run(argc, argv);
	} catch (const newsboat::Exception& e) {
		std::cerr << strprintf::fmt(_("Caught newsboat::Exception with "
					      "message: %s"),
				     e.what())
			  << std::endl;
		::exit(EXIT_FAILURE);
	}

	return ret;
}

#include <cstring>
#include <errno.h>
#include <iostream>

#include "config.h"
#include "exception.h"
#include "pbcontroller.h"
#include "pbview.h"
#include "utils.h"

using namespace podboat;

int main(int argc, char* argv[])
{
	Utils::initialize_ssl_implementation();

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
		std::cerr << StrPrintf::fmt(_("Caught newsboat::Exception with "
					      "message: %s"),
				     e.what())
			  << std::endl;
		::exit(EXIT_FAILURE);
	}

	return ret;
}

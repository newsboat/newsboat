#include <cstring>
#include <errno.h>
#include <iostream>

#include "cache.h"
#include "config.h"
#include "controller.h"
#include "exception.h"
#include "exceptions.h"
#include "rss.h"
#include "rsspp.h"
#include "view.h"

using namespace newsboat;

int main(int argc, char* argv[])
{
	utils::initialize_ssl_implementation();

	setlocale(LC_CTYPE, "");
	setlocale(LC_MESSAGES, "");

	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	rsspp::parser::global_init();

	controller c;
	newsboat::view v(&c);
	c.set_view(&v);

	int ret;
	try {
		ret = c.run(argc, argv);
	} catch (const newsboat::dbexception& e) {
		std::cerr << strprintf::fmt(
				     _("Caught newsboat::dbexception with "
				       "message: %s"),
				     e.what())
			  << std::endl;
		::exit(EXIT_FAILURE);
	} catch (const newsboat::matcherexception& e) {
		std::cerr << strprintf::fmt(
				     _("Caught newsboat::matcherexception with "
				       "message: %s"),
				     e.what())
			  << std::endl;
		::exit(EXIT_FAILURE);
	} catch (const newsboat::exception& e) {
		std::cerr << strprintf::fmt(
				     _("Caught newsboat::exception with "
				       "message: %s"),
				     e.what())
			  << std::endl;
		::exit(EXIT_FAILURE);
	}

	rsspp::parser::global_cleanup();

	return ret;
}

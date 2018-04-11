#include <iostream>

#include <rss.h>
#include <view.h>
#include <controller.h>
#include <cache.h>
#include <config.h>
#include <rsspp.h>
#include <errno.h>
#include <cstring>

using namespace newsboat;

int main(int argc, char * argv[]) {
	utils::initialize_ssl_implementation();

	setlocale(LC_CTYPE,"");
	setlocale(LC_MESSAGES,"");

	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	rsspp::parser::global_init();

	controller c;
	newsboat::view v(&c);
	c.set_view(&v);

	int ret = c.run(argc,argv);

	rsspp::parser::global_cleanup();

	return ret;
}

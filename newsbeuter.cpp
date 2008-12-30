#include <iostream>

#include <rss.h>
#include <view.h>
#include <controller.h>
#include <cache.h>
#include <config.h>
#include <rsspp.h>
#include <errno.h>

using namespace newsbeuter;

int main(int argc, char * argv[]) {

	if (!setlocale(LC_CTYPE,"") || !setlocale(LC_MESSAGES,"")) {
		std::cerr << "setlocale failed: " << strerror(errno) << std::endl;
		return 1;	
	}
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	rsspp::parser::global_init();

	controller c;
	newsbeuter::view v(&c);
	c.set_view(&v);

	c.run(argc,argv);

	rsspp::parser::global_cleanup();

	return 0;
}

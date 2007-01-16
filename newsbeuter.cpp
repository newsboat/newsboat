#include <iostream>

#include <rss.h>
#include <view.h>
#include <controller.h>
#include <cache.h>
#include <locale.h>

using namespace newsbeuter;

int main(int argc, char * argv[]) {
	
	if (!setlocale(LC_CTYPE,"")) {
		std::cerr << "setlocale failed: " << strerror(errno) << std::endl;
		return 1;	
	}
	
	controller c;
	view v(&c);
	c.set_view(&v);

	c.run(argc,argv);

	return 0;
}

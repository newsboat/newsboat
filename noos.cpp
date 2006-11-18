#include <iostream>

#include <rss.h>
#include <view.h>
#include <controller.h>
#include <cache.h>

using namespace noos;

int main(int argc, char * argv[]) {
	cache ch("cache.db");
	controller c;
	view v(&c);
	c.set_view(&v);

	c.run(argc,argv);

	return 0;
}

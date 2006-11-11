#include <iostream>

#include <rss.h>
#include <view.h>
#include <controller.h>

using namespace noos;

int main(int argc, char * argv[]) {
	controller c;
	view v(&c);
	c.set_view(&v);

	c.run();

	return 0;
}

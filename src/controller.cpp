#include <view.h>
#include <controller.h>

using namespace noos;

controller::controller() : v(0) { }

void controller::set_view(view * vv) {
	v = vv;
}

void controller::run() {
	std::vector<std::string> feeds;
	feeds.push_back("http://blog.fefe.de/rss.xml?html");
	feeds.push_back("http://foobar.xyz/");

	v->set_feedlist(feeds);
	v->run_feedlist();
}

#include <view.h>
#include <controller.h>
#include <sstream>
#include <cstdlib>
#include <iostream>

using namespace noos;

controller::controller() : v(0), rsscache(0) {
	std::ostringstream cfgfile;
	cfgfile << "urls.txt"; // XXX refactor
	cfg.load_config(cfgfile.str());

	if (cfg.get_urls().size() == 0) {
		std::cerr << "error: no URLs configured." << std::endl;
		::exit(1);
	}

	rsscache = new cache("cache.db");
}

controller::~controller() {
	if (rsscache)
		delete rsscache;
}

void controller::set_view(view * vv) {
	v = vv;
}

void controller::run() {
	v->set_feedlist(cfg.get_urls());
	v->run_feedlist();
}

void controller::open_item(rss_item& item) {
	v->run_itemview(item);
}

void controller::open_feed(unsigned int pos) {
	if (pos <= cfg.get_urls().size()) {
		std::string feedurl = cfg.get_urls()[pos];
		v->feedlist_status("Opening feed...");

		rss_parser parser(feedurl.c_str());
		parser.parse();
		rss_feed& feed = parser.get_feed();

		rsscache->externalize_rssfeed(feed);

		v->feedlist_status("");

		if (feed.items().size() == 0) {
			v->feedlist_error("Error: feed contains no items!");
		} else {
			v->run_itemlist(feed);
		}
	} else {
		v->feedlist_error("Error: invalid feed!");
	}
}

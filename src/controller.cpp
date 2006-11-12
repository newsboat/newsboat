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

	for (std::vector<std::string>::const_iterator it=cfg.get_urls().begin(); it != cfg.get_urls().end(); ++it) {
		rss_feed feed;
		feed.rssurl() = *it;
		rsscache->internalize_rssfeed(feed);
		feeds.push_back(feed);
	}
}

controller::~controller() {
	if (rsscache)
		delete rsscache;
}

void controller::set_view(view * vv) {
	v = vv;
}

void controller::run() {
	v->set_feedlist(feeds);
	v->run_feedlist();
}

void controller::open_item(rss_item& item) {
	v->run_itemview(item);
}

void controller::open_feed(unsigned int pos) {
	if (pos < feeds.size()) {
		v->feedlist_status("Opening feed...");

		rss_feed& feed = feeds[pos];

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

void controller::reload(unsigned int pos) {
	if (pos < feeds.size()) {
		std::string msg = "Loading ";
		msg.append(feeds[pos].rssurl());
		msg.append("...");
		v->feedlist_status(msg.c_str());
		rss_parser parser(feeds[pos].rssurl().c_str());
		parser.parse();
		feeds[pos] = parser.get_feed();
		rsscache->externalize_rssfeed(feeds[pos]);
		v->feedlist_status("");
		v->set_feedlist(feeds);
	} else {
		v->feedlist_error("Error: invalid feed!");
	}
}

void controller::reload_all() {
	for (unsigned int i=0;i<feeds.size();++i) {
		this->reload(i);
	}
}

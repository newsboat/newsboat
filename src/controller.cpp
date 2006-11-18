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
		std::cout << "error: no URLs configured." << std::endl;
		::exit(1);
	}

	rsscache = new cache("cache.db");

	for (std::vector<std::string>::const_iterator it=cfg.get_urls().begin(); it != cfg.get_urls().end(); ++it) {
		rss_feed feed;
		feed.rssurl() = *it;
		rsscache->internalize_rssfeed(feed);
		feeds.push_back(feed);
	}
	rsscache->cleanup_cache(feeds);

}

controller::~controller() {
	if (rsscache)
		delete rsscache;
}

void controller::set_view(view * vv) {
	v = vv;
}

void controller::run(int argc, char * argv[]) {
	if (argc>1) {
		if (strcmp(argv[1],"-e")==0) {
			export_opml();
		} else if (strcmp(argv[1],"-i")==0) {
			if (argc>2) {
				import_opml(argv[2]);
			} else {
				usage(argv[0]);
			}
		} else
			usage(argv[0]);
		return;
	}

	v->set_feedlist(feeds);
	v->run_feedlist();
}

void controller::open_item(rss_item& item) {
	v->run_itemview(item);
	item.unread() = false; // XXX: see TODO list
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
			rsscache->externalize_rssfeed(feed); // save possibly changed unread flags
			v->set_feedlist(feeds);
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
		rsscache->internalize_rssfeed(feeds[pos]);
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

void controller::usage(char * argv0) {
	std::cout << argv0 << ": usage: " << argv0 << " [-i <file>|-e]" << std::endl;
	std::cout << "\t-e\texport OPML feed to stdout" << std::endl;
	std::cout << "\t-i <file>\timport OPML file" << std::endl;
	::exit(1);
}

void controller::import_opml(char * filename) {
	std::cout << "Error: unimplemented!" << std::endl;
}

void controller::export_opml() {
	std::cout << "<?xml version=\"1.0\"?>" << std::endl;
	std::cout << "<opml version=\"1.0\">" << std::endl;
	std::cout << "\t<head>" << std::endl << "\t\t<title>noos - Exported Feeds</title>" << std::endl << "\t</head>" << std::endl;
	std::cout << "\t<body>" << std::endl;
	for (std::vector<rss_feed>::iterator it=feeds.begin(); it != feeds.end(); ++it) {
		std::cout << "\t\t<outline type=\"rss\" xmlUrl=\"" << it->rssurl() << "\" text=\"" << it->title() << "\" />" << std::endl;
	}
	std::cout << "\t</body>" << std::endl;
	std::cout << "</opml>" << std::endl;
}

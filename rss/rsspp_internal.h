#ifndef RSSPP_INTERNAL__H
#define RSSPP_INTERNAL__H

#include <rsspp.h>

namespace rsspp {

struct rss_parser {
	virtual void parse_feed(feed& f, xmlNode * rootNode) = 0;
	virtual ~rss_parser() { }
};

struct rss_09x_parser : public rss_parser {
	virtual void parse_feed(feed& f, xmlNode * rootNode);
	item parse_item(xmlNode * itemNode);
};

struct rss_10_parser : public rss_parser {
	virtual void parse_feed(feed& f, xmlNode * rootNode);
};

struct rss_20_parser : public rss_parser {
	virtual void parse_feed(feed& f, xmlNode * rootNode);
};

struct atom_parser : public rss_parser {
	virtual void parse_feed(feed& f, xmlNode * rootNode);
};

struct rss_parser_factory {
	static rss_parser * get_object(feed& f);
};

}

#endif

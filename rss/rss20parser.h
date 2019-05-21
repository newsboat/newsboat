#ifndef NEWSBOAT_RSSPP_RSS20PARSER_H_
#define NEWSBOAT_RSSPP_RSS20PARSER_H_

#include <libxml/tree.h>

#include "rss09xparser.h"

namespace rsspp {

class Feed;

struct Rss20Parser : public Rss09xParser {
	explicit Rss20Parser(xmlDocPtr doc)
		: Rss09xParser(doc)
	{
	}
	void parse_feed(Feed& f, xmlNode* rootNode) override;
	~Rss20Parser() override {}
};

} // namespace rsspp

#endif /* NEWSBOAT_RSSPP_RSS20PARSER_H_ */

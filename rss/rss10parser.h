#ifndef NEWSBOAT_RSSPP_RSS10PARSER_H_
#define NEWSBOAT_RSSPP_RSS10PARSER_H_

#include <libxml/tree.h>

#include "rssparser.h"

namespace rsspp {

class Feed;

struct Rss10Parser : public RssParser {
	void parse_feed(Feed& f, xmlNode* rootNode) override;
	explicit Rss10Parser(xmlDocPtr doc)
		: RssParser(doc)
	{
	}
	~Rss10Parser() override {}
};

} // namespace rsspp

#endif /* NEWSBOAT_RSSPP_RSS10PARSER_H_ */


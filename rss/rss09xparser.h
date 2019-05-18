#ifndef NEWSBOAT_RSSPP_RSS09XPARSER_H_
#define NEWSBOAT_RSSPP_RSS09XPARSER_H_

#include <libxml/tree.h>

#include "rssparser.h"

namespace rsspp {

class Feed;
class Item;

struct Rss09xParser : public RssParser {
	void parse_feed(Feed& f, xmlNode* rootNode) override;
	explicit Rss09xParser(xmlDocPtr doc)
		: RssParser(doc)
		, ns(nullptr)
	{
	}
	~Rss09xParser() override;

protected:
	const char* ns;

private:
	Item parse_item(xmlNode* itemNode);
};

} // namespace rsspp

#endif /* NEWSBOAT_RSSPP_RSS09XPARSER_H_ */

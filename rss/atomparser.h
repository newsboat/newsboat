#ifndef NEWSBOAT_RSSPP_ATOMPARSER_H_
#define NEWSBOAT_RSSPP_ATOMPARSER_H_

#include <libxml/tree.h>

#include "rssparser.h"

namespace rsspp {

class Feed;
class Item;

struct AtomParser : public RssParser {
	void parse_feed(Feed& f, xmlNode* rootNode) override;
	explicit AtomParser(xmlDocPtr doc)
		: RssParser(doc)
		, ns(0)
	{
	}
	~AtomParser() override {}

private:
	Item parse_entry(xmlNode* itemNode);
	void parse_and_update_author(xmlNode* authorNode, std::string& author);
	static std::string content_type_to_mime(const std::string& type);
	const char* ns;
};

} // namespace rsspp

#endif /* NEWSBOAT_RSSPP_ATOMPARSER_H_ */


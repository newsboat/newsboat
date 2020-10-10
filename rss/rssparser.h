#ifndef NEWSBOAT_RSSPP_RSSPARSER_H_
#define NEWSBOAT_RSSPP_RSSPARSER_H_

#include <libxml/tree.h>
#include <string>

namespace rsspp {

class Feed;

struct RssParser {
	virtual void parse_feed(Feed& f, xmlNode* rootNode) = 0;
	explicit RssParser(xmlDocPtr d)
		: doc(d)
	{
	}
	virtual ~RssParser() {}
	static std::string w3cdtf_to_rfc822(const std::string& w3cdtf);

protected:
	xmlDocPtr doc;
	std::string globalbase;
};

} // namespace rsspp

#endif /* NEWSBOAT_RSSPP_RSSPARSER_H_ */

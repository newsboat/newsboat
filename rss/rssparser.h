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
	static std::string __w3cdtf_to_rfc822(const std::string& w3cdtf);

protected:
	std::string get_content(xmlNode* node);
	std::string get_xml_content(xmlNode* node);
	void cleanup_namespaces(xmlNodePtr node);
	std::string get_prop(xmlNode* node,
		const std::string& prop,
		const std::string& ns = "");
	std::string w3cdtf_to_rfc822(const std::string& w3cdtf);
	bool
	node_is(xmlNode* node, const char* name, const char* ns_uri = nullptr);
	xmlDocPtr doc;
	std::string globalbase;
};

} // namespace rsspp

#endif /* NEWSBOAT_RSSPP_RSSPARSER_H_ */

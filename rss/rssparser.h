#ifndef NEWSBOAT_RSSPP_RSSPARSER_H_
#define NEWSBOAT_RSSPP_RSSPARSER_H_

#include <libxml/tree.h>
#include <string>

#define CONTENT_URI "http://purl.org/rss/1.0/modules/content/"
#define RDF_URI "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define ITUNES_URI "http://www.itunes.com/dtds/podcast-1.0.dtd"
#define DC_URI "http://purl.org/dc/elements/1.1/"
#define ATOM_0_3_URI "http://purl.org/atom/ns#"
#define ATOM_1_0_URI "http://www.w3.org/2005/Atom"
#define MEDIA_RSS_URI "http://search.yahoo.com/mrss/"
#define XML_URI "http://www.w3.org/XML/1998/namespace"
#define RSS20USERLAND_URI "http://backend.userland.com/rss2"

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

#ifndef NEWSBOAT_RSSPP_INTERNAL_H_
#define NEWSBOAT_RSSPP_INTERNAL_H_

#include <memory>
#include <libxml/tree.h>

#include "rssparser.h"
#include "rss09xparser.h"
#include "rss20parser.h"
#include "rss10parser.h"

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
	const char* ns;
};

struct RssParserFactory {
	static std::shared_ptr<RssParser> get_object(Feed& f, xmlDocPtr doc);
};

} // namespace rsspp

#endif /* NEWSBOAT_RSSPP_INTERNAL_H_ */

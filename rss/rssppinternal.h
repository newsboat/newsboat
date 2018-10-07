#ifndef NEWSBOAT_RSSPP_INTERNAL_H_
#define NEWSBOAT_RSSPP_INTERNAL_H_

#include <memory>

#include "rsspp.h"

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

struct Rss20Parser : public Rss09xParser {
	explicit Rss20Parser(xmlDocPtr doc)
		: Rss09xParser(doc)
	{
	}
	void parse_feed(Feed& f, xmlNode* rootNode) override;
	~Rss20Parser() override {}
};

struct Rss10Parser : public RssParser {
	void parse_feed(Feed& f, xmlNode* rootNode) override;
	explicit Rss10Parser(xmlDocPtr doc)
		: RssParser(doc)
	{
	}
	~Rss10Parser() override {}
};

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

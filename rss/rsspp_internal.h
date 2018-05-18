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

struct rss_parser {
	virtual void parse_feed(feed& f, xmlNode* rootNode) = 0;
	explicit rss_parser(xmlDocPtr d)
		: doc(d)
	{
	}
	virtual ~rss_parser() {}
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

struct rss_09x_parser : public rss_parser {
	void parse_feed(feed& f, xmlNode* rootNode) override;
	explicit rss_09x_parser(xmlDocPtr doc)
		: rss_parser(doc)
		, ns(nullptr)
	{
	}
	~rss_09x_parser() override;

protected:
	const char* ns;

private:
	item parse_item(xmlNode* itemNode);
};

struct rss_20_parser : public rss_09x_parser {
	explicit rss_20_parser(xmlDocPtr doc)
		: rss_09x_parser(doc)
	{
	}
	void parse_feed(feed& f, xmlNode* rootNode) override;
	~rss_20_parser() override {}
};

struct rss_10_parser : public rss_parser {
	void parse_feed(feed& f, xmlNode* rootNode) override;
	explicit rss_10_parser(xmlDocPtr doc)
		: rss_parser(doc)
	{
	}
	~rss_10_parser() override {}
};

struct atom_parser : public rss_parser {
	void parse_feed(feed& f, xmlNode* rootNode) override;
	explicit atom_parser(xmlDocPtr doc)
		: rss_parser(doc)
		, ns(0)
	{
	}
	~atom_parser() override {}

private:
	item parse_entry(xmlNode* itemNode);
	const char* ns;
};

struct rss_parser_factory {
	static std::shared_ptr<rss_parser> get_object(feed& f, xmlDocPtr doc);
};

} // namespace rsspp

#endif /* NEWSBOAT_RSSPP_INTERNAL_H_ */

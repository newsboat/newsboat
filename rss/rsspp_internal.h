/* rsspp - Copyright (C) 2008-2012 Andreas Krennmair <ak@newsbeuter.org>
 * Licensed under the MIT/X Consortium License. See file LICENSE
 * for more information.
 */

#ifndef RSSPP_INTERNAL__H
#define RSSPP_INTERNAL__H

#include <rsspp.h>
#include <memory>

#define CONTENT_URI		"http://purl.org/rss/1.0/modules/content/"
#define RDF_URI			"http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define ITUNES_URI		"http://www.itunes.com/dtds/podcast-1.0.dtd"
#define DC_URI			"http://purl.org/dc/elements/1.1/"
#define ATOM_0_3_URI	"http://purl.org/atom/ns#"
#define ATOM_1_0_URI	"http://www.w3.org/2005/Atom"
#define MEDIA_RSS_URI	"http://search.yahoo.com/mrss/"
#define XML_URI			"http://www.w3.org/XML/1998/namespace"
#define RSS20USERLAND_URI "http://backend.userland.com/rss2"

namespace rsspp {

struct rss_parser {
		virtual void parse_feed(feed& f, xmlNode * rootNode) = 0;
		rss_parser(xmlDocPtr d) : doc(d) { }
		virtual ~rss_parser() { }
		static std::string __w3cdtf_to_rfc822(const std::string& w3cdtf);
	protected:
		std::string get_content(xmlNode * node);
		std::string get_xml_content(xmlNode * node);
		std::string get_prop(xmlNode * node, const char * prop, const char * ns = NULL);
		std::string w3cdtf_to_rfc822(const std::string& w3cdtf);
		bool node_is(xmlNode * node, const char * name, const char * ns_uri = NULL);
		xmlDocPtr doc;
};

struct rss_09x_parser : public rss_parser {
		virtual void parse_feed(feed& f, xmlNode * rootNode);
		rss_09x_parser(xmlDocPtr doc) : rss_parser(doc), ns(NULL) { }
		virtual ~rss_09x_parser();
	protected:
		const char * ns;
	private:
		item parse_item(xmlNode * itemNode);
};

struct rss_20_parser : public rss_09x_parser {
	rss_20_parser(xmlDocPtr doc) : rss_09x_parser(doc) { }
	virtual void parse_feed(feed& f, xmlNode * rootNode);
	virtual ~rss_20_parser() { }
};

struct rss_10_parser : public rss_parser {
	virtual void parse_feed(feed& f, xmlNode * rootNode);
	rss_10_parser(xmlDocPtr doc) : rss_parser(doc) { }
	virtual ~rss_10_parser() { }
};


struct atom_parser : public rss_parser {
		virtual void parse_feed(feed& f, xmlNode * rootNode);
		atom_parser(xmlDocPtr doc) : rss_parser(doc), ns(0) { }
		virtual ~atom_parser() { }
	private:
		item parse_entry(xmlNode * itemNode);
		std::string globalbase;
		const char * ns;
};

struct rss_parser_factory {
	static std::shared_ptr<rss_parser> get_object(feed& f, xmlDocPtr doc);
};

}

#endif

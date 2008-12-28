/* rsspp - Copyright (C) 2008 Andreas Krennmair <ak@newsbeuter.org>
 * Licensed under the MIT/X Consortium License. See file LICENSE
 * for more information.
 */

#include <rsspp.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

namespace rsspp {

parser::parser(unsigned int timeout, const char * user_agent, const char * proxy, const char * proxy_auth) 
	: to(timeout), ua(user_agent), prx(proxy), prxauth(proxy_auth) {
}

parser::~parser() {
	xmlCleanupParser();
}

feed parser::parse_url(const std::string& url) {
	return parse_file(url);
}

feed parser::parse_buffer(const char * buffer, size_t size) {
	xmlDoc *doc = NULL;

	doc = xmlParseMemory(buffer, size);
	if (doc == NULL) {
		throw exception(0, "unable to parse buffer");
	}

	xmlNode* root_element = xmlDocGetRootElement(doc);

	feed f = parse_xmlnode(root_element);

	xmlFreeDoc(doc);

	throw exception(0, "unimplemented");

	return f;
}

feed parser::parse_file(const std::string& filename) {
	xmlDoc *doc = NULL;

	doc = xmlReadFile(filename.c_str(), NULL, 0);
	if (doc == NULL) {
		throw exception(0, "unable to parse file");
	}

	xmlNode* root_element = xmlDocGetRootElement(doc);

	feed f = parse_xmlnode(root_element);

	xmlFreeDoc(doc);

	throw exception(0, "unimplemented");

	return f;
}

feed parser::parse_xmlnode(xmlNode* node) {
	feed f;

	if (node) {
		if (node->name && node->type == XML_ELEMENT_NODE) {
			if (strcmp((const char *)node->name, "rss")==0) {
				// TODO: parse RSS 0.91, 0.92 or 2.0
			} else if (strcmp((const char *)node->name, "RDF")==0) {
				// TODO: parse RSS 1.0
			} else if (strcmp((const char *)node->name, "atom")==0) {
				// TODO: parse Atom 0.3 or 1.0
			}
		}
	} else {
		// TODO: throw exception
	}

	return f;
}

void parser::global_init() {
	LIBXML_TEST_VERSION
}

void parser::global_cleanup() {
	xmlCleanupParser();
}


}

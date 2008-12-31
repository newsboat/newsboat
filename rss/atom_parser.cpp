/* rsspp - Copyright (C) 2008-2009 Andreas Krennmair <ak@newsbeuter.org>
 * Licensed under the MIT/X Consortium License. See file LICENSE
 * for more information.
 */

#include <rsspp_internal.h>
#include <cstring>

namespace rsspp {


void atom_parser::parse_feed(feed& f, xmlNode * rootNode) {
	if (!rootNode)
		throw exception(0, "rootNode is NULL");

	char * lang = (char *)xmlGetProp(rootNode, (xmlChar *)"lang");
	if (lang) {
		f.language = lang;
		xmlFree(lang);
	}

	for (xmlNode * node = rootNode->children; node != NULL; node = node->next) {
		if (strcmp((const char *)node->name, "title")==0) {
			f.title = get_content(node);
			char * type = (char *)xmlGetProp(node, (xmlChar *)"type");
			if (type) {
				f.title_type = type;
				xmlFree(type);
			} else {
				f.title_type = "text";
			}
		} else if (strcmp((const char *)node->name, "subtitle")==0) {
			f.description = get_content(node);
		} else if (strcmp((const char *)node->name, "link")==0) {
			char * rel = (char *)xmlGetProp(node, (xmlChar *)"rel");
			if (rel) {
				if (strcmp(rel, "alternate")==0) {
					char * href = (char *)xmlGetProp(node, (xmlChar *)"href");
					if (href) {
						f.link = href;
						xmlFree(href);
					}
				}
				xmlFree(rel);
			}
		} else if (strcmp((const char *)node->name, "updated")==0) {
			f.pubDate = w3cdtf_to_rfc822(get_content(node));
		} else if (strcmp((const char *)node->name, "entry")==0) {
			f.items.push_back(parse_entry(node));
		}
	}

}

item atom_parser::parse_entry(xmlNode * entryNode) {
	item it;
	std::string summary;
	std::string summary_type;

	for (xmlNode * node = entryNode->children; node != NULL; node = node->next) {
		if (strcmp((const char *)node->name, "author")==0) {
			for (xmlNode * authornode = node->children; authornode != NULL; authornode = authornode->next) {
				if (strcmp((const char *)authornode->name, "name")==0) {
					it.author = get_content(authornode);
				} // TODO: is there more?
			}
		} else if (strcmp((const char *)node->name, "title")==0) {
			it.title = get_content(node);
			char * type = (char *)xmlGetProp(node, (xmlChar *)"type");
			if (type) {
				it.title_type = type;
				xmlFree(type);
			} else {
				it.title_type = "text";
			}
		} else if (strcmp((const char *)node->name, "content")==0) {
			it.description = get_content(node);
			char * type = (char *)xmlGetProp(node, (xmlChar *)"type");
			if (type) {
				it.description_type = type;
				xmlFree(type);
			} else {
				it.description_type = "text";
			}
		} else if (strcmp((const char *)node->name, "id")==0) {
			it.guid = get_content(node);
			it.guid_isPermaLink = false;
		} else if (strcmp((const char *)node->name, "published")==0) {
			it.pubDate = w3cdtf_to_rfc822(get_content(node));
		} else if (strcmp((const char *)node->name, "link")==0) {
			char * rel = (char *)xmlGetProp(node, (xmlChar *)"rel");
			if (rel) {
				char * href = (char *)xmlGetProp(node, (xmlChar *)"href");
				if (href) {
					it.link = href;
					xmlFree(href);
				}
				xmlFree(rel);
			}
		} else if (strcmp((const char *)node->name, "summary")==0) {
			summary = get_content(node);
			char * type = (char *)xmlGetProp(node, (xmlChar *)"type");
			if (type) {
				summary_type = type;
				xmlFree(type);
			} else {
				summary_type = "text";
			}
		}
	} // for

	if (it.description == "") {
		it.description = summary;
		it.description_type = summary_type;
	}

	return it;
}

}

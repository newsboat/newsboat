/* rsspp - Copyright (C) 2008-2009 Andreas Krennmair <ak@newsbeuter.org>
 * Licensed under the MIT/X Consortium License. See file LICENSE
 * for more information.
 */

#include <config.h>
#include <rsspp_internal.h>
#include <utils.h>
#include <cstring>

namespace rsspp {


void atom_parser::parse_feed(feed& f, xmlNode * rootNode) {
	if (!rootNode)
		throw exception(_("XML root node is NULL"));

	char * lang = (char *)xmlGetProp(rootNode, (xmlChar *)"lang");
	if (lang) {
		f.language = lang;
		xmlFree(lang);
	}

	for (xmlNode * node = rootNode->children; node != NULL; node = node->next) {
		if (node_is(node, "title")) {
			f.title = get_content(node);
			char * type = (char *)xmlGetProp(node, (xmlChar *)"type");
			if (type) {
				f.title_type = type;
				xmlFree(type);
			} else {
				f.title_type = "text";
			}
		} else if (node_is(node, "subtitle")) {
			f.description = get_content(node);
		} else if (node_is(node, "link")) {
			std::string rel = get_prop(node, "rel");
			if (rel == "alternate") {
				f.link = get_prop(node, "href");
			}
		} else if (node_is(node, "updated")) {
			f.pubDate = w3cdtf_to_rfc822(get_content(node));
		} else if (node_is(node, "entry")) {
			f.items.push_back(parse_entry(node));
		}
	}

}

item atom_parser::parse_entry(xmlNode * entryNode) {
	item it;
	std::string summary;
	std::string summary_type;

	std::string base = get_prop(entryNode, "base", XML_URI);

	for (xmlNode * node = entryNode->children; node != NULL; node = node->next) {
		if (node_is(node, "author")) {
			for (xmlNode * authornode = node->children; authornode != NULL; authornode = authornode->next) {
				if (node_is(authornode, "name")) {
					it.author = get_content(authornode);
				} // TODO: is there more?
			}
		} else if (node_is(node, "title")) {
			it.title = get_content(node);
			it.title_type = get_prop(node, "type");
			if (it.title_type == "")
				it.title_type = "text";
		} else if (node_is(node, "content")) {
			it.description = get_content(node);
			it.description_type = get_prop(node, "type");
			if (it.description_type == "")
				it.description_type = "text";
		} else if (node_is(node, "id")) {
			it.guid = get_content(node);
			it.guid_isPermaLink = false;
		} else if (node_is(node, "published")) {
			it.pubDate = w3cdtf_to_rfc822(get_content(node));
		} else if (node_is(node, "link")) {
			std::string rel = get_prop(node, "rel");
			if (rel == "" || rel == "alternate") {
				it.link = newsbeuter::utils::absolute_url(base, get_prop(node, "href"));
			} else if (rel == "enclosure") {
				it.enclosure_url = get_prop(node, "href");
				it.enclosure_type = get_prop(node, "type");
			}
		} else if (node_is(node, "summary")) {
			summary = get_content(node);
			summary_type = get_prop(node, "type");
			if (summary_type == "")
				summary_type = "text";
		}
	} // for

	if (it.description == "") {
		it.description = summary;
		it.description_type = summary_type;
	}

	return it;
}

}

/* rsspp - Copyright (C) 2008-2009 Andreas Krennmair <ak@newsbeuter.org> 
 * Licensed under the MIT/X Consortium License. See file LICENSE 
 * for more information. 
 */
#include <config.h>
#include <rsspp_internal.h>
#include <cstring>

namespace rsspp {

void rss_10_parser::parse_feed(feed& f, xmlNode * rootNode) {
	if (!rootNode)
		throw exception(_("XML root node is NULL"));

	for (xmlNode * node = rootNode->children; node != NULL; node = node->next) {
		if (node_is(node, "channel")) {
			for (xmlNode * cnode = node->children; cnode != NULL; cnode = cnode->next) {
				if (node_is(cnode, "title")) {
					f.title = get_content(cnode);
					f.title_type = "text";
				} else if (node_is(cnode, "link")) {
					f.link = get_content(cnode);
				} else if (node_is(cnode, "description")) {
					f.description = get_content(cnode);
				} else if (node_is(cnode, "date", DC_URI)) {
					f.pubDate = w3cdtf_to_rfc822(get_content(cnode));
				} else if (node_is(cnode, "creator", DC_URI)) {
					f.dc_creator = get_content(cnode);
				}
			}
		} else if (node_is(node, "item")) {
			item it;
			it.guid = get_prop(node, "about", RDF_URI);
			for (xmlNode * itnode = node->children; itnode != NULL; itnode = itnode->next) {
				if (node_is(itnode, "title")) {
					it.title = get_content(itnode);
					it.title_type = "text";
				} else if (node_is(itnode, "link")) {
					it.link = get_content(itnode);
				} else if (node_is(itnode, "description")) {
					it.description = get_content(itnode);
				} else if (node_is(itnode, "date", DC_URI)) {
					it.pubDate = w3cdtf_to_rfc822(get_content(itnode));
				} else if (node_is(itnode, "encoded", CONTENT_URI)) {
					it.content_encoded = get_content(itnode);
				} else if (node_is(itnode, "summary", ITUNES_URI)) {
					it.itunes_summary = get_content(itnode);
				}
			}
			f.items.push_back(it);
		}
	}
}

}

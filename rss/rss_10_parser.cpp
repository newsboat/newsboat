#include "rsspp_internal.h"

#include <cstring>

#include "config.h"

#define RSS_1_0_NS "http://purl.org/rss/1.0/"

namespace rsspp {

void rss_10_parser::parse_feed(feed& f, xmlNode* rootNode)
{
	if (!rootNode)
		throw exception(_("XML root node is NULL"));

	for (xmlNode* node = rootNode->children; node != nullptr;
	     node = node->next) {
		if (node_is(node, "channel", RSS_1_0_NS)) {
			for (xmlNode* cnode = node->children; cnode != nullptr;
			     cnode = cnode->next) {
				if (node_is(cnode, "title", RSS_1_0_NS)) {
					f.title = get_content(cnode);
					f.title_type = "text";
				} else if (node_is(cnode, "link", RSS_1_0_NS)) {
					f.link = get_content(cnode);
				} else if (node_is(cnode,
						   "description",
						   RSS_1_0_NS)) {
					f.description = get_content(cnode);
				} else if (node_is(cnode, "date", DC_URI)) {
					f.pubDate = w3cdtf_to_rfc822(
						get_content(cnode));
				} else if (node_is(cnode, "creator", DC_URI)) {
					f.dc_creator = get_content(cnode);
				}
			}
		} else if (node_is(node, "item", RSS_1_0_NS)) {
			item it;
			it.guid = get_prop(node, "about", RDF_URI);
			for (xmlNode* itnode = node->children;
			     itnode != nullptr;
			     itnode = itnode->next) {
				if (node_is(itnode, "title", RSS_1_0_NS)) {
					it.title = get_content(itnode);
					it.title_type = "text";
				} else if (node_is(itnode,
						   "link",
						   RSS_1_0_NS)) {
					it.link = get_content(itnode);
				} else if (node_is(itnode,
						   "description",
						   RSS_1_0_NS)) {
					it.description = get_content(itnode);
				} else if (node_is(itnode, "date", DC_URI)) {
					it.pubDate = w3cdtf_to_rfc822(
						get_content(itnode));
				} else if (node_is(itnode,
						   "encoded",
						   CONTENT_URI)) {
					it.content_encoded =
						get_content(itnode);
				} else if (node_is(itnode,
						   "summary",
						   ITUNES_URI)) {
					it.itunes_summary = get_content(itnode);
				} else if (node_is(itnode, "creator", DC_URI)) {
					it.author = get_content(itnode);
				}
			}
			f.items.push_back(it);
		}
	}
}

} // namespace rsspp

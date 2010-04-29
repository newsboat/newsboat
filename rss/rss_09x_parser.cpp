/* rsspp - Copyright (C) 2008-2010 Andreas Krennmair <ak@newsbeuter.org>
 * Licensed under the MIT/X Consortium License. See file LICENSE
 * for more information.
 */

#include <config.h>
#include <rsspp_internal.h>
#include <utils.h>
#include <cstring>

using namespace newsbeuter;

namespace rsspp {

void rss_20_parser::parse_feed(feed& f, xmlNode * rootNode) {
	if (!rootNode)
		throw exception(_("XML root node is NULL"));

	const char * ns = rootNode->ns ? (const char *)rootNode->ns->href : NULL; //(const char *)xmlGetProp(rootNode, (const xmlChar *)"xmlns");

	if (ns != NULL) {
		if (strcmp(ns, RSS20USERLAND_URI) == 0) {
			this->ns = strdup(ns);
		}
	}

	rss_09x_parser::parse_feed(f, rootNode);
}

void rss_09x_parser::parse_feed(feed& f, xmlNode * rootNode) {
	if (!rootNode)
		throw exception(_("XML root node is NULL"));

	xmlNode * channel = rootNode->children;
	while (channel && strcmp((const char *)channel->name, "channel")!=0)
		channel = channel->next;

	if (!channel)
		throw exception(_("no RSS channel found"));

	for (xmlNode * node = channel->children; node != NULL; node = node->next) {
		if (node_is(node, "title", ns)) {
			f.title = get_content(node);
			f.title_type = "text";
		} else if (node_is(node, "link", ns)) {
			f.link = get_content(node);
		} else if (node_is(node, "description", ns)) {
			f.description = get_content(node);
		} else if (node_is(node, "language", ns)) {
			f.language = get_content(node);
		} else if (node_is(node, "managingEditor", ns)) {
			f.managingeditor = get_content(node);
		} else if (node_is(node, "item", ns)) {
			f.items.push_back(parse_item(node));
		}
	}
}

item rss_09x_parser::parse_item(xmlNode * itemNode) {
	item it;
	std::string author;
	std::string dc_date;

	for (xmlNode * node = itemNode->children; node != NULL; node = node->next) {
		if (node_is(node, "title", ns)) {
			it.title = get_content(node);
			it.title_type = "text";
		} else if (node_is(node, "link", ns)) {
			it.link = get_content(node);
		} else if (node_is(node, "description", ns)) {
			it.description = get_content(node);
		} else if (node_is(node, "encoded", CONTENT_URI)) {
			it.content_encoded = get_content(node);
		} else if (node_is(node, "summary", ITUNES_URI)) {
			it.itunes_summary = get_content(node);
		} else if (node_is(node, "guid", ns)) {
			it.guid = get_content(node);
			it.guid_isPermaLink = false;
			std::string isPermaLink = get_prop(node,"isPermaLink");
			if (isPermaLink == "true")
				it.guid_isPermaLink = true;
		} else if (node_is(node, "pubDate", ns)) {
			it.pubDate = get_content(node);
		} else if (node_is(node, "date", DC_URI)) {
			dc_date = w3cdtf_to_rfc822(get_content(node));
		} else if (node_is(node, "author", ns)) {
			std::string authorfield = get_content(node);
			if (authorfield[authorfield.length()-1] == ')') {
				it.author_email = newsbeuter::utils::tokenize(authorfield, " ")[0];
				unsigned int start, end;
				end = authorfield.length()-2;
				for (start = end;start > 0 && authorfield[start] != '(';start--) { }
				it.author = authorfield.substr(start+1, end-start);
			} else {
				it.author_email = authorfield;
				it.author = authorfield;
			}
		} else if (node_is(node, "creator", DC_URI)) {
			author = get_content(node);
		} else if (node_is(node, "enclosure", ns)) {
			it.enclosure_url = get_prop(node, "url");
			it.enclosure_type = get_prop(node, "type");
		} else if (node_is(node, "content", MEDIA_RSS_URI)) {
			it.enclosure_url = get_prop(node, "url");
			it.enclosure_type = get_prop(node, "type");
		} else if (node_is(node, "group", MEDIA_RSS_URI)) {
			for (xmlNode * mnode = node->children; mnode != NULL; mnode = mnode->next) {
				if (node_is(mnode, "content", MEDIA_RSS_URI)) {
					it.enclosure_url = get_prop(mnode, "url");
					it.enclosure_type = get_prop(mnode, "type");
				}
			}
		}
	}

	if (it.author == "") {
		it.author = author;
	}

	if (it.pubDate == "") {
		it.pubDate = dc_date;
	}

	return it;
}

rss_09x_parser::~rss_09x_parser() { 
	free((void *)ns);
}

}

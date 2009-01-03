/* rsspp - Copyright (C) 2008-2009 Andreas Krennmair <ak@newsbeuter.org>
 * Licensed under the MIT/X Consortium License. See file LICENSE
 * for more information.
 */

#include <config.h>
#include <rsspp_internal.h>
#include <utils.h>
#include <cstring>

namespace rsspp {

void rss_09x_parser::parse_feed(feed& f, xmlNode * rootNode) {
	if (!rootNode)
		throw exception(_("XML root node is NULL"));

	xmlNode * channel = rootNode->children;
	while (channel && strcmp((const char *)channel->name, "channel")!=0)
		channel = channel->next;

	if (!channel)
		throw exception(_("no RSS channel found"));

	for (xmlNode * node = channel->children; node != NULL; node = node->next) {
		if (node_is(node, "title")) {
			f.title = get_content(node);
			f.title_type = "text";
		} else if (node_is(node, "link")) {
			f.link = get_content(node);
		} else if (node_is(node, "description")) {
			f.description = get_content(node);
		} else if (node_is(node, "language")) {
			f.language = get_content(node);
		} else if (node_is(node, "managingEditor")) {
			f.managingeditor = get_content(node);
		} else if (node_is(node, "item")) {
			f.items.push_back(parse_item(node));
		}
	}
}

item rss_09x_parser::parse_item(xmlNode * itemNode) {
	item it;

	for (xmlNode * node = itemNode->children; node != NULL; node = node->next) {
		if (node_is(node, "title")) {
			it.title = get_content(node);
			it.title_type = "text";
		} else if (node_is(node, "link")) {
			it.link = get_content(node);
		} else if (node_is(node, "description")) {
			it.description = get_content(node);
		} else if (node_is(node, "encoded", CONTENT_URI)) {
			it.content_encoded = get_content(node);
		} else if (node_is(node, "summary", ITUNES_URI)) {
			it.itunes_summary = get_content(node);
		} else if (node_is(node, "guid")) {
			it.guid = get_content(node);
			it.guid_isPermaLink = false;
			std::string isPermaLink = get_prop(node,"isPermaLink");
			if (isPermaLink == "true")
				it.guid_isPermaLink = true;
		} else if (node_is(node, "pubDate")) {
			it.pubDate = get_content(node);
		} else if (node_is(node, "author")) {
			std::string authorfield = get_content(node);
			if (authorfield[authorfield.length()-1] == ')') {
				it.author_email = newsbeuter::utils::tokenize(authorfield, " ")[0];
				unsigned int start, end;
				end = authorfield.length()-2;
				for (start = end;start > 0 && authorfield[start] != '(';start--) { }
				it.author = authorfield.substr(start+1, end-start);
			} else {
				it.author_email = authorfield;
			}
		} else if (node_is(node, "enclosure")) {
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

	return it;
}

}

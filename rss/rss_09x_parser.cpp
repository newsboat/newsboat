/* rsspp - Copyright (C) 2008 Andreas Krennmair <ak@newsbeuter.org>
 * Licensed under the MIT/X Consortium License. See file LICENSE
 * for more information.
 */

#include <rsspp_internal.h>
#include <utils.h>

namespace rsspp {

void rss_09x_parser::parse_feed(feed& f, xmlNode * rootNode) {
	if (!rootNode)
		throw exception(0, "rootNode is NULL");

	xmlNode * channel = rootNode->children;
	while (channel && strcmp((const char *)channel->name, "channel")!=0)
		channel = channel->next;

	if (!channel)
		throw exception(0, "no channel found");

	for (xmlNode * node = channel->children; node != NULL; node = node->next) {
		if (strcmp((const char *)node->name, "title")==0) {
			f.title = get_content(node);
			f.title_type = "text";
		} else if (strcmp((const char *)node->name, "link")==0) {
			f.link = get_content(node);
		} else if (strcmp((const char *)node->name, "description")==0) {
			f.description = get_content(node);
		} else if (strcmp((const char *)node->name, "language")==0) {
			f.language = get_content(node);
		} else if (strcmp((const char *)node->name, "item")==0) {
			f.items.push_back(parse_item(node));
		}
	}
}

item rss_09x_parser::parse_item(xmlNode * itemNode) {
	item it;

	for (xmlNode * node = itemNode->children; node != NULL; node = node->next) {
		if (strcmp((const char *)node->name, "title")==0) {
			it.title = get_content(node);
			it.title_type = "text";
		} else if (strcmp((const char *)node->name, "link")==0) {
			it.link = get_content(node);
		} else if (strcmp((const char *)node->name, "description")==0) {
			it.description = get_content(node);
		} else if (strcmp((const char *)node->name, "encoded")==0) {
			if (node->ns != NULL && node->ns->href != NULL && strcmp((const char *)node->ns->href, CONTENT_URI)==0) {
				it.content_encoded = get_content(node);
			}
		} else if (strcmp((const char *)node->name, "guid")==0) {
			it.guid = get_content(node);
			it.guid_isPermaLink = false;
			const char * isPermaLink = (const char *)xmlGetProp(node, (const xmlChar *)"isPermaLink");
			if (isPermaLink) {
				if (strcmp(isPermaLink, "true")==0)
					it.guid_isPermaLink = true;
				xmlFree((void *)isPermaLink);
			}
		} else if (strcmp((const char *)node->name, "pubDate")==0) {
			it.pubDate = get_content(node);
		} else if (strcmp((const char *)node->name, "author")==0) {
			std::string authorfield = get_content(node);
			if (authorfield[authorfield.length()-1] == ')') {
				it.author_email = newsbeuter::utils::tokenize(authorfield, " ")[0];
				unsigned int start, end;
				end = authorfield.length()-2;
				for (start = end;start > 0 && authorfield[start] != '(';start--);
				it.author = authorfield.substr(start+1, end-start);
			} else {
				it.author_email = authorfield;
			}
		}
	}

	return it;
}

}

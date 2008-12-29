#include <rsspp_internal.h>

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
			const char * content = (const char *)xmlNodeGetContent(node);
			if (content) {
				f.title = content;
				f.title_type = "text";
			}
			xmlFree((void *)content);
		} else if (strcmp((const char *)node->name, "link")==0) {
			const char * content = (const char *)xmlNodeGetContent(node);
			if (content) {
				f.link = content;
			}
			xmlFree((void *)content);
		} else if (strcmp((const char *)node->name, "description")==0) {
			const char * content = (const char *)xmlNodeGetContent(node);
			if (content) {
				f.description = content;
			}
			xmlFree((void *)content);
		} else if (strcmp((const char *)node->name, "language")==0) {
			const char * content = (const char *)xmlNodeGetContent(node);
			if (content) {
				f.language = content;
			}
			xmlFree((void *)content);
		} else if (strcmp((const char *)node->name, "item")==0) {
			f.items.push_back(parse_item(node));
		}
	}
}

item rss_09x_parser::parse_item(xmlNode * itemNode) {
	item it;

	for (xmlNode * node = itemNode->children; node != NULL; node = node->next) {
		if (strcmp((const char *)node->name, "title")==0) {
			const char * content = (const char *)xmlNodeGetContent(node);
			if (content) {
				it.title = content;
				it.title_type = "text";
			}
			xmlFree((void *)content);
		} else if (strcmp((const char *)node->name, "link")==0) {
			const char * content = (const char *)xmlNodeGetContent(node);
			if (content) {
				it.link = content;
			}
			xmlFree((void *)content);
		} else if (strcmp((const char *)node->name, "description")==0) {
			const char * content = (const char *)xmlNodeGetContent(node);
			if (content) {
				it.description = content;
			}
			xmlFree((void *)content);
		}
	}

	return it;
}

}

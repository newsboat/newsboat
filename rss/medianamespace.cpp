#include "medianamespace.h"

#include <string>

#include "utils.h"
#include "xmlutilities.h"

#define MEDIA_RSS_URI "http://search.yahoo.com/mrss/"

using namespace newsboat;

namespace rsspp {

bool is_media_node(xmlNode* node)
{
	return has_namespace(node, MEDIA_RSS_URI);
}

void parse_media_node(xmlNode* node, Item& it, Enclosure* enclosure)
{
	if (node_is(node, "group", MEDIA_RSS_URI)) {
		for (xmlNode* mnode = node->children; mnode != nullptr; mnode = mnode->next) {
			parse_media_node(mnode, it);
		}
	} else if (node_is(node, "content", MEDIA_RSS_URI)) {
		it.enclosures.push_back(
		Enclosure {
			get_prop(node, "url"),
			get_prop(node, "type"),
			"",
			"",
		}
		);
		for (xmlNode* mnode = node->children; mnode != nullptr; mnode = mnode->next) {
			parse_media_node(mnode, it, &it.enclosures.back());
		}
	} else if (node_is(node, "description", MEDIA_RSS_URI)) {
		const std::string description = get_content(node);
		const std::string type = get_prop(node, "type");
		const std::string mime_type = (type == "html" ? "text/html" : "text/plain");
		if (it.description.empty()) {
			it.description = description;
			it.description_mime_type = mime_type;
		}
		if (enclosure) {
			enclosure->description = description;
			enclosure->description_mime_type = mime_type;
		} else {
			if (it.description.empty()) {
				it.description = description;
				it.description_mime_type = mime_type;
			}
		}
	} else if (node_is(node, "title", MEDIA_RSS_URI)) {
		const std::string title = get_content(node);
		const std::string type = get_prop(node, "type");
		const std::string mime_type = (type == "html" ? "text/html" : "text/plain");
		if (it.title.empty()) {
			it.title = title;
			it.title_type = type;
		}
		if (enclosure) {
			if (enclosure->description.empty()) {
				enclosure->description = title;
				enclosure->description_mime_type = mime_type;
			}
		} else {
			if (it.title.empty()) {
				it.title = title;
			}
		}
	} else if (node_is(node, "player", MEDIA_RSS_URI)) {
		if (it.link.empty()) {
			it.link = get_prop(node, "url");
		}
	}
}

} // namespace rsspp

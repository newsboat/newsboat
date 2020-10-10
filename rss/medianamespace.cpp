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

void parse_media_node(xmlNode* node, Item& it)
{
	if (node_is(node, "group", MEDIA_RSS_URI)) {
		for (xmlNode* mnode = node->children; mnode != nullptr; mnode = mnode->next) {
			parse_media_node(mnode, it);
		}
	} else if (node_is(node, "content", MEDIA_RSS_URI)) {
		const std::string type = get_prop(node, "type");
		if (utils::is_valid_podcast_type(type)) {
			it.enclosure_url = get_prop(node, "url");
			it.enclosure_type = std::move(type);
		}
		for (xmlNode* mnode = node->children; mnode != nullptr; mnode = mnode->next) {
			parse_media_node(mnode, it);
		}
	} else if (node_is(node, "description", MEDIA_RSS_URI)) {
		const std::string type = get_prop(node, "type");
		if (it.description.empty()) {
			it.description = get_content(node);
			if (type == "html") {
				it.description_type = "html";
			} else {
				it.description_type = "text";
			}
		}
	} else if (node_is(node, "title", MEDIA_RSS_URI)) {
		if (it.title.empty()) {
			it.title = get_content(node);
		}
	} else if (node_is(node, "player", MEDIA_RSS_URI)) {
		if (it.link.empty()) {
			it.link = get_prop(node, "url");
		}
	}
}

} // namespace rsspp

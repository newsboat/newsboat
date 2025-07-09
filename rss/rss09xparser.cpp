#include "rss09xparser.h"

#include <cstring>

#include "config.h"
#include "exception.h"
#include "feed.h"
#include "item.h"
#include "medianamespace.h"
#include "rsspp_uris.h"
#include "utils.h"
#include "xmlutilities.h"

using namespace newsboat;

namespace rsspp {

void Rss09xParser::parse_feed(Feed& f, xmlNode* rootNode)
{
	if (!rootNode) {
		throw Exception(_("XML root node is NULL"));
	}

	globalbase = get_prop(rootNode, "base", XML_URI);

	xmlNode* channel = rootNode->children;
	while (channel && channel->name && strcmp((const char*)channel->name, "channel") != 0) {
		channel = channel->next;
	}

	if (!channel || channel->name) {
		throw Exception(_("no RSS channel found"));
	}

	for (xmlNode* node = channel->children; node != nullptr;
		node = node->next) {
		if (node_is(node, "title", ns)) {
			f.title = get_content(node);
			f.title_type = "text";
		} else if (node_is(node, "link", ns)) {
			f.link = utils::absolute_url(
					globalbase, get_content(node));
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

Item Rss09xParser::parse_item(xmlNode* itemNode)
{
	Item it;
	std::string author;
	std::string dc_date;

	std::string base = get_prop(itemNode, "base", XML_URI);
	if (base.empty()) {
		base = globalbase;
	}

	for (xmlNode* node = itemNode->children; node != nullptr;
		node = node->next) {
		if (node_is(node, "title", ns)) {
			it.title = get_content(node);
			it.title_type = "text";
		} else if (node_is(node, "link", ns)) {
			if (it.link.empty() || !utils::is_http_url(it.link)) {
				it.link = utils::absolute_url(base, get_content(node));
			}
		} else if (node_is(node, "description", ns)) {
			it.base = get_prop(node, "base", XML_URI);
			if (it.base.empty()) {
				it.base = base;
			}
			it.description = get_content(node);
			it.description_mime_type = "";
		} else if (node_is(node, "encoded", CONTENT_URI)) {
			it.content_encoded = get_content(node);
		} else if (node_is(node, "summary", ITUNES_URI)) {
			it.itunes_summary = get_content(node);
		} else if (node_is(node, "guid", ns)) {
			it.guid = get_content(node);
			if (get_prop(node, "isPermaLink") == "false") {
				it.guid_isPermaLink = false;
			} else {
				it.guid_isPermaLink = true;
				it.guid = utils::absolute_url(base, it.guid);
			}
		} else if (node_is(node, "pubDate", ns)) {
			it.pubDate = get_content(node);
		} else if (node_is(node, "date", DC_URI)) {
			dc_date = w3cdtf_to_rfc822(get_content(node));
		} else if (node_is(node, "author", ns)) {
			const std::string authorfield = get_content(node);
			std::string name;
			std::string email;
			utils::parse_rss_author_email({authorfield.begin(), authorfield.end()}, name, email);

			it.author = name;
			it.author_email = email;
		} else if (node_is(node, "creator", DC_URI)) {
			author = get_content(node);
		} else if (node_is(node, "enclosure", ns)) {
			it.enclosures.push_back(
			Enclosure {
				get_prop(node, "url"),
				get_prop(node, "type"),
				"",
				"",
			}
			);
		} else if (is_media_node(node)) {
			parse_media_node(node, it);
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

Rss09xParser::~Rss09xParser()
{
	free((void*)ns);
}

} // namespace rsspp

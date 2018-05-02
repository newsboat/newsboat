#include "rsspp_internal.h"

#include <cstring>

#include "config.h"
#include "utils.h"

using namespace newsboat;

namespace rsspp {

void rss_20_parser::parse_feed(feed& f, xmlNode* rootNode)
{
	if (!rootNode)
		throw exception(_("XML root node is NULL"));

	if (rootNode->ns) {
		const char* ns = (const char*)rootNode->ns->href;
		if (strcmp(ns, RSS20USERLAND_URI) == 0) {
			this->ns = strdup(ns);
		}
	}

	rss_09x_parser::parse_feed(f, rootNode);
}

void rss_09x_parser::parse_feed(feed& f, xmlNode* rootNode)
{
	if (!rootNode)
		throw exception(_("XML root node is NULL"));

	globalbase = get_prop(rootNode, "base", XML_URI);

	xmlNode* channel = rootNode->children;
	while (channel && strcmp((const char*)channel->name, "channel") != 0)
		channel = channel->next;

	if (!channel)
		throw exception(_("no RSS channel found"));

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

item rss_09x_parser::parse_item(xmlNode* itemNode)
{
	item it;
	std::string author;
	std::string dc_date;

	std::string base = get_prop(itemNode, "base", XML_URI);
	if (base.empty())
		base = globalbase;

	for (xmlNode* node = itemNode->children; node != nullptr;
		node = node->next) {
		if (node_is(node, "title", ns)) {
			it.title = get_content(node);
			it.title_type = "text";
		} else if (node_is(node, "link", ns)) {
			it.link = utils::absolute_url(base, get_content(node));
		} else if (node_is(node, "description", ns)) {
			it.base = get_prop(node, "base", XML_URI);
			if (it.base.empty())
				it.base = base;
			it.description = get_content(node);
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
			std::string authorfield = get_content(node);
			if (authorfield[authorfield.length() - 1] == ')') {
				it.author_email =
					utils::tokenize(authorfield, " ")[0];
				unsigned int start, end;
				end = authorfield.length() - 2;
				for (start = end;
					start > 0 && authorfield[start] != '(';
					start--) {
				}
				it.author = authorfield.substr(
					start + 1, end - start);
			} else {
				it.author_email = authorfield;
				it.author = authorfield;
			}
		} else if (node_is(node, "creator", DC_URI)) {
			author = get_content(node);
		} else if (node_is(node, "enclosure", ns)) {
			const std::string type = get_prop(node, "type");
			if (utils::is_valid_podcast_type(type)) {
				it.enclosure_url = get_prop(node, "url");
				it.enclosure_type = std::move(type);
			}
		} else if (node_is(node, "content", MEDIA_RSS_URI)) {
			const std::string type = get_prop(node, "type");
			if (utils::is_valid_podcast_type(type)) {
				it.enclosure_url = get_prop(node, "url");
				it.enclosure_type = std::move(type);
			}
		} else if (node_is(node, "group", MEDIA_RSS_URI)) {
			for (xmlNode* mnode = node->children; mnode != nullptr;
				mnode = mnode->next) {
				if (node_is(mnode, "content", MEDIA_RSS_URI)) {
					const std::string type =
						get_prop(mnode, "type");
					if (utils::is_valid_podcast_type(
						    type)) {
						it.enclosure_url =
							get_prop(mnode, "url");
						it.enclosure_type =
							std::move(type);
					}
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

rss_09x_parser::~rss_09x_parser()
{
	free((void*)ns);
}

} // namespace rsspp

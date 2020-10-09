#include "atomparser.h"

#include <cstring>

#include "config.h"
#include "exception.h"
#include "feed.h"
#include "item.h"
#include "rsspp_uris.h"
#include "utils.h"
#include "xmlutilities.h"

namespace rsspp {

void AtomParser::parse_feed(Feed& f, xmlNode* rootNode)
{
	if (!rootNode) {
		throw Exception(_("XML root node is NULL"));
	}

	switch (f.rss_version) {
	case Feed::ATOM_0_3:
		ns = ATOM_0_3_URI;
		break;
	case Feed::ATOM_1_0:
		ns = ATOM_1_0_URI;
		break;
	case Feed::ATOM_0_3_NONS:
		ns = nullptr;
		break;
	default:
		ns = nullptr;
		break;
	}

	f.language = get_prop(rootNode, "lang");
	globalbase = get_prop(rootNode, "base", XML_URI);

	for (xmlNode* node = rootNode->children; node != nullptr;
		node = node->next) {
		if (node_is(node, "title", ns)) {
			f.title = get_content(node);
			f.title_type = get_prop(node, "type");
			if (f.title_type == "") {
				f.title_type = "text";
			}
		} else if (node_is(node, "subtitle", ns)) {
			f.description = get_content(node);
		} else if (node_is(node, "link", ns)) {
			std::string rel = get_prop(node, "rel");
			if (rel == "alternate") {
				f.link = newsboat::utils::absolute_url(
						globalbase, get_prop(node, "href"));
			}
		} else if (node_is(node, "updated", ns)) {
			f.pubDate = w3cdtf_to_rfc822(get_content(node));
		} else if (node_is(node, "entry", ns)) {
			f.items.push_back(parse_entry(node));
		}
	}
}

Item AtomParser::parse_entry(xmlNode* entryNode)
{
	Item it;
	std::string summary;
	std::string summary_type;
	std::string updated;

	std::string base = get_prop(entryNode, "base", XML_URI);
	if (base == "") {
		base = globalbase;
	}

	for (xmlNode* node = entryNode->children; node != nullptr;
		node = node->next) {
		if (node_is(node, "author", ns)) {
			for (xmlNode* authornode = node->children;
				authornode != nullptr;
				authornode = authornode->next) {
				if (node_is(authornode, "name", ns)) {
					if (!it.author.empty()) {
						it.author += ", ";
					}
					it.author += get_content(authornode);
				} // TODO: is there more?
			}
		} else if (node_is(node, "title", ns)) {
			it.title = get_content(node);
			it.title_type = get_prop(node, "type");
			if (it.title_type == "") {
				it.title_type = "text";
			}
		} else if (node_is(node, "content", ns)) {
			std::string mode = get_prop(node, "mode");
			std::string type = get_prop(node, "type");
			if (mode == "xml" || mode == "") {
				if (type == "html" || type == "text") {
					it.description = get_content(node);
				} else {
					it.description = get_xml_content(node, doc);
				}
			} else if (mode == "escaped") {
				it.description = get_content(node);
			}
			it.description_type = type;
			if (it.description_type == "") {
				it.description_type = "text";
			}
			it.base = get_prop(node, "base", XML_URI);
			if (it.base.empty()) {
				it.base = base;
			}
		} else if (node_is(node, "id", ns)) {
			it.guid = get_content(node);
			it.guid_isPermaLink = false;
		} else if (node_is(node, "published", ns)) {
			it.pubDate = w3cdtf_to_rfc822(get_content(node));
		} else if (node_is(node, "updated", ns)) {
			updated = w3cdtf_to_rfc822(get_content(node));
		} else if (node_is(node, "link", ns)) {
			std::string rel = get_prop(node, "rel");
			if (rel == "" || rel == "alternate") {
				it.link = newsboat::utils::absolute_url(
						base, get_prop(node, "href"));
			} else if (rel == "enclosure") {
				const std::string type = get_prop(node, "type");
				if (newsboat::utils::is_valid_podcast_type(
						type)) {
					it.enclosure_url =
						get_prop(node, "href");
					it.enclosure_type = std::move(type);
				}
			}
		} else if (node_is(node, "summary", ns)) {
			std::string mode = get_prop(node, "mode");
			summary_type = get_prop(node, "type");
			if (mode == "xml" || mode == "") {
				if (summary_type == "html" ||
					summary_type == "text") {
					summary = get_content(node);
				} else {
					summary = get_xml_content(node, doc);
				}
			} else if (mode == "escaped") {
				summary = get_content(node);
			}
			if (summary_type == "") {
				summary_type = "text";
			}
		} else if (node_is(node, "category", ns) &&
			get_prop(node, "scheme") ==
			"http://www.google.com/reader/") {
			it.labels.push_back(get_prop(node, "label"));
		}
	} // for

	if (it.description == "") {
		it.description = summary;
		it.description_type = summary_type;
	}

	if (it.pubDate == "") {
		it.pubDate = updated;
	}

	return it;
}

} // namespace rsspp

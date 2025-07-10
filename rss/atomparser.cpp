#include "atomparser.h"

#include "config.h"
#include "exception.h"
#include "feed.h"
#include "item.h"
#include "medianamespace.h"
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
	std::string author;

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
			const std::string rel = get_prop(node, "rel");
			if (rel == "alternate") {
				f.link = Newsboat::utils::absolute_url(
						globalbase, get_prop(node, "href"));
			}
		} else if (node_is(node, "updated", ns)) {
			f.pubDate = w3cdtf_to_rfc822(get_content(node));
		} else if (node_is(node, "author", ns)) {
			parse_and_update_author(node, author);
		} else if (node_is(node, "entry", ns)) {
			f.items.push_back(parse_entry(node));
		}
	}

	if (!author.empty()) {
		for (auto& it : f.items) {
			if (it.author.empty()) {
				it.author = author;
			}
		}
	}
}

Item AtomParser::parse_entry(xmlNode* entryNode)
{
	Item it;
	std::string author;
	std::string summary;
	std::string summary_mime_type;
	std::string updated;

	std::string base = get_prop(entryNode, "base", XML_URI);
	if (base == "") {
		base = globalbase;
	}

	for (xmlNode* node = entryNode->children; node != nullptr;
		node = node->next) {
		if (node_is(node, "author", ns)) {
			parse_and_update_author(node, it.author);
		} else if (node_is(node, "source", ns)) {
			for (xmlNode* child = node->children; child != nullptr;
				child = child->next) {
				if (node_is(child, "author", ns)) {
					parse_and_update_author(child, author);
				}
			}
		} else if (node_is(node, "title", ns)) {
			it.title = get_content(node);
			it.title_type = get_prop(node, "type");
			if (it.title_type == "") {
				it.title_type = "text";
			}
		} else if (node_is(node, "content", ns)) {
			const std::string mode = get_prop(node, "mode");
			const std::string type = get_prop(node, "type");
			if (mode == "xml" || mode == "") {
				if (type == "html" || type == "text" || type == "text/plain") {
					it.description = get_content(node);
				} else {
					it.description = get_xml_content(node, doc);
				}
			} else if (mode == "escaped") {
				it.description = get_content(node);
			}
			it.description_mime_type = content_type_to_mime(type);
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
			const std::string rel = get_prop(node, "rel");
			if (rel == "" || rel == "alternate") {
				if (it.link.empty() || !Newsboat::utils::is_http_url(it.link)) {
					it.link = Newsboat::utils::absolute_url(
							base, get_prop(node, "href"));
				}
			} else if (rel == "enclosure") {
				it.enclosures.push_back(
				Enclosure {
					get_prop(node, "href"),
					get_prop(node, "type"),
					"",
					"",
				}
				);
			}
		} else if (node_is(node, "summary", ns)) {
			const std::string mode = get_prop(node, "mode");
			const std::string type = get_prop(node, "type");
			if (mode == "xml" || mode == "") {
				if (type == "html" ||
					type == "text") {
					summary = get_content(node);
				} else {
					summary = get_xml_content(node, doc);
				}
			} else if (mode == "escaped") {
				summary = get_content(node);
			}
			summary_mime_type = content_type_to_mime(type);
		} else if (node_is(node, "category", ns) &&
			get_prop(node, "scheme") ==
			"http://www.google.com/reader/") {
			it.labels.push_back(get_prop(node, "label"));
		} else if (is_media_node(node)) {
			parse_media_node(node, it);
		}
	} // for

	if (it.author == "") {
		it.author = author;
	}

	if (it.description == "") {
		it.description = summary;
		it.description_mime_type = summary_mime_type;
	}

	if (it.pubDate == "") {
		it.pubDate = updated;
	}

	return it;
}

void AtomParser::parse_and_update_author(xmlNode* authorNode, std::string& author)
{
	for (xmlNode* child = authorNode->children; child != nullptr;
		child = child->next) {
		if (node_is(child, "name", ns)) {
			if (!author.empty()) {
				author += ", ";
			}
			author += get_content(child);
		}
	}
}

std::string AtomParser::content_type_to_mime(const std::string& type)
{
	// Convert type to mime-type according to:
	// https://tools.ietf.org/html/rfc4287#section-4.1.3.1
	if (type == "html") {
		return "text/html";
	} else if (type == "xhtml") {
		return "application/xhtml+xml";
	} else if (type == "text") {
		return "text/plain";
	} else if (!type.empty()) {
		return type;
	} else {
		return "text/plain";
	}
}

} // namespace rsspp

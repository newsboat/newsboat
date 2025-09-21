#include "opml.h"

#include <algorithm>
#include <cinttypes>
#include <cstring>
#include <sstream>

#include "logger.h"
#include "rssfeed.h"

namespace newsboat {

xmlDocPtr opml::generate(const FeedContainer& feedcontainer, bool version2)
{
	xmlDocPtr root = xmlNewDoc((const xmlChar*)"1.0");
	xmlNodePtr opml_node =
		xmlNewDocNode(root, nullptr, (const xmlChar*)"opml", nullptr);
	xmlSetProp(opml_node,
		(const xmlChar*)"version",
		(const xmlChar*)(version2 ? "2.0" : "1.0"));
	xmlDocSetRootElement(root, opml_node);

	xmlNodePtr head = xmlNewTextChild(
			opml_node, nullptr, (const xmlChar*)"head", nullptr);
	xmlNewTextChild(head,
		nullptr,
		(const xmlChar*)"title",
		(const xmlChar*)PROGRAM_NAME " - Exported Feeds");
	xmlNodePtr body = xmlNewTextChild(
			opml_node, nullptr, (const xmlChar*)"body", nullptr);

	for (const auto& feed : feedcontainer.get_all_feeds()) {
		if (!utils::is_special_url(feed->rssurl())) {
			std::string rssurl = feed->rssurl();
			std::string link = feed->link();
			std::string title = feed->title();

			xmlNodePtr outline = xmlNewTextChild(body,
					nullptr,
					(const xmlChar*)"outline",
					nullptr);
			xmlSetProp(outline,
				(const xmlChar*)"type",
				(const xmlChar*)"rss");
			xmlSetProp(outline,
				(const xmlChar*)"xmlUrl",
				(const xmlChar*)rssurl.c_str());
			xmlSetProp(outline,
				(const xmlChar*)"htmlUrl",
				(const xmlChar*)link.c_str());
			xmlSetProp(outline,
				(const xmlChar*)"title",
				(const xmlChar*)title.c_str());
			xmlSetProp(outline,
				(const xmlChar*)"text",
				(const xmlChar*)title.c_str());

			if (version2) {
				// OPML 2.0 supports including tags
				std::vector<std::string> tags = feed->get_tags();
				std::string opml_tags;

				bool first_tag = true;
				for (auto t : tags) {
					utils::trim(t);
					t = utils::replace_all(t, ",", "_");
					if (first_tag) {
						first_tag = false;
					} else {
						opml_tags.append(",");
					}
					opml_tags.append(t);
				}
				xmlSetProp(outline,
					(const xmlChar*)"category",
					(const xmlChar*)opml_tags.c_str());
			}
		}
	}

	return root;
}

void rec_find_rss_outlines(
	FileUrlReader& urlcfg,
	xmlNode* node,
	std::string tag)
{
	while (node) {
		std::string newtag = tag;

		if (strcmp((const char*)node->name, "outline") == 0) {
			char* url_p = (char*)xmlGetProp(
					node, (const xmlChar*)"xmlUrl");
			if (!url_p) {
				url_p = (char*)xmlGetProp(
						node, (const xmlChar*)"url");
			}

			if (url_p) {
				const std::string url(url_p);
				xmlFree(url_p);

				LOG(Level::DEBUG,
					"opml::import: found RSS outline with "
					"url = "
					"%s",
					url);

				std::string nurl = std::string(url);

				// Liferea uses a pipe to signal feeds read from
				// the output of a program in its OPMLs. Convert
				// them to our syntax.
				if (url.length() >= 1 && url[0] == '|') {
					nurl = strprintf::fmt(
							"exec:%s", url.substr(1));
					LOG(Level::DEBUG,
						"opml::import: liferea-style "
						"url %s converted to %s",
						url,
						nurl);
				}

				// Handle OPML filters.
				char* filtercmd = (char*)xmlGetProp(
						node, (const xmlChar*)"filtercmd");
				if (filtercmd) {
					LOG(Level::DEBUG,
						"opml::import: adding filter "
						"command %s to url %s",
						filtercmd,
						nurl);
					nurl.insert(0,
						strprintf::fmt("filter:%s:",
							filtercmd));
					xmlFree(filtercmd);
				}

				// Filters and scripts may have arguments, so,
				// quote them when needed.
				const std::string quoted_url = utils::quote_if_necessary(nurl);

				LOG(Level::DEBUG,
					"opml::import: size = %" PRIu64,
					static_cast<uint64_t>(urlcfg.get_urls().size()));

				auto& urls = urlcfg.get_urls();
				if (std::find(urls.begin(), urls.end(), quoted_url) == urls.end()) {
					LOG(Level::DEBUG, "opml::import: added url = %s", quoted_url);
					std::vector<std::string> tags;

					char* text_p = (char*)xmlGetProp(node, (const xmlChar*)"text");
					if (!text_p) {
						text_p = (char*)xmlGetProp(node, (const xmlChar*)"title");
					}
					if (text_p) {
						if (text_p[0] != '\0') {
							tags.push_back(std::string{"~"} + text_p);
						}
						xmlFree(text_p);
					}

					if (tag.length() > 0) {
						LOG(Level::DEBUG,
							"opml::import: appending tag %s to url %s",
							tag,
							quoted_url);
						tags.push_back(tag);
					}

					urlcfg.add_url(quoted_url, tags);
				} else {
					LOG(Level::DEBUG,
						"opml::import: url = %s is already in list",
						quoted_url);
				}

				// Add tags
				std::string token;
				std::istringstream ss;
				char* category = (char*)xmlGetProp(node, (const xmlChar*)"category");
				if (category) {
					ss = std::istringstream(category);
				}

				auto& urltags = urlcfg.get_tags(quoted_url);
				while (std::getline(ss, token, ',')) {
					if (std::find(urltags.begin(), urltags.end(), token) == urltags.end()) {
						urltags.push_back(token);
					}
				}

				xmlFree(category);

			} else {
				char* text = (char*)xmlGetProp(
						node, (const xmlChar*)"text");
				if (!text) {
					text = (char*)xmlGetProp(
							node, (const xmlChar*)"title");
				}
				if (text) {
					if (newtag.length() > 0) {
						newtag.append("/");
					}
					newtag.append(text);
					xmlFree(text);
				}
			}
		}
		rec_find_rss_outlines(urlcfg, node->children, newtag);

		node = node->next;
	}
}

std::optional<std::string> opml::import(
	const Filepath& filename,
	FileUrlReader& urlcfg)
{
	xmlDoc* doc = xmlReadFile(filename.to_locale_string().c_str(), nullptr, 0);
	if (doc == nullptr) {
		return strprintf::fmt(_("Error: failed to parse OPML file \"%s\""), filename);
	}

	std::optional<std::string> error_message;

	xmlNode* root = xmlDocGetRootElement(doc);
	if (strcmp((const char*)root->name, "opml") != 0) {
		xmlFreeDoc(doc);
		return _("the <opml> root element is missing");
	}

	bool foundBody = false;
	for (xmlNode* node = root->children; node != nullptr;
		node = node->next) {
		if (strcmp((const char*)node->name, "body") == 0) {
			foundBody = true;
			LOG(Level::DEBUG, "opml::import: found body");
			rec_find_rss_outlines(urlcfg, node->children, "");

			error_message = urlcfg.write_config();
			if (error_message.has_value()) {
				break;
			}
		}
	}

	xmlFreeDoc(doc);

	if (!foundBody) {
		return _("the <body> element in the <opml> root element is missing");
	}

	return error_message;
}

} // namespace newsboat

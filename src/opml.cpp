#include "opml.h"

#include <cassert>
#include <cinttypes>
#include <cstring>

#include "rssfeed.h"

namespace newsboat {

xmlDocPtr opml::generate(const FeedContainer& feedcontainer)
{
	xmlDocPtr root = xmlNewDoc((const xmlChar*)"1.0");
	xmlNodePtr opml_node =
		xmlNewDocNode(root, nullptr, (const xmlChar*)"opml", nullptr);
	xmlSetProp(opml_node, (const xmlChar*)"version", (const xmlChar*)"1.0");
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
			char* url = (char*)xmlGetProp(
					node, (const xmlChar*)"xmlUrl");
			if (!url) {
				url = (char*)xmlGetProp(
						node, (const xmlChar*)"url");
			}

			if (url) {
				LOG(Level::DEBUG,
					"opml::import: found RSS outline with "
					"url = "
					"%s",
					url);

				std::string nurl = std::string(url);

				// Liferea uses a pipe to signal feeds read from
				// the output of a program in its OPMLs. Convert
				// them to our syntax.
				if (*url == '|') {
					nurl = strprintf::fmt(
							"exec:%s", url + 1);
					LOG(Level::DEBUG,
						"opml::import: liferea-style "
						"url %s "
						"converted to %s",
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

				xmlFree(url);
				// Filters and scripts may have arguments, so,
				// quote them when needed.
				// TODO: get rid of xmlStrdup, it's useless
				url = (char*)xmlStrdup(
						(const xmlChar*)
						utils::quote_if_necessary(nurl)
						.c_str());
				assert(url);

				bool found = false;

				LOG(Level::DEBUG,
					"opml::import: size = %" PRIu64,
					static_cast<uint64_t>(urlcfg.get_urls().size()));
				// TODO: replace with algorithm::any or something
				if (urlcfg.get_urls().size() > 0) {
					for (const auto& u :
						urlcfg.get_urls()) {
						if (u == url) {
							found = true;
						}
					}
				}

				if (!found) {
					LOG(Level::DEBUG,
						"opml::import: added url = %s",
						url);
					urlcfg.get_urls().push_back(
						std::string(url));
					if (tag.length() > 0) {
						LOG(Level::DEBUG,
							"opml::import: "
							"appending "
							"tag %s to url %s",
							tag,
							url);
						urlcfg.get_tags(url).push_back(
							tag);
					}
				} else {
					LOG(Level::DEBUG,
						"opml::import: url = %s is "
						"already "
						"in list",
						url);
				}
				xmlFree(url);
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

nonstd::optional<std::string> opml::import(
	const std::string& filename,
	FileUrlReader& urlcfg)
{
	xmlDoc* doc = xmlReadFile(filename.c_str(), nullptr, 0);
	if (doc == nullptr) {
		return strprintf::fmt(_("Error: Failed to parse OPML file \"%s\""), filename);
	}

	nonstd::optional<std::string> error_message;

	xmlNode* root = xmlDocGetRootElement(doc);
	for (xmlNode* node = root->children; node != nullptr;
		node = node->next) {
		if (strcmp((const char*)node->name, "body") == 0) {
			LOG(Level::DEBUG, "opml::import: found body");
			rec_find_rss_outlines(urlcfg, node->children, "");
			error_message =
				urlcfg.write_config(); // Only an error if optional string has a value
		}
	}

	xmlFreeDoc(doc);

	return error_message;
}

} // namespace newsboat

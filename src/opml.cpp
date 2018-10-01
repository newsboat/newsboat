#include "opml.h"

#include <cassert>
#include <cstring>

namespace newsboat {

xmlDocPtr OPML::generate_opml(const FeedContainer& feedcontainer)
{
	xmlDocPtr root = xmlNewDoc((const xmlChar*)"1.0");
	xmlNodePtr Opmlnode =
		xmlNewDocNode(root, nullptr, (const xmlChar*)"opml", nullptr);
	xmlSetProp(Opmlnode, (const xmlChar*)"version", (const xmlChar*)"1.0");
	xmlDocSetRootElement(root, Opmlnode);

	xmlNodePtr head = xmlNewTextChild(
		Opmlnode, nullptr, (const xmlChar*)"head", nullptr);
	xmlNewTextChild(head,
		nullptr,
		(const xmlChar*)"title",
		(const xmlChar*)PROGRAM_NAME " - Exported Feeds");
	xmlNodePtr body = xmlNewTextChild(
		Opmlnode, nullptr, (const xmlChar*)"body", nullptr);

	for (const auto& feed : feedcontainer.feeds) {
		if (!Utils::is_special_url(feed->rssurl())) {
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
		UrlReader* urlcfg,
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
					"OPML::import: found RSS outline with "
					"url = "
					"%s",
					url);

				std::string nurl = std::string(url);

				// Liferea uses a pipe to signal feeds read from
				// the output of a program in its OPMLs. Convert
				// them to our syntax.
				if (*url == '|') {
					nurl = StrPrintf::fmt(
						"exec:%s", url + 1);
					LOG(Level::DEBUG,
						"OPML::import: liferea-style "
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
						"OPML::import: adding filter "
						"command %s to url %s",
						filtercmd,
						nurl);
					nurl.insert(0,
						StrPrintf::fmt("filter:%s:",
							filtercmd));
					xmlFree(filtercmd);
				}

				xmlFree(url);
				// Filters and scripts may have arguments, so,
				// quote them when needed.
				// TODO: get rid of xmlStrdup, it's useless
				url = (char*)xmlStrdup(
					(const xmlChar*)
						Utils::quote_if_necessary(nurl)
							.c_str());
				assert(url);

				bool found = false;

				LOG(Level::DEBUG,
					"OPML::import: size = %u",
					urlcfg->get_urls().size());
				// TODO: replace with algorithm::any or something
				if (urlcfg->get_urls().size() > 0) {
					for (const auto& u :
						urlcfg->get_urls()) {
						if (u == url) {
							found = true;
						}
					}
				}

				if (!found) {
					LOG(Level::DEBUG,
						"OPML::import: added url = %s",
						url);
					urlcfg->get_urls().push_back(
						std::string(url));
					if (tag.length() > 0) {
						LOG(Level::DEBUG,
							"OPML::import: "
							"appending "
							"tag %s to url %s",
							tag,
							url);
						urlcfg->get_tags(url).push_back(
							tag);
					}
				} else {
					LOG(Level::DEBUG,
						"OPML::import: url = %s is "
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

bool OPML::import(
		const std::string& filename,
		UrlReader* urlcfg)
{
	xmlDoc* doc = xmlReadFile(filename.c_str(), nullptr, 0);
	if (doc == nullptr) {
		return false;
	}

	xmlNode* root = xmlDocGetRootElement(doc);

	for (xmlNode* node = root->children; node != nullptr;
		node = node->next) {
		if (strcmp((const char*)node->name, "body") == 0) {
			LOG(Level::DEBUG, "OPML::import: found body");
			rec_find_rss_outlines(urlcfg, node->children, "");
			urlcfg->write_config();
		}
	}

	xmlFreeDoc(doc);

	return true;
}

} // namespace newsboat

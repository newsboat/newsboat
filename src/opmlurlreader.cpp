#include "opmlurlreader.h"

#include <cstring>

#include "logger.h"
#include "utils.h"

namespace newsboat {

OpmlUrlReader::OpmlUrlReader(ConfigContainer& c, const Filepath& url_file)
	: cfg(c), file(url_file)
{
}

std::optional<utils::ReadTextFileError> OpmlUrlReader::reload()
{
	urls.clear();
	tags.clear();

	std::vector<std::string> opml_urls =
		utils::tokenize_quoted(this->get_source(), " ");

	load_query_urls_from_file(file);

	for (const auto& url : opml_urls) {
		LOG(Level::DEBUG,
			"OpmlUrlReader::reload: downloading `%s'",
			url);
		std::string urlcontent = utils::retrieve_url(url, cfg);

		xmlDoc* doc =
			xmlParseMemory(urlcontent.c_str(), urlcontent.length());

		if (doc == nullptr) {
			LOG(Level::ERROR,
				"OpmlUrlReader::reload: parsing XML file `%s'"
				"failed", url);
			continue;
		}

		xmlNode* root = xmlDocGetRootElement(doc);

		if (root) {
			for (xmlNode* node = root->children; node != nullptr;
				node = node->next) {
				if (strcmp((const char*)node->name, "body") ==
					0) {
					LOG(Level::DEBUG,
						"OpmlUrlReader::reload: found "
						"body");
					rec_find_rss_outlines(
						node->children, "");
				}
			}
		}

		xmlFreeDoc(doc);
	}

	return {};
}

void OpmlUrlReader::handle_node(xmlNode* node, const std::string& tag)
{
	if (node) {
		char* rssurl =
			(char*)xmlGetProp(node, (const xmlChar*)"xmlUrl");
		if (rssurl && strlen(rssurl) > 0) {
			std::string theurl(rssurl);
			urls.push_back(theurl);
			if (tag.length() > 0) {
				std::vector<std::string> tmptags;
				tmptags.push_back(tag);
				tags[theurl] = tmptags;
			}
		}
		if (rssurl) {
			xmlFree(rssurl);
		}
	}
}

void OpmlUrlReader::rec_find_rss_outlines(xmlNode* node, std::string tag)
{
	while (node) {
		char* type = (char*)xmlGetProp(node, (const xmlChar*)"type");

		std::string newtag = tag;

		if (strcmp((const char*)node->name, "outline") == 0) {
			if (type && strcmp(type, "rss") == 0) {
				handle_node(node, tag);
				xmlFree(type);
			} else {
				char* text = (char*)xmlGetProp(
						node, (const xmlChar*)"title");
				if (text) {
					if (newtag.length() > 0) {
						newtag.append("/");
					}
					newtag.append(text);
					xmlFree(text);
				}
			}
		}
		rec_find_rss_outlines(node->children, newtag);
		node = node->next;
	}
}

std::string OpmlUrlReader::get_source() const
{
	return cfg.get_configvalue("opml-url");
}

}

#include "opmlurlreader.h"

#include <cstring>

#include "utils.h"

namespace newsboat {

opml_urlreader::opml_urlreader(configcontainer* c)
	: cfg(c)
{
}

void opml_urlreader::write_config()
{
	// do nothing.
}

void opml_urlreader::reload()
{
	urls.clear();
	tags.clear();
	alltags.clear();

	std::vector<std::string> urls =
		utils::tokenize_quoted(this->get_source(), " ");

	for (const auto& url : urls) {
		LOG(level::DEBUG,
			"opml_urlreader::reload: downloading `%s'",
			url);
		std::string urlcontent = utils::retrieve_url(url, cfg);

		xmlDoc* doc =
			xmlParseMemory(urlcontent.c_str(), urlcontent.length());

		if (doc == nullptr) {
			LOG(level::ERROR,
				"opml_urlreader::reload: parsing XML file `%s'"
				"failed", url);
			continue;
		}

		xmlNode* root = xmlDocGetRootElement(doc);

		if (root) {
			for (xmlNode* node = root->children; node != nullptr;
				node = node->next) {
				if (strcmp((const char*)node->name, "body") ==
					0) {
					LOG(level::DEBUG,
						"opml_urlreader::reload: found "
						"body");
					rec_find_rss_outlines(
						node->children, "");
				}
			}
		}

		xmlFreeDoc(doc);
	}
}

void opml_urlreader::handle_node(xmlNode* node, const std::string& tag)
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
				alltags.insert(tag);
			}
		}
		if (rssurl) {
			xmlFree(rssurl);
		}
	}
}

void opml_urlreader::rec_find_rss_outlines(xmlNode* node, std::string tag)
{
	while (node) {
		char* type = (char*)xmlGetProp(node, (const xmlChar*)"type");

		std::string newtag = tag;

		if (strcmp((const char*)node->name, "outline") == 0) {
			if (type && strcmp(type, "rss") == 0) {
				handle_node(node, tag);
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

std::string opml_urlreader::get_source()
{
	return cfg->get_configvalue("opml-url");
}

}

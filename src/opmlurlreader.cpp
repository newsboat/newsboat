#include "opmlurlreader.h"

#include <cstring>
#include <iostream>
#include <cassert>

#include "utils.h"

namespace newsboat {

OpmlUrlReader::OpmlUrlReader(ConfigContainer* c)
	: cfg(c)
{
}

xmlDocPtr OpmlUrlReader::generate(const FeedContainer& feedcontainer)
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

	for (const auto& feed : feedcontainer.feeds) {
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

void OpmlUrlReader::export_opml(FeedContainer& feedcontainer)
{
	xmlDocPtr root = generate(feedcontainer);

	xmlSaveCtxtPtr savectx = xmlSaveToFd(1, nullptr, 1);
	xmlSaveDoc(savectx, root);
	xmlSaveClose(savectx);

	xmlFreeDoc(root);
}

void  OpmlUrlReader::import(const std::string& filename)
{
	xmlDoc* doc = xmlReadFile(filename.c_str(), nullptr, 0);
	if (doc == nullptr) {
		std::cout << strprintf::fmt(
				     _("An error occurred while parsing %s."),
				     filename)
			  << std::endl;
		return;
	}

	xmlNode* root = xmlDocGetRootElement(doc);

	for (xmlNode* node = root->children; node != nullptr;
		node = node->next) {
		if (strcmp((const char*)node->name, "body") == 0) {
			LOG(Level::DEBUG, "OpmlUrlReader::import: found body");
			rec_find_rss_outlines(node->children, "", true);
		}
	}

	xmlFreeDoc(doc);

	std::cout << strprintf::fmt(
			_("Import of %s finished."), filename)
		<< std::endl;

	return;
}

void OpmlUrlReader::write_config()
{
	// do nothing.
}

void OpmlUrlReader::reload()
{
	urls.clear();
	tags.clear();
	alltags.clear();

	std::vector<std::string> urls =
		utils::tokenize_quoted(this->get_source(), " ");

	for (const auto& url : urls) {
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
						node->children, "", false);
				}
			}
		}

		xmlFreeDoc(doc);
	}
}

void OpmlUrlReader::handle_node(const std::string& rssurl, const std::string& tag)
{
	LOG(Level::DEBUG, "opml::import: added url = %s", rssurl.c_str());
	urls.push_back(rssurl);
	if (tag.length() > 0) {
		LOG(Level::DEBUG, "opml::import: " "appending " "tag %s to url %s",
				tag.c_str(),
				rssurl.c_str()
				);
		get_tags(rssurl).push_back(tag);
		alltags.insert(tag);
	}
}

void OpmlUrlReader::rec_find_rss_outlines( xmlNode* node, std::string tag, bool permissive)
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
					"opml::import: size = %u",
					get_urls().size());
				// TODO: replace with algorithm::any or something
				if (get_urls().size() > 0) {
					for (const auto& u :
						get_urls()) {
						if (u == url) {
							found = true;
						}
					}
				}

				if (!found) {
					std::string type;
					char* pretype = (char*)xmlGetProp(node, (const xmlChar*)"type");
					if(pretype) {
						type = std::string(pretype);
					}

					if (permissive || (type == "rss")) { 
						handle_node(nurl, tag);
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
		rec_find_rss_outlines(node->children, newtag, permissive);

		node = node->next;
	}
}

std::string OpmlUrlReader::get_source()
{
	return cfg->get_configvalue("opml-url");
}

}

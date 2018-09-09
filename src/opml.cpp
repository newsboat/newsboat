#include "opml.h"

namespace newsboat {

xmlDocPtr OPML::prepare_opml(const FeedContainer& feedcontainer)
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

} // namespace newsboat

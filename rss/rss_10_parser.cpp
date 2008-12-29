#include <rsspp_internal.h>

namespace rsspp {

void rss_10_parser::parse_feed(feed& f, xmlNode * rootNode) {
	if (!rootNode)
		throw exception(0, "rootNode is NULL");

	for (xmlNode * node = rootNode->children; node != NULL; node = node->next) {
		if (strcmp((const char *)node->name, "channel")==0) {
			for (xmlNode * cnode = node->children; cnode != NULL; cnode = cnode->next) {
				if (strcmp((const char *)cnode->name, "title")==0) {
					f.title = get_content(cnode);
					f.title_type = "text";
				} else if (strcmp((const char *)cnode->name, "link")==0) {
					f.link = get_content(cnode);
				} else if (strcmp((const char *)cnode->name, "description")==0) {
					f.description = get_content(cnode);
				} // TODO: also use the dc namespace
			}
		} else if (strcmp((const char *)node->name, "item")==0) {
			item it;
			xmlChar * about = xmlGetNsProp(node, (xmlChar *)"about", (xmlChar *)RDF_URI);
			if (about) {
				it.guid = (const char *)about;
				xmlFree(about);
			}
			for (xmlNode * itnode = node->children; itnode != NULL; itnode = itnode->next) {
				if (strcmp((const char *)itnode->name, "title")==0) {
					it.title = get_content(itnode);
					it.title_type = "text";
				} else if (strcmp((const char *)itnode->name, "link")==0) {
					it.link = get_content(itnode);
				} else if (strcmp((const char *)itnode->name, "description")==0) {
					it.description = get_content(itnode);
				} // TODO: also use the dc namespace
			}
			f.items.push_back(it);
		}
	}
}

}

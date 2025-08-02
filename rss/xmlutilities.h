#ifndef NEWSBOAT_RSSPP_XMLUTILITIES_H_
#define NEWSBOAT_RSSPP_XMLUTILITIES_H_

#include <libxml/tree.h>
#include <string>

namespace rsspp {

std::string get_content(const xmlNode* node);
std::string get_xml_content(xmlNode* node, xmlDocPtr doc);
void cleanup_namespaces(xmlNodePtr node);
std::string get_prop(xmlNode* node, const std::string& prop,
	const std::string& ns = "");
bool has_namespace(const xmlNode* node, const char* ns_uri = nullptr);
bool node_is(xmlNode* node, const char* name, const char* ns_uri = nullptr);

} // namespace rsspp

#endif // NEWSBOAT_RSSPP_XMLUTILITIES_H_

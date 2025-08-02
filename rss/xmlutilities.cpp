#include "xmlutilities.h"

#include <cstring>

namespace rsspp {

std::string get_content(const xmlNode* node)
{
	std::string retval;
	if (node) {
		xmlChar* content = xmlNodeGetContent(node);
		if (content) {
			retval = reinterpret_cast<const char*>(content);
			xmlFree(content);
		}
	}
	return retval;
}

std::string get_xml_content(xmlNode* node, xmlDocPtr doc)
{
	xmlBufferPtr buf = xmlBufferCreate();
	std::string result;

	cleanup_namespaces(node);

	if (node->children) {
		for (xmlNodePtr ptr = node->children; ptr != nullptr;
			ptr = ptr->next) {
			if (xmlNodeDump(buf, doc, ptr, 0, 0) >= 0) {
				result.append(
					reinterpret_cast<const char*>(xmlBufferContent(buf)));
				xmlBufferEmpty(buf);
			} else {
				result.append(get_content(ptr));
			}
		}
	} else {
		result = get_content(node); // fallback
	}
	xmlBufferFree(buf);

	return result;
}

void cleanup_namespaces(xmlNodePtr node)
{
	node->ns = nullptr;
	for (auto ptr = node->children; ptr != nullptr; ptr = ptr->next) {
		cleanup_namespaces(ptr);
	}
}

std::string get_prop(xmlNode* node, const std::string& prop,
	const std::string& ns)
{
	std::string retval;
	if (node) {
		xmlChar* value = nullptr;
		if (ns.empty()) {
			value = xmlGetProp(node,
					reinterpret_cast<const xmlChar*>(prop.c_str()));
		} else {
			value = xmlGetNsProp(node,
					reinterpret_cast<const xmlChar*>(prop.c_str()),
					reinterpret_cast<const xmlChar*>(ns.c_str()));
		}
		if (value) {
			retval = reinterpret_cast<const char*>(value);
			xmlFree(value);
		}
	}
	return retval;
}

bool has_namespace(const xmlNode* node, const char* ns_uri)
{
	if (!ns_uri && !node->ns) {
		return true;
	}
	if (ns_uri && node->ns && node->ns->href &&
		strcmp(reinterpret_cast<const char*>(node->ns->href), ns_uri) == 0) {
		return true;
	}
	return false;
}

bool node_is(xmlNode* node, const char* name, const char* ns_uri)
{
	if (!node || !name || !node->name) {
		return false;
	}

	if (strcmp(reinterpret_cast<const char*>(node->name), name) == 0) {
		return has_namespace(node, ns_uri);
	}
	return false;
}

} // namespace rsspp

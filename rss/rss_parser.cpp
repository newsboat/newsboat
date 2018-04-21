#include "rsspp_internal.h"

#include <cstring>
#include <libxml/tree.h>

namespace rsspp {

std::string rss_parser::get_content(xmlNode* node)
{
	std::string retval;
	if (node) {
		xmlChar* content = xmlNodeGetContent(node);
		if (content) {
			retval = (const char*)content;
			xmlFree(content);
		}
	}
	return retval;
}

void rss_parser::cleanup_namespaces(xmlNodePtr node)
{
	node->ns = nullptr;
	for (auto ptr = node->children; ptr != nullptr; ptr = ptr->next) {
		cleanup_namespaces(ptr);
	}
}

std::string rss_parser::get_xml_content(xmlNode* node)
{
	xmlBufferPtr buf = xmlBufferCreate();
	std::string result;

	cleanup_namespaces(node);

	if (node->children) {
		for (xmlNodePtr ptr = node->children; ptr != nullptr;
		     ptr = ptr->next) {
			if (xmlNodeDump(buf, doc, ptr, 0, 0) >= 0) {
				result.append(
					(const char*)xmlBufferContent(buf));
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

std::string rss_parser::get_prop(
	xmlNode* node,
	const std::string& prop,
	const std::string& ns)
{
	std::string retval;
	if (node) {
		xmlChar* value = nullptr;
		if (ns.empty()) {
			value = xmlGetProp(
				node,
				reinterpret_cast<const xmlChar*>(prop.c_str()));
		} else {
			value = xmlGetNsProp(
				node,
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

std::string rss_parser::w3cdtf_to_rfc822(const std::string& w3cdtf)
{
	return __w3cdtf_to_rfc822(w3cdtf);
}

std::string rss_parser::__w3cdtf_to_rfc822(const std::string& w3cdtf)
{
	if (w3cdtf.empty()) {
		return "";
	}

	struct tm stm;
	memset(&stm, 0, sizeof(stm));
	stm.tm_mday = 1;

	char* ptr = strptime(w3cdtf.c_str(), "%Y", &stm);

	if (ptr != nullptr) {
		ptr = strptime(ptr, "-%m", &stm);
	} else {
		return "";
	}

	if (ptr != nullptr) {
		ptr = strptime(ptr, "-%d", &stm);
	}
	if (ptr != nullptr) {
		ptr = strptime(ptr, "T%H", &stm);
	}
	if (ptr != nullptr) {
		ptr = strptime(ptr, ":%M", &stm);
	}
	if (ptr != nullptr) {
		ptr = strptime(ptr, ":%S", &stm);
	}

	int offs = 0;
	if (ptr != nullptr) {
		if (ptr[0] == '+' || ptr[0] == '-') {
			unsigned int hour, min;
			if (sscanf(ptr + 1, "%02u:%02u", &hour, &min) == 2) {
				offs = 60 * 60 * hour + 60 * min;
				if (ptr[0] == '+')
					offs = -offs;
				stm.tm_gmtoff = offs;
			}
		} else if (ptr[0] == 'Z') {
			stm.tm_gmtoff = 0;
		}
	}

	// tm_isdst will force mktime to consider DST, like localtime(), but
	// then the offset will be zeroed out, since that was manually added
	// https://github.com/akrennmair/newsbeuter/issues/369
	stm.tm_isdst = -1;
	time_t gmttime = mktime(&stm) + offs;
	char datebuf[256];
	strftime(
		datebuf,
		sizeof(datebuf),
		"%a, %d %b %Y %H:%M:%S +0000",
		localtime(&gmttime));
	return datebuf;
}

bool rss_parser::node_is(xmlNode* node, const char* name, const char* ns_uri)
{
	if (!node || !name || !node->name)
		return false;

	if (strcmp((const char*)node->name, name) == 0) {
		if (!ns_uri && !node->ns)
			return true;
		if (ns_uri && node->ns && node->ns->href
		    && strcmp((const char*)node->ns->href, ns_uri) == 0)
			return true;
	}
	return false;
}

} // namespace rsspp

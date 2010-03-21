/* rsspp - Copyright (C) 2008-2010 Andreas Krennmair <ak@newsbeuter.org>
 * Licensed under the MIT/X Consortium License. See file LICENSE
 * for more information.
 */

#include <rsspp_internal.h>
#include <cstring>
#include <libxml/tree.h>

namespace rsspp {

std::string rss_parser::get_content(xmlNode * node) {
	std::string retval;
	if (node) {
		xmlChar * content = xmlNodeGetContent(node);
		if (content) {
			retval = (const char *)content;
			xmlFree(content);
		}
	}
	return retval;
}

std::string rss_parser::get_xml_content(xmlNode * node) {
	xmlBufferPtr buf = xmlBufferCreate();
	std::string result;

	if (node->children) {
		for (xmlNodePtr ptr = node->children; ptr != NULL; ptr = ptr->next) {
			if (xmlNodeDump(buf, doc, ptr, 0, 0) >= 0) {
				result.append((const char *)xmlBufferContent(buf));
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

std::string rss_parser::get_prop(xmlNode * node, const char * prop, const char * ns) {
	std::string retval;
	if (node) {
		xmlChar * value;
		if (ns)
			value = xmlGetProp(node, (xmlChar *)prop);
		else
			value = xmlGetNsProp(node, (xmlChar *)prop, (xmlChar *)ns);
		if (value) {
			retval = (const char*)value;
			xmlFree(value);
		}
	}
	return retval;
}

std::string rss_parser::w3cdtf_to_rfc822(const std::string& w3cdtf) {
	return __w3cdtf_to_rfc822(w3cdtf);
}

std::string rss_parser::__w3cdtf_to_rfc822(const std::string& w3cdtf) {
	struct tm stm;
	memset(&stm, 0, sizeof (stm));
	stm.tm_mday = 1;

	//ptr = strptime(w3cdtf.c_str(), "%Y-%m-%dT%H:%M:%S", &stm);
	char * ptr = strptime(w3cdtf.c_str(), "%Y", &stm);

	if (ptr != NULL) {
		ptr = strptime(ptr, "-%m", &stm);
	} else {
		return "";
	}

	if (ptr != NULL) {
		ptr = strptime(ptr, "-%d", &stm);
	}
	if (ptr != NULL) {
		ptr = strptime(ptr, "T%H", &stm);
	}
	if (ptr != NULL) {
		ptr = strptime(ptr, ":%M", &stm);
	}
	if (ptr != NULL) {
		ptr = strptime(ptr, ":%S", &stm);
	}

	int offs = 0;
	if (ptr != NULL) {
		if (ptr[0] == '+' || ptr[0] == '-') {
			unsigned int hour, min;
			if (sscanf(ptr+1,"%02u:%02u", &hour, &min)==2) {
				offs = 60*60*hour + 60*min;
				if (ptr[0] == '+')
					offs = -offs;
				stm.tm_gmtoff = offs;
			}
		} else if (ptr[0] == 'Z') {
			stm.tm_gmtoff = 0;
		}
	}

	time_t t = mktime(&stm);
	time_t x = time(NULL);
	t += localtime(&x)->tm_gmtoff + offs;
	char datebuf[256];
	strftime (datebuf, sizeof (datebuf), "%a, %d %b %Y %H:%M:%S %z", gmtime(&t));
	return datebuf;
}

bool rss_parser::node_is(xmlNode * node, const char * name, const char * ns_uri) {
	if (!node || !name)
		return false;

	if (strcmp((const char *)node->name, name)==0) {
		if (!ns_uri && !node->ns)
			return true;
		if (ns_uri && node->ns && node->ns->href && strcmp((const char *)node->ns->href, ns_uri)==0)
			return true;
	}
	return false;
}

}

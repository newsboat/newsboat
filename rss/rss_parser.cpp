/* rsspp - Copyright (C) 2008-2009 Andreas Krennmair <ak@newsbeuter.org>
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

	int rc;
	/* format: 2007-01-17T07:45:50Z */
	char sign;
	int hour;
	int min;
	int offset = 0;
	if ((rc = sscanf(w3cdtf.c_str(), "%04d-%02d-%02dT%02d:%02d:%02d%c%02d:%02d", 
		&stm.tm_year, &stm.tm_mon, &stm.tm_mday, &stm.tm_hour, &stm.tm_min, &stm.tm_sec, &sign, &hour, &min)) >= 1) {
		char datebuf[256];
		stm.tm_year -= 1900;
		stm.tm_mon -= 1;
		switch (rc) {
			case 1:
				stm.tm_mon = 0;
			case 2:
				stm.tm_mday = 1;
			case 3:
				stm.tm_hour = 0;
			case 4:
				stm.tm_min = 0;
			case 5:
				stm.tm_sec = 0;
				break;
			case 7: {
				offset = hour*3600 + min*60;
				if (sign == '-') {
					offset = -offset;
				}
				// stm.tm_gmtoff = offset;
			}

		}

		time_t x = time(NULL);
		struct tm * tmx = localtime(&x);
		time_t t = mktime(&stm);

		strftime (datebuf, sizeof (datebuf), "%a, %d %b %Y %H:%M:%S %z", gmtime(&t));
		return datebuf;
	}

	return "";
}

bool rss_parser::node_is(xmlNode * node, const char * name, const char * ns_uri) {
	if (!node || !name)
		return false;

	if (strcmp((const char *)node->name, name)==0) {
		if (!ns_uri)
			return true;
		if (node->ns && node->ns->href && strcmp((const char *)node->ns->href, ns_uri)==0)
			return true;
	}
	return false;
}

}

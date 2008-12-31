/* rsspp - Copyright (C) 2008 Andreas Krennmair <ak@newsbeuter.org>
 * Licensed under the MIT/X Consortium License. See file LICENSE
 * for more information.
 */

#include <rsspp_internal.h>
#include <cstring>

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

std::string rss_parser::w3cdtf_to_rfc822(const std::string& w3cdtf) {
	struct tm stm;
	memset(&stm, 0, sizeof (stm));

	/* format: 2007-01-17T07:45:50Z */
	if (sscanf(w3cdtf.c_str(), "%04d-%02d-%02dT%02d:%02d:%02d", 
		&stm.tm_year, &stm.tm_mon, &stm.tm_mday, &stm.tm_hour, &stm.tm_min, &stm.tm_sec) >= 3) {
		char datebuf[256];
		stm.tm_year -= 1900;
		stm.tm_mon -= 1;

		time_t t = mktime(&stm);

		strftime (datebuf, sizeof (datebuf), "%a, %d %b %Y %H:%M:%S %z", gmtime(&t));
		return datebuf;
	}

	return "";
}

}

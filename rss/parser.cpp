/* rsspp - Copyright (C) 2008 Andreas Krennmair <ak@newsbeuter.org>
 * Licensed under the MIT/X Consortium License. See file LICENSE
 * for more information.
 */

#include <rsspp.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

namespace rsspp {

parser::parser(unsigned int timeout, const char * user_agent, const char * proxy, const char * proxy_auth) 
	: to(timeout), ua(user_agent), prx(proxy), prxauth(proxy_auth) {
}

parser::~parser() {
	xmlCleanupParser();
}

feed parser::parse_url(const std::string& url) {
	throw exception(0, "unimplemented");
}

feed parser::parse_buffer(const char * buffer, size_t size) {
	throw exception(0, "unimplemented");
}

feed parser::parse_file(const std::string& filename) {
	throw exception(0, "unimplemented");
}

void parser::global_init() {
	LIBXML_TEST_VERSION
}

void parser::global_cleanup() {
	xmlCleanupParser();
}

}

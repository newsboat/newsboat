/* rsspp - Copyright (C) 2008-2009 Andreas Krennmair <ak@newsbeuter.org>
 * Licensed under the MIT/X Consortium License. See file LICENSE
 * for more information.
 */

#include <rsspp.h>
#include <config.h>

namespace rsspp {

exception::exception(unsigned int error_code, const std::string& errmsg) : ecode(error_code), emsg(errmsg) { 
	buf = std::string(_("Error: "));
	buf.append(emsg);
}

exception::~exception() throw() {

}

const char* exception::what() const throw() {
	return buf.c_str();
}

}

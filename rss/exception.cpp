/* rsspp - Copyright (C) 2008-2010 Andreas Krennmair <ak@newsbeuter.org>
 * Licensed under the MIT/X Consortium License. See file LICENSE
 * for more information.
 */

#include <rsspp.h>
#include <config.h>

namespace rsspp {

exception::exception(const std::string& errmsg) : emsg(errmsg) { 
}

exception::~exception() throw() {

}

const char* exception::what() const throw() {
	return emsg.c_str();
}

}

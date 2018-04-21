#include "rsspp.h"

#include "config.h"

namespace rsspp {

exception::exception(const std::string& errmsg) : emsg(errmsg) {
}

exception::~exception() throw() {

}

const char* exception::what() const throw() {
	return emsg.c_str();
}

}

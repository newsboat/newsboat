#include "exception.h"

namespace rsspp {

Exception::Exception(const std::string& errmsg)
	: emsg(errmsg)
{
}

Exception::~Exception() throw() {}

const char* Exception::what() const throw()
{
	return emsg.c_str();
}

} // namespace rsspp

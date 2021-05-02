#include "exception.h"

namespace rsspp {

Exception::Exception(const std::string& errmsg)
	: emsg(newsboat::Utf8String::from_utf8(errmsg))
{
}

Exception::~Exception() throw() {}

const char* Exception::what() const throw()
{
	return emsg.to_utf8().c_str();
}

} // namespace rsspp

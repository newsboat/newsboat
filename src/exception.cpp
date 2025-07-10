#include "exception.h"

#include <cstring>

namespace Newsboat {

Exception::Exception(unsigned int error_code)
	: ecode(error_code)
{
}

Exception::~Exception() throw() {}

const char* Exception::what() const throw()
{
	return ::strerror(ecode);
}

} // namespace Newsboat

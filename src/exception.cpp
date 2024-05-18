#include "exception.h"

#include <cstring>

namespace newsboat {

Exception::Exception(unsigned int error_code)
	: ecode(error_code)
{
}

Exception::~Exception() throw() {}

const char* Exception::what() const throw()
{
	return ::strerror(ecode);
}

} // namespace newsboat

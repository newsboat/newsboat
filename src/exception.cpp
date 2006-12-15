#include <exception.h>
#include <cerrno>
#include <cstring>

using namespace noos;

exception::exception(unsigned int error_code) : ecode(error_code) { }

exception::~exception() throw() { }

const char * exception::what() const throw() {
	return std::strerror(ecode);
}

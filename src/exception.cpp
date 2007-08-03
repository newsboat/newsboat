#include <exception.h>
#include <exceptions.h>
#include <config.h>
#include <cerrno>
#include <cstring>

using namespace newsbeuter;

exception::exception(unsigned int error_code) : ecode(error_code) { }

exception::~exception() throw() { }

const char * exception::what() const throw() {
	return std::strerror(ecode);
}


const char * matcherexception::what() const throw() {
	static char errmsgbuf[2048];
	switch (type) {
		case ATTRIB_UNAVAIL:
			snprintf(errmsgbuf, sizeof(errmsgbuf), _("attribute `%s' is not available."), addinfo.c_str());
			break;
		default:
			strcpy(errmsgbuf,"");
	}
	return errmsgbuf;
}

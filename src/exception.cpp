#include <exception.h>
#include <exceptions.h>
#include <config.h>
#include <cerrno>
#include <cstring>
#include <utils.h>

using namespace newsbeuter;

exception::exception(unsigned int error_code) : ecode(error_code) { }

exception::~exception() throw() { }

const char * exception::what() const throw() {
	return ::strerror(ecode);
}


const char * matcherexception::what() const throw() {
	static std::string errmsg;
	switch (type) {
		case ATTRIB_UNAVAIL:
			errmsg = utils::strprintf(_("attribute `%s' is not available."), addinfo.c_str());
			break;
		default:
			errmsg = "";
	}
	return errmsg.c_str();
}

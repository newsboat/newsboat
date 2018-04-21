#include "exception.h"

#include <cerrno>
#include <cstring>

#include "exceptions.h"
#include "config.h"
#include "utils.h"
#include "strprintf.h"

namespace newsboat {

exception::exception(unsigned int error_code) : ecode(error_code) { }

exception::~exception() throw() { }

const char * exception::what() const throw() {
	return ::strerror(ecode);
}


const char * matcherexception::what() const throw() {
	static std::string errmsg;
	switch (type_) {
	case type::ATTRIB_UNAVAIL:
		errmsg = strprintf::fmt(_("attribute `%s' is not available."), addinfo);
		break;
	case type::INVALID_REGEX:
		errmsg = strprintf::fmt(_("regular expression '%s' is invalid: %s"), addinfo, addinfo2);
		break;
	}
	return errmsg.c_str();
}

confighandlerexception::confighandlerexception(action_handler_status e) {
	msg = get_errmsg(e);
}

const char * confighandlerexception::get_errmsg(action_handler_status status) {
	switch (status) {
	case action_handler_status::INVALID_PARAMS:
		return _("invalid parameters.");
	case action_handler_status::TOO_FEW_PARAMS:
		return _("too few parameters.");
	case action_handler_status::INVALID_COMMAND:
		return _("unknown command (bug).");
	case action_handler_status::FILENOTFOUND:
		return _("file couldn't be opened.");
	default:
		return _("unknown error (bug).");
	}
}

}

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
		case INVALID_REGEX:
			errmsg = utils::strprintf(_("regular expression '%s' is invalid: %s"), addinfo.c_str(), addinfo2.c_str());
			break;
		default:
			errmsg = "";
	}
	return errmsg.c_str();
}

confighandlerexception::confighandlerexception(action_handler_status e) {
	msg = get_errmsg(e);
}

const char * confighandlerexception::get_errmsg(action_handler_status status) {
	switch (status) {
		case AHS_INVALID_PARAMS:
			return _("invalid parameters.");
		case AHS_TOO_FEW_PARAMS:
			return _("too few parameters.");
		case AHS_INVALID_COMMAND:
			return _("unknown command (bug).");
		case AHS_FILENOTFOUND:
			return _("file couldn't be opened.");
		default:
			return _("unknown error (bug).");
	}
}

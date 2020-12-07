#include "confighandlerexception.h"

#include "config.h"
#include "configparser.h"

namespace newsboat {

ConfigHandlerException::ConfigHandlerException(ActionHandlerStatus e)
{
	msg = get_errmsg(e);
}

const char* ConfigHandlerException::get_errmsg(ActionHandlerStatus status)
{
	switch (status) {
	case ActionHandlerStatus::INVALID_PARAMS:
		return _("invalid parameters.");
	case ActionHandlerStatus::TOO_FEW_PARAMS:
		return _("too few parameters.");
	case ActionHandlerStatus::INVALID_COMMAND:
		return _("unknown command (bug).");
	case ActionHandlerStatus::FILENOTFOUND:
		return _("file couldn't be opened.");
	default:
		return _("unknown error (bug).");
	}
}

} // namespace newsboat

#include "confighandlerexception.h"

#include <cassert>

#include "config.h"
#include "configparser.h"

namespace Newsboat {

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
	case ActionHandlerStatus::TOO_MANY_PARAMS:
		return _("too many parameters.");
	case ActionHandlerStatus::INVALID_COMMAND:
		return _("unknown command (bug).");
	case ActionHandlerStatus::FILENOTFOUND:
		return _("file couldn't be opened.");
	}

	assert(0 && "unreachable, because the switch() above handles everything");
}

} // namespace Newsboat

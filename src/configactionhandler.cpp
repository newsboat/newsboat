#include "configactionhandler.h"

#include "utils.h"

namespace newsboat {

void ConfigActionHandler::handle_action(std::string_view action,
	std::string_view params)
{
	const std::vector<std::string> tokens = utils::tokenize_quoted(params);
	handle_action(action, tokens);
}

void ConfigActionHandler::handle_action(std::string_view,
	const std::vector<std::string>&)
{
}

} // namespace newsboat

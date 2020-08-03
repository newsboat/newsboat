#include "configactionhandler.h"

#include "utils.h"

namespace newsboat {

void ConfigActionHandler::handle_action(const std::string& action,
	const std::string& params)
{
	std::vector<std::string> tokens = utils::tokenize_quoted(params);
	handle_action(action, tokens);
}

void ConfigActionHandler::handle_action(const std::string&,
	const std::vector<std::string>&)
{
}

} // namespace newsboat

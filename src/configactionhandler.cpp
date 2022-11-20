#include "configactionhandler.h"

#include "utils.h"

namespace newsboat {

void ConfigActionHandler::handle_action(const Utf8String& action,
	const Utf8String& params)
{
	const auto raw_tokens = utils::tokenize_quoted(params);
	std::vector<Utf8String> tokens;
	for (const auto& raw : raw_tokens) {
		tokens.push_back(raw);
	}
	handle_action(action, tokens);
}

void ConfigActionHandler::handle_action(const Utf8String&,
	const std::vector<Utf8String>&)
{
}

} // namespace newsboat

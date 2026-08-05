#include "colormanager.h"

#include "confighandlerexception.h"
#include "configparser.h"
#include "logger.h"

#include "libnewsboat-ffi/src/colormanager.rs.h"

using namespace podboat;

namespace newsboat {

ColorManager::ColorManager()
	: rs_object(colormanager::bridged::create())
{
}

void ColorManager::register_commands(ConfigParser& cfgparser)
{
	cfgparser.register_handler("color", *this);
}

void ColorManager::handle_action(std::string_view action,
	const std::vector<std::string>& params)
{
	LOG(Level::DEBUG,
		"ColorManager::handle_action(%s,...) was called",
		action);

	std::vector<rust::Str> rust_params;
	for (const auto& param : params) {
		rust_params.push_back(param);
	}
	auto result = colormanager::bridged::handle_action(*rs_object, rust::Str(action.data(),
				action.size()), rust::Slice<const rust::Str>(rust_params.data(), rust_params.size()));

	switch (result->status) {
	case colormanager::bridged::Status::Ok:
		break;
	case colormanager::bridged::Status::InvalidCommand:
		throw ConfigHandlerException(ActionHandlerStatus::INVALID_COMMAND);
	case colormanager::bridged::Status::TooFewParameters:
		throw ConfigHandlerException(ActionHandlerStatus::TOO_FEW_PARAMS);
	case colormanager::bridged::Status::CustomErrorMessage:
		throw ConfigHandlerException(std::string(result->message));
	}
}

void ColorManager::dump_config(std::vector<std::string>& config_output) const
{
	const auto lines = colormanager::bridged::dump_config(*rs_object);
	for (const auto& line : lines) {
		config_output.push_back(std::string(line));
	}
}

std::map<std::string, std::string> ColorManager::get_stfl_styles() const
{
	const auto rust_styles = colormanager::bridged::get_stfl_styles(*rs_object);
	std::map<std::string, std::string> styles;
	for (const auto& [key, value] : rust_styles) {
		styles.insert_or_assign(std::string(key), std::string(value));
	}
	return styles;
}

} // namespace newsboat

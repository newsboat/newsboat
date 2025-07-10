#include "colormanager.h"

#include "config.h"
#include "confighandlerexception.h"
#include "configparser.h"
#include "logger.h"
#include "strprintf.h"
#include "utils.h"

using namespace Podboat;

namespace {
const std::map<std::string, Newsboat::TextStyle> default_styles {
	{ "listnormal",           {"default", "default", {}} },
	{ "listfocus",            {"yellow",  "blue",    {"bold"}} },
	{ "listnormal_unread",    {"default", "default", {"bold"}} },
	{ "listfocus_unread",     {"yellow",  "blue",    {"bold"}} },
	{ "info",                 {"yellow",  "blue",    {"bold"}} },
	{ "background",           {"default", "default", {}} },
	{ "article",              {"default", "default", {}} },
	{ "end-of-text-marker",   {"blue",    "default", {"bold"}} },
	{ "title",                {"yellow",  "blue",    {"bold"}} },
	{ "hint-key",             {"yellow",  "blue",    {"bold"}} },
	{ "hint-keys-delimiter",  {"white",   "blue",    {}} },
	{ "hint-separator",       {"white",   "blue",    {"bold"}} },
	{ "hint-description",     {"white",   "blue",    {}} },
};

const std::vector<std::string> supported_elements({
	"listnormal",
	"listfocus",
	"listnormal_unread",
	"listfocus_unread",
	"info",
	"background",
	"article",
	"end-of-text-marker",
	"title",
	"hint-key",
	"hint-keys-delimiter",
	"hint-separator",
	"hint-description",
});

const std::map<std::string, std::string> element_fallbacks {
	{ "title", "info" },
	{ "hint-key", "info" },
	{ "hint-keys-delimiter", "info" },
	{ "hint-separator", "info" },
	{ "hint-description", "info" },
};
}

namespace Newsboat {

void ColorManager::register_commands(ConfigParser& cfgparser)
{
	cfgparser.register_handler("color", *this);
}

void ColorManager::handle_action(const std::string& action,
	const std::vector<std::string>& params)
{
	LOG(Level::DEBUG,
		"ColorManager::handle_action(%s,...) was called",
		action);
	if (action == "color") {
		if (params.size() < 3) {
			throw ConfigHandlerException(ActionHandlerStatus::TOO_FEW_PARAMS);
		}

		/*
		 * the command syntax is:
		 * color <element> <fgcolor> <bgcolor> [<attribute> ...]
		 */
		const std::string element = params[0];
		const std::string fgcolor = params[1];
		const std::string bgcolor = params[2];

		if (!utils::is_valid_color(fgcolor)) {
			throw ConfigHandlerException(strprintf::fmt(
					_("`%s' is not a valid color"), fgcolor));
		}
		if (!utils::is_valid_color(bgcolor)) {
			throw ConfigHandlerException(strprintf::fmt(
					_("`%s' is not a valid color"), bgcolor));
		}

		const std::vector<std::string> attribs(
			std::next(params.cbegin(), 3),
			params.cend());
		for (const auto& attr : attribs) {
			if (!utils::is_valid_attribute(attr)) {
				throw ConfigHandlerException(strprintf::fmt(
						_("`%s' is not a valid attribute"),
						attr));
			}
		}

		/* we only allow certain elements to be configured, also to
		 * indicate the user possible mis-spellings */
		const auto element_is_supported = std::find(supported_elements.cbegin(),
				supported_elements.cend(), element) != supported_elements.cend();

		if (element_is_supported) {
			element_styles[element] = {fgcolor, bgcolor, attribs};
		} else {
			throw ConfigHandlerException(strprintf::fmt(
					_("`%s' is not a valid configuration element"),
					element));
		}
	} else {
		throw ConfigHandlerException(ActionHandlerStatus::INVALID_COMMAND);
	}
}

void ColorManager::dump_config(std::vector<std::string>& config_output) const
{
	for (const auto& element_style : element_styles) {
		const std::string& element = element_style.first;
		const TextStyle& style = element_style.second;
		std::string configline = strprintf::fmt("color %s %s %s",
				element,
				style.fg_color,
				style.bg_color);
		for (const auto& attrib : style.attributes) {
			configline.append(" ");
			configline.append(attrib);
		}
		config_output.push_back(configline);
	}
}

std::string format_style(const TextStyle& style)
{
	std::string result;

	if (style.fg_color != "default") {
		result.append("fg=");
		result.append(style.fg_color);
	}
	if (style.bg_color != "default") {
		if (!result.empty()) {
			result.append(",");
		}
		result.append("bg=");
		result.append(style.bg_color);
	}
	for (const auto& attr : style.attributes) {
		if (!result.empty()) {
			result.append(",");
		}
		result.append("attr=");
		result.append(attr);
	}

	return result;
}

void ColorManager::emit_fallback_from_to(const std::string& from_element,
	const std::string& to_element,
	const std::function<void(const std::string&, const std::string&)>& stfl_value_setter) const
{
	const auto from_style = element_styles.find(from_element);
	const auto to_style = element_styles.find(to_element);
	if (from_style == element_styles.cend() && to_style != element_styles.cend()) {
		const auto style = format_style(to_style->second);
		LOG(Level::DEBUG, "ColorManager::apply_colors: %s inherited from %s %s\n", from_element,
			to_element, style);
		stfl_value_setter(from_element, style);
	}
}

void ColorManager::apply_colors(
	std::function<void(const std::string&, const std::string&)> stfl_value_setter)
const
{
	for (const std::string& element : supported_elements) {
		const TextStyle* style = nullptr;
		const auto configured_style = element_styles.find(element);
		if (configured_style != element_styles.end()) {
			style = &configured_style->second;
		}

		if (style == nullptr) {
			const auto fallback = element_fallbacks.find(element);
			if (fallback != element_fallbacks.end()) {
				const auto fallback_style = element_styles.find(fallback->second);
				if (fallback_style != element_styles.end()) {
					style = &fallback_style->second;
				}
			}
		}

		if (style == nullptr) {
			style = &default_styles.at(element);
		}

		const auto colorattr = format_style(*style);

		LOG(Level::DEBUG,
			"ColorManager::apply_colors: %s %s\n",
			element,
			colorattr);

		stfl_value_setter(element, colorattr);

		if (element == "article") {
			std::string bold = colorattr;
			std::string underline = colorattr;
			if (!bold.empty()) {
				bold.append(",");
			}
			if (!underline.empty()) {
				underline.append(",");
			}
			bold.append("attr=bold");
			underline.append("attr=underline");
			// STFL will just ignore those in forms which don't have the
			// `color_bold` and `color_underline` variables.
			LOG(Level::DEBUG, "ColorManager::apply_colors: color_bold %s\n", bold);
			stfl_value_setter("color_bold", bold);
			LOG(Level::DEBUG, "ColorManager::apply_colors: color_underline %s\n", underline);
			stfl_value_setter("color_underline", underline);
		}
	}
}

} // namespace Newsboat

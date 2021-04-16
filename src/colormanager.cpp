#include "colormanager.h"

#include "config.h"
#include "confighandlerexception.h"
#include "feedlistformaction.h"
#include "filebrowserformaction.h"
#include "helpformaction.h"
#include "itemlistformaction.h"
#include "itemviewformaction.h"
#include "logger.h"
#include "matcherexception.h"
#include "pbview.h"
#include "selectformaction.h"
#include "strprintf.h"
#include "urlviewformaction.h"
#include "utils.h"

using namespace podboat;

namespace newsboat {

ColorManager::ColorManager()
{
}

ColorManager::~ColorManager() {}

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
		const auto element = Utf8String::from_utf8(params[0]);
		const auto fgcolor = Utf8String::from_utf8(params[1]);
		const auto bgcolor = Utf8String::from_utf8(params[2]);

		if (!utils::is_valid_color(fgcolor.to_utf8())) {
			throw ConfigHandlerException(strprintf::fmt(
					_("`%s' is not a valid color"), fgcolor));
		}
		if (!utils::is_valid_color(bgcolor.to_utf8())) {
			throw ConfigHandlerException(strprintf::fmt(
					_("`%s' is not a valid color"), bgcolor));
		}

		std::vector<Utf8String> attribs;
		for (unsigned int i = 3; i < params.size(); ++i) {
			if (!utils::is_valid_attribute(params[i])) {
				throw ConfigHandlerException(strprintf::fmt(
						_("`%s' is not a valid attribute"),
						params[i]));
			}
			attribs.push_back(Utf8String::from_utf8(params[i]));
		}

		/* we only allow certain elements to be configured, also to
		 * indicate the user possible mis-spellings */
		if (element == "listnormal" || element == "listfocus" ||
			element == "listnormal_unread" ||
			element == "listfocus_unread" || element == "info" ||
			element == "background" || element == "article" ||
			element == "end-of-text-marker") {
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
		const auto& element = element_style.first;
		const auto& style = element_style.second;
		auto configline = Utf8String::from_utf8(strprintf::fmt("color %s %s %s",
					element,
					style.fg_color,
					style.bg_color));
		for (const auto& attrib : style.attributes) {
			configline.append(" ");
			configline.append(attrib);
		}
		config_output.push_back(configline.to_utf8());
	}
}

void ColorManager::apply_colors(
	std::function<void(const std::string&, const std::string&)> stfl_value_setter)
const
{
	for (const auto& element_style : element_styles) {
		const auto& element = element_style.first;
		const auto& style = element_style.second;
		Utf8String colorattr;
		if (style.fg_color != "default") {
			colorattr.append("fg=");
			colorattr.append(style.fg_color);
		}
		if (style.bg_color != "default") {
			if (colorattr.length() > 0) {
				colorattr.append(",");
			}
			colorattr.append("bg=");
			colorattr.append(style.bg_color);
		}
		for (const auto& attr : style.attributes) {
			if (colorattr.length() > 0) {
				colorattr.append(",");
			}
			colorattr.append("attr=");
			colorattr.append(attr);
		}

		LOG(Level::DEBUG,
			"ColorManager::set_pb_colors: %s %s\n",
			element,
			colorattr);

		stfl_value_setter(element.to_utf8(), colorattr.to_utf8());

		if (element == "article") {
			Utf8String bold = colorattr;
			Utf8String ul = colorattr;
			if (bold.length() > 0) {
				bold.append(",");
			}
			if (ul.length() > 0) {
				ul.append(",");
			}
			bold.append("attr=bold");
			ul.append("attr=underline");
			// STFL will just ignore those in forms which don't have the
			// `color_bold` and `color_underline` variables.
			stfl_value_setter("color_bold", bold.to_utf8());
			stfl_value_setter("color_underline", ul.to_utf8());
		}
	}
}

} // namespace newsboat

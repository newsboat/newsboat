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
		std::string element = params[0];
		std::string fgcolor = params[1];
		std::string bgcolor = params[2];

		if (!utils::is_valid_color(fgcolor)) {
			throw ConfigHandlerException(strprintf::fmt(
					_("`%s' is not a valid color"), fgcolor));
		}
		if (!utils::is_valid_color(bgcolor)) {
			throw ConfigHandlerException(strprintf::fmt(
					_("`%s' is not a valid color"), bgcolor));
		}

		std::vector<std::string> attribs;
		for (unsigned int i = 3; i < params.size(); ++i) {
			if (!utils::is_valid_attribute(params[i])) {
				throw ConfigHandlerException(strprintf::fmt(
						_("`%s' is not a valid attribute"),
						params[i]));
			}
			attribs.push_back(params[i]);
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

void ColorManager::apply_colors(Stfl::Form& form) const
{
	for (const auto& element_style : element_styles) {
		const std::string& element = element_style.first;
		const TextStyle& style = element_style.second;
		std::string colorattr;
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

		form.set(element, colorattr);

		if (element == "article") {
			std::string bold = colorattr;
			std::string ul = colorattr;
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
			form.set("color_bold", bold);
			form.set("color_underline", ul);
		}
	}
}

} // namespace newsboat

#include "colormanager.h"

#include "config.h"
#include "exceptions.h"
#include "feedlist_formaction.h"
#include "filebrowser_formaction.h"
#include "help_formaction.h"
#include "itemlist_formaction.h"
#include "itemview_formaction.h"
#include "logger.h"
#include "pb_view.h"
#include "select_formaction.h"
#include "strprintf.h"
#include "urlview_formaction.h"
#include "utils.h"

using namespace podboat;

namespace newsboat {

colormanager::colormanager()
	: colors_loaded_(false)
{
}

colormanager::~colormanager() {}

void colormanager::register_commands(configparser& cfgparser)
{
	cfgparser.register_handler("color", this);
}

void colormanager::handle_action(const std::string& action,
	const std::vector<std::string>& params)
{
	LOG(level::DEBUG,
		"colormanager::handle_action(%s,...) was called",
		action);
	if (action == "color") {
		if (params.size() < 3) {
			throw confighandlerexception(
				action_handler_status::TOO_FEW_PARAMS);
		}

		/*
		 * the command syntax is:
		 * color <element> <fgcolor> <bgcolor> [<attribute> ...]
		 */
		std::string element = params[0];
		std::string fgcolor = params[1];
		std::string bgcolor = params[2];

		if (!utils::is_valid_color(fgcolor))
			throw confighandlerexception(strprintf::fmt(
				_("`%s' is not a valid color"), fgcolor));
		if (!utils::is_valid_color(bgcolor))
			throw confighandlerexception(strprintf::fmt(
				_("`%s' is not a valid color"), bgcolor));

		std::vector<std::string> attribs;
		for (unsigned int i = 3; i < params.size(); ++i) {
			if (!utils::is_valid_attribute(params[i]))
				throw confighandlerexception(strprintf::fmt(
					_("`%s' is not a valid attribute"),
					params[i]));
			attribs.push_back(params[i]);
		}

		/* we only allow certain elements to be configured, also to
		 * indicate the user possible mis-spellings */
		if (element == "listnormal" || element == "listfocus" ||
			element == "listnormal_unread" ||
			element == "listfocus_unread" || element == "info" ||
			element == "background" || element == "article") {
			fg_colors[element] = fgcolor;
			bg_colors[element] = bgcolor;
			attributes[element] = attribs;
			colors_loaded_ = true;
		} else
			throw confighandlerexception(strprintf::fmt(
				_("`%s' is not a valid configuration element"),
				element));

	} else
		throw confighandlerexception(
			action_handler_status::INVALID_COMMAND);
}

void colormanager::dump_config(std::vector<std::string>& config_output)
{
	for (auto color : fg_colors) {
		std::string configline = strprintf::fmt("color %s %s %s",
			color.first,
			color.second,
			bg_colors[color.first]);
		for (auto attrib : attributes[color.first]) {
			configline.append(" ");
			configline.append(attrib);
		}
		config_output.push_back(configline);
	}
}

/*
 * this is podboat-specific color management
 * TODO: refactor this
 */
void colormanager::set_pb_colors(podboat::pb_view* v)
{
	auto fgcit = fg_colors.begin();
	auto bgcit = bg_colors.begin();
	auto attit = attributes.begin();

	for (; fgcit != fg_colors.end(); ++fgcit, ++bgcit, ++attit) {
		std::string colorattr;
		if (fgcit->second != "default") {
			colorattr.append("fg=");
			colorattr.append(fgcit->second);
		}
		if (bgcit->second != "default") {
			if (colorattr.length() > 0)
				colorattr.append(",");
			colorattr.append("bg=");
			colorattr.append(bgcit->second);
		}
		for (auto attr : attit->second) {
			if (colorattr.length() > 0)
				colorattr.append(",");
			colorattr.append("attr=");
			colorattr.append(attr);
		}

		LOG(level::DEBUG,
			"colormanager::set_pb_colors: %s %s\n",
			fgcit->first,
			colorattr);

		v->dllist_form.set(fgcit->first, colorattr);
		v->help_form.set(fgcit->first, colorattr);

		if (fgcit->first == "article") {
			std::string styleend_str;

			if (bgcit->second != "default") {
				styleend_str.append("bg=");
				styleend_str.append(bgcit->second);
			}
			if (styleend_str.length() > 0)
				styleend_str.append(",");
			styleend_str.append("attr=bold");

			v->help_form.set("styleend", styleend_str.c_str());
		}
	}
}

} // namespace newsboat

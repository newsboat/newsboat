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
			throw ConfigHandlerException(
				ActionHandlerStatus::TOO_FEW_PARAMS);
		}

		/*
		 * the command syntax is:
		 * color <element> <fgcolor> <bgcolor> [<attribute> ...]
		 */
		std::string element = params[0];
		std::string fgcolor = params[1];
		std::string bgcolor = params[2];

		if (!utils::is_valid_color(fgcolor))
			throw ConfigHandlerException(strprintf::fmt(
					_("`%s' is not a valid color"), fgcolor));
		if (!utils::is_valid_color(bgcolor))
			throw ConfigHandlerException(strprintf::fmt(
					_("`%s' is not a valid color"), bgcolor));

		std::vector<std::string> attribs;
		for (unsigned int i = 3; i < params.size(); ++i) {
			if (!utils::is_valid_attribute(params[i]))
				throw ConfigHandlerException(strprintf::fmt(
						_("`%s' is not a valid attribute"),
						params[i]));
			attribs.push_back(params[i]);
		}

		/* we only allow certain elements to be configured, also to
		 * indicate the user possible mis-spellings */
		if (element == "listnormal" || element == "listfocus" ||
			element == "listnormal_unread" ||
			element == "listfocus_unread" || element == "info" ||
			element == "background" || element == "article" ||
			element == "end-of-text-marker") {
			fg_colors[element] = fgcolor;
			bg_colors[element] = bgcolor;
			attributes[element] = attribs;
		} else
			throw ConfigHandlerException(strprintf::fmt(
					_("`%s' is not a valid configuration element"),
					element));

	} else
		throw ConfigHandlerException(
			ActionHandlerStatus::INVALID_COMMAND);
}

void ColorManager::dump_config(std::vector<std::string>& config_output)
{
	for (const auto& color : fg_colors) {
		std::string configline = strprintf::fmt("color %s %s %s",
				color.first,
				color.second,
				bg_colors[color.first]);
		for (const auto& attrib : attributes[color.first]) {
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
void ColorManager::set_pb_colors(podboat::PbView* v)
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
			if (colorattr.length() > 0) {
				colorattr.append(",");
			}
			colorattr.append("bg=");
			colorattr.append(bgcit->second);
		}
		for (const auto& attr : attit->second) {
			if (colorattr.length() > 0) {
				colorattr.append(",");
			}
			colorattr.append("attr=");
			colorattr.append(attr);
		}

		LOG(Level::DEBUG,
			"ColorManager::set_pb_colors: %s %s\n",
			fgcit->first,
			colorattr);

		v->dllist_form.set(fgcit->first, colorattr);
		v->help_form.set(fgcit->first, colorattr);
	}
}

} // namespace newsboat

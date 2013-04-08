#include <logger.h>
#include <colormanager.h>
#include <utils.h>
#include <pb_view.h>

#include <feedlist_formaction.h>
#include <itemlist_formaction.h>
#include <itemview_formaction.h>
#include <help_formaction.h>
#include <filebrowser_formaction.h>
#include <urlview_formaction.h>
#include <select_formaction.h>
#include <exceptions.h>
#include <config.h>

using namespace podbeuter;

namespace newsbeuter {

colormanager::colormanager() : colors_loaded_(false) { }

colormanager::~colormanager() { }

void colormanager::register_commands(configparser& cfgparser) {
	cfgparser.register_handler("color", this);
}

void colormanager::handle_action(const std::string& action, const std::vector<std::string>& params) {
	LOG(LOG_DEBUG, "colormanager::handle_action(%s,...) was called",action.c_str());
	if (action == "color") {
		if (params.size() < 3) {
			throw confighandlerexception(AHS_TOO_FEW_PARAMS);
		}

		/*
		 * the command syntax is:
		 * color <element> <fgcolor> <bgcolor> [<attribute> ...]
		 */
		std::string element = params[0];
		std::string fgcolor = params[1];
		std::string bgcolor = params[2];

		if (!utils::is_valid_color(fgcolor))
			throw confighandlerexception(utils::strprintf(_("`%s' is not a valid color"), fgcolor.c_str()));
		if (!utils::is_valid_color(bgcolor))
			throw confighandlerexception(utils::strprintf(_("`%s' is not a valid color"), bgcolor.c_str()));
		
		std::vector<std::string> attribs;
		for (unsigned int i=3;i<params.size();++i) {
			if (!utils::is_valid_attribute(params[i]))
				throw confighandlerexception(utils::strprintf(_("`%s' is not a valid attribute"), params[i].c_str()));
			attribs.push_back(params[i]);
		}

		/* we only allow certain elements to be configured, also to indicate the user possible mis-spellings */
		if (element == "listnormal" || element == "listfocus" || element == "listnormal_unread" || element =="listfocus_unread"
                || element == "info" || element == "background" || element == "article") {
			fg_colors[element] = fgcolor;
			bg_colors[element] = bgcolor;
			attributes[element] = attribs;
			colors_loaded_ = true;
		} else
			throw confighandlerexception(utils::strprintf(_("`%s' is not a valid configuration element")));

	} else
		throw confighandlerexception(AHS_INVALID_COMMAND);
}

void colormanager::dump_config(std::vector<std::string>& config_output) {
	for (std::map<std::string, std::string>::iterator it=fg_colors.begin();it!=fg_colors.end();++it) {
		std::string configline = utils::strprintf("color %s %s %s", it->first.c_str(), it->second.c_str(), bg_colors[it->first].c_str());
		std::vector<std::string> attribs = attributes[it->first];
		for (std::vector<std::string>::iterator jt=attribs.begin();jt!=attribs.end();++jt) {
			configline.append(" ");
			configline.append(*jt);
		}
		config_output.push_back(configline);
	}
}

/*
 * this is podbeuter-specific color management
 * TODO: refactor this
 */
void colormanager::set_pb_colors(podbeuter::pb_view * v) {
	std::map<std::string,std::string>::iterator fgcit = fg_colors.begin();
	std::map<std::string,std::string>::iterator bgcit = bg_colors.begin();
	std::map<std::string,std::vector<std::string> >::iterator attit = attributes.begin();

	for (;fgcit != fg_colors.end(); ++fgcit, ++bgcit, ++attit) {
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
		for (std::vector<std::string>::iterator it=attit->second.begin(); it!= attit->second.end(); ++it) {
			if (colorattr.length() > 0)
				colorattr.append(",");
			colorattr.append("attr=");
			colorattr.append(*it);
		} 

		LOG(LOG_DEBUG,"colormanager::set_pb_colors: %s %s\n",fgcit->first.c_str(), colorattr.c_str());

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


}

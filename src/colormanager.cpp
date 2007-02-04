#include <logger.h>
#include <colormanager.h>

namespace newsbeuter {

colormanager::colormanager() : colors_loaded_(false) { }

colormanager::~colormanager() { }

void colormanager::register_commands(configparser& cfgparser) {
	cfgparser.register_handler("color", this);
}

action_handler_status colormanager::handle_action(const std::string& action, const std::vector<std::string>& params) {
	GetLogger().log(LOG_DEBUG, "colormanager::handle_action(%s,...) was called",action.c_str());
	if (action == "color") {
		if (params.size() < 3) {
			return AHS_TOO_FEW_PARAMS;
		}

		std::string element = params[0];
		std::string fgcolor = params[1];
		std::string bgcolor = params[2];

		std::vector<std::string> attribs;
		for (unsigned int i=3;i<params.size();++i) {
			attribs.push_back(params[i]);
		}

		if (element == "listnormal" || element == "listfocus" || element == "info" || element == "background" || element == "article") {
			fg_colors[element] = fgcolor;
			bg_colors[element] = bgcolor;
			attributes[element] = attribs;
			colors_loaded_ = true;
			return AHS_OK;
		} else
			return AHS_INVALID_PARAMS;

	} else
		return AHS_INVALID_COMMAND;
}

void colormanager::set_colors(view * v) {
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

		GetLogger().log(LOG_DEBUG,"colormanager::set_colors: %s %s\n",fgcit->first.c_str(), colorattr.c_str());

		v->feedlist_form.set(fgcit->first, colorattr);
		v->itemlist_form.set(fgcit->first, colorattr);
		v->itemview_form.set(fgcit->first, colorattr);
		v->help_form.set(fgcit->first, colorattr);
		v->filebrowser_form.set(fgcit->first, colorattr);
		v->urlview_form.set(fgcit->first, colorattr);
		v->selecttag_form.set(fgcit->first, colorattr);

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
			v->itemview_form.set("styleend", styleend_str.c_str());
		}
	}
}


}

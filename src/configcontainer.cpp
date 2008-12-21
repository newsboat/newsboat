#include <config.h>
#include <configcontainer.h>
#include <configparser.h>
#include <logger.h>
#include <sstream>
#include <iostream>
#include <utils.h>

#include <sys/types.h>
#include <pwd.h>


namespace newsbeuter
{

configcontainer::configcontainer()
{
	// create the config options and set their resp. default value and type
	config_data["show-read-feeds"] = configdata("yes", configdata::BOOL);
	config_data["browser"]         = configdata("lynx", configdata::PATH);
	config_data["use-proxy"]       = configdata("no", configdata::BOOL);
	config_data["auto-reload"]     = configdata("no", configdata::BOOL);
	config_data["reload-time"]     = configdata("60", configdata::INT);
	config_data["max-items"]       = configdata("0", configdata::INT);
	config_data["save-path"]       = configdata("~/", configdata::PATH);
	config_data["download-path"]   = configdata("~/", configdata::PATH);
	config_data["max-downloads"]   = configdata("1", configdata::INT);
	config_data["podcast-auto-enqueue"] = configdata("no", configdata::BOOL);
	config_data["player"]          = configdata("", configdata::PATH);
	config_data["cleanup-on-quit"] = configdata("yes", configdata::BOOL);
	config_data["user-agent"]      = configdata("", configdata::STR);
	config_data["refresh-on-startup"] = configdata("no", configdata::BOOL);
	config_data["suppress-first-reload"] = configdata("no", configdata::BOOL);
	config_data["cache-file"]      = configdata("", configdata::PATH);
	config_data["proxy"]           = configdata("", configdata::STR);
	config_data["proxy-auth"]      = configdata("", configdata::STR);
	config_data["confirm-exit"]    = configdata("no", configdata::BOOL);
	config_data["error-log"]       = configdata("", configdata::PATH);
	config_data["notify-screen"]   = configdata("no", configdata::BOOL);
	config_data["notify-xterm"]    = configdata("no", configdata::BOOL);
	config_data["notify-program"]  = configdata("", configdata::PATH);
	config_data["notify-format"]   = configdata(_("newsbeuter: finished reload, %f unread feeds (%n unread articles total)"), configdata::STR);
	config_data["datetime-format"] = configdata("%b %d", configdata::STR);
	config_data["urls-source"]     = configdata("local", configdata::STR);
	config_data["bloglines-auth"]  = configdata("", configdata::STR);
	config_data["bloglines-mark-read"] = configdata("no", configdata::BOOL);
	config_data["bookmark-cmd"]    = configdata("", configdata::STR);
	config_data["opml-url"]        = configdata("", configdata::STR, true);
	config_data["html-renderer"]   = configdata("internal", configdata::PATH);
	config_data["feedlist-format"] = configdata("%4i %n %11u %t", configdata::STR);
	config_data["articlelist-format"] = configdata("%4i %f %D   %?T?|%-17T|  &?%t", configdata::STR);
	config_data["text-width"]      = configdata("0", configdata::INT);
	config_data["always-display-description"] = configdata("false", configdata::BOOL);
	config_data["reload-only-visible-feeds"] = configdata("false", configdata::BOOL);
	config_data["article-sort-order"] = configdata("date", configdata::STR);
	config_data["show-read-articles"] = configdata("yes", configdata::BOOL);
	config_data["goto-next-feed"] = configdata("yes", configdata::BOOL);
	config_data["display-article-progress"] = configdata("yes", configdata::BOOL);
	config_data["show-keymap-hint"] = configdata("yes", configdata::BOOL);
	config_data["download-timeout"] = configdata("30", configdata::INT);
	config_data["download-retries"] = configdata("1", configdata::INT);
	config_data["feed-sort-order"] = configdata("none", configdata::STR);
	config_data["reload-threads"] = configdata("1", configdata::INT);
	config_data["keep-articles-days"] = configdata("0", configdata::INT);


	/* undocumented: */
	config_data["feedlist-title-format"] = configdata(_("%N %V - Your feeds (%u unread, %t total)%?T? - tag `%T'&?"), configdata::STR);
	config_data["urlview-title-format"] = configdata(_("%N %V - URLs"), configdata::STR);
	config_data["articlelist-title-format"] = configdata(_("%N %V - Articles in feed '%T' (%u unread, %t total) - %U"), configdata::STR);
	config_data["searchresult-title-format"] = configdata(_("%N %V - Search result (%u unread, %t total)"), configdata::STR);
	config_data["filebrowser-title-format"] = configdata(_("%N %V - %?O?Open File&Save File? - %f"), configdata::STR);
	config_data["help-title-format"] = configdata(_("%N %V - Help"), configdata::STR);
	config_data["selecttag-title-format"] = configdata(_("%N %V - Select Tag"), configdata::STR);
	config_data["selectfilter-title-format"] = configdata(_("%N %V - Select Filter"), configdata::STR);
	config_data["itemview-title-format"] = configdata(_("%N %V - Article '%T'"), configdata::STR);
	config_data["urlview-title-format"] = configdata(_("%N %V - URLs"), configdata::STR);
	config_data["dialogs-title-format"] = configdata(_("%N %V - Dialogs"), configdata::STR);
}

configcontainer::~configcontainer()
{
}

void configcontainer::register_commands(configparser& cfgparser)
{
	// this registers the config options defined above in the configuration parser
	// -> if the resp. config option is encountered, it is passed to the configcontainer
	for (std::map<std::string,configdata>::iterator it=config_data.begin();it!=config_data.end();++it) {
		cfgparser.register_handler(it->first, this);
	}
}

action_handler_status configcontainer::handle_action(const std::string& action, const std::vector<std::string>& params) {
	configdata& cfgdata = config_data[action];

	// configdata::INVALID indicates that the action didn't exist, and that the returned object was created ad-hoc.
	if (cfgdata.type == configdata::INVALID) {
		GetLogger().log(LOG_WARN, "configcontainer::handler_action: unknown action %s", action.c_str());
		return AHS_INVALID_COMMAND;	
	}

	GetLogger().log(LOG_DEBUG, "configcontainer::handle_action: action = %s, type = %u", action.c_str(), cfgdata.type);

	if (params.size() < 1) {
		return AHS_TOO_FEW_PARAMS;
	}

	switch (cfgdata.type) {
		case configdata::BOOL:
			if (!is_bool(params[0]))
				return AHS_INVALID_PARAMS;
			cfgdata.value = params[0];
			return AHS_OK; 

		case configdata::INT:
			if (!is_int(params[0]))
				return AHS_INVALID_PARAMS;
			cfgdata.value = params[0];
			return AHS_OK;	

		case configdata::STR:
		case configdata::PATH:
			if (cfgdata.multi_option) {
				cfgdata.value = utils::join(params, " ");
			} else
				cfgdata.value = params[0];
			return AHS_OK;	

		default:
			// should not happen
			return AHS_INVALID_COMMAND;	
	}

	// should not happen
	return AHS_INVALID_COMMAND;	
}

bool configcontainer::is_bool(const std::string& s) {
	const char * bool_values[] = { "yes", "no", "true", "false", 0 };
	for (int i=0; bool_values[i] ; ++i) {
		if (s == bool_values[i])
			return true;
	}
	return false;
}

bool configcontainer::is_int(const std::string& s) {
	const char * s1 = s.c_str();
	for (;*s1;s1++) {
		if (!isdigit(*s1))
			return false;
	}
	return true;
}

std::string configcontainer::get_configvalue(const std::string& key) {
	std::string retval = config_data[key].value;
	if (config_data[key].type == configdata::PATH) {
		retval = utils::resolve_tilde(retval);
	}

	return retval;
}

int configcontainer::get_configvalue_as_int(const std::string& key) {
	std::istringstream is(config_data[key].value);
	int i;
	is >> i;
	return i;
}

bool configcontainer::get_configvalue_as_bool(const std::string& key) {
	if (config_data[key].value == "true" || config_data[key].value == "yes")
		return true;
	return false;
}

void configcontainer::set_configvalue(const std::string& key, const std::string& value) {
	GetLogger().log(LOG_DEBUG,"configcontainer::set_configvalue(%s,%s) called", key.c_str(), value.c_str());
	config_data[key].value = value;
}

}

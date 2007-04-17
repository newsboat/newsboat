#include <configcontainer.h>
#include <configparser.h>
#include <logger.h>
#include <sstream>
#include <iostream>

#include <sys/types.h>
#include <pwd.h>


namespace newsbeuter
{

// TODO: refactor this. having the configuration command literals scattered around the place several times is unacceptable

configcontainer::configcontainer()
{
	// configure default values
	config_data["show-read-feeds"] = "yes";
	config_data["browser"] = "lynx";
	config_data["use-proxy"] = "no";
	config_data["auto-reload"] = "no";
	config_data["reload-time"] = "30";
	config_data["max-items"] = "0";
	config_data["save-path"] = "~/";
	config_data["download-path"] = "~/";
	config_data["max-downloads"] = "1";
	config_data["podcast-auto-enqueue"] = "no";
	config_data["player"] = "";
	config_data["cleanup-on-quit"] = "yes";
	config_data["user-agent"] = "";
	config_data["refresh-on-startup"] = "no";
	config_data["suppress-first-reload"] = "no";
	config_data["cache-file"] = "";
}

configcontainer::~configcontainer()
{
}

void configcontainer::register_commands(configparser& cfgparser)
{
	// register this as handler for the supported configuration commands
	cfgparser.register_handler("show-read-feeds", this);
	cfgparser.register_handler("browser", this);
	cfgparser.register_handler("max-items", this);
	cfgparser.register_handler("use-proxy", this);
	cfgparser.register_handler("proxy", this);
	cfgparser.register_handler("proxy-auth", this);
	cfgparser.register_handler("auto-reload", this);
	cfgparser.register_handler("reload-time", this);
	cfgparser.register_handler("save-path", this);
	cfgparser.register_handler("download-path", this);
	cfgparser.register_handler("max-downloads", this);
	cfgparser.register_handler("podcast-auto-enqueue", this);
	cfgparser.register_handler("player", this);
	cfgparser.register_handler("cleanup-on-quit", this);
	cfgparser.register_handler("user-agent", this);
	cfgparser.register_handler("refresh-on-startup", this);
	cfgparser.register_handler("suppress-first-reload", this);
	cfgparser.register_handler("cache-file", this);
}

action_handler_status configcontainer::handle_action(const std::string& action, const std::vector<std::string>& params) {
	// handle the action when a configuration command has been encountered
	GetLogger().log(LOG_DEBUG, "configcontainer::handle_action(%s,...) was called",action.c_str());

	// the bool configuration values
	if (action == "show-read-feeds" || action == "auto-reload" || action == "podcast-auto-enqueue" || action == "cleanup-on-quit" || action == "refresh-on-startup" || action == "suppress-first-reload" || action == "use-proxy") {
		if (params.size() < 1) {
			return AHS_TOO_FEW_PARAMS;
		}
		if (!is_bool(params[0])) {
			return AHS_INVALID_PARAMS;
		}
		config_data[action] = params[0];
		// std::cerr << "setting " << action << " to `" << params[0] << "'" << std::endl;
		return AHS_OK; 
	// the integer configuration values
	} else if (action == "max-items" || action == "reload-time" || action == "max-downloads") {
		if (params.size() < 1) {
			return AHS_TOO_FEW_PARAMS;
		}

		if (!is_int(params[0])) {
			return AHS_INVALID_PARAMS;
		}

		config_data[action] = params[0];
		return AHS_OK;	

	// the regular string values
	} else if (action == "proxy" || action == "proxy-auth" || action == "user-agent") {
		if (params.size() < 1) {
			return AHS_TOO_FEW_PARAMS;
		}
		config_data[action] = params[0];
		return AHS_OK;

	// the path string values where ~/ substitution has to happen before
	} else if (action == "cache-file" || action == "save-path" || action == "download-path" || action == "player" || action == "browser") {
		if (params.size() < 1) {
			return AHS_TOO_FEW_PARAMS;
		}

		char * homedir;
		std::string filepath;

		if (!(homedir = ::getenv("HOME"))) {
			struct passwd * spw = ::getpwuid(::getuid());
			if (spw) {
					homedir = spw->pw_dir;
			} else {
					homedir = "";
			}
		}

		if (strcmp(homedir,"")!=0 && params[0].substr(0,2) == "~/") {
			filepath.append(homedir);
			filepath.append(1,'/');
			filepath.append(params[0].substr(2,params[0].length()-2));
		} else {
			filepath.append(params[0]);
		}

		config_data[action] = filepath;

		GetLogger().log(LOG_DEBUG, "configcontainer::handle_action: %s = %s", action.c_str(), filepath.c_str());

		return AHS_OK;
	}
	return AHS_INVALID_COMMAND;	
}

bool configcontainer::is_bool(const std::string& s) {
	char * bool_values[] = { "yes", "no", "true", "false", 0 };
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
	return config_data[key];
}

int configcontainer::get_configvalue_as_int(const std::string& key) {
	std::istringstream is(config_data[key]);
	int i;
	is >> i;
	return i;	
}

bool configcontainer::get_configvalue_as_bool(const std::string& key) {
	if (config_data[key] == "true" || config_data[key] == "yes")
		return true;
	return false;	
}

void configcontainer::set_configvalue(const std::string& key, const std::string& value) {
	GetLogger().log(LOG_DEBUG,"configcontainer::set_configvalue(%s,%s) called", key.c_str(), value.c_str());
	config_data[key] = value;
}

}

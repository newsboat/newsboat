#include <configcontainer.h>
#include <configparser.h>
#include <sstream>
#include <iostream>

namespace noos
{

configcontainer::configcontainer()
{
	// configure default values
	config_data["show-read-feeds"] = "yes";
	config_data["browser"] = "lynx";
}

configcontainer::~configcontainer()
{
}

void configcontainer::register_commands(configparser& cfgparser)
{
	cfgparser.register_handler("show-read-feeds", this);
	cfgparser.register_handler("browser", this);
	cfgparser.register_handler("max-items", this);
}

action_handler_status configcontainer::handle_action(const std::string& action, const std::vector<std::string>& params) {
	if (action == "show-read-feeds") {
		if (params.size() < 1) {
			return AHS_TOO_FEW_PARAMS;
		}
		if (!is_bool(params[0])) {
			return AHS_INVALID_PARAMS;
		}
		config_data[action] = params[0];
		// std::cerr << "setting " << action << " to `" << params[0] << "'" << std::endl;
		return AHS_OK; 
	} else if (action == "browser") {
		if (params.size() < 1) {
			return AHS_TOO_FEW_PARAMS;
		}
		config_data[action] = params[0];
		return AHS_OK;	
	} else if (action == "max-items") {
		if (params.size() < 1) {
			return AHS_TOO_FEW_PARAMS;
		}
		config_data[action] = params[0];
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

}

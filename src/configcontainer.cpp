#include <configcontainer.h>
#include <configparser.h>
#include <sstream>

namespace noos
{

configcontainer::configcontainer()
{
	// configure default values
	config_data["show-read-feeds"] = "yes";
}

configcontainer::~configcontainer()
{
}

void configcontainer::register_commands(configparser& cfgparser)
{
	cfgparser.register_handler("show-read-feeds",this);
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
	if (key == "true" || key == "yes")
		return true;
	return false;	
}

}

#include <configcontainer.h>
#include <configparser.h>

namespace noos
{

configcontainer::configcontainer()
{
}

configcontainer::~configcontainer()
{
}

void configcontainer::register_commands(configparser& cfgparser)
{
	// TODO: register actual commands
}

action_handler_status configcontainer::handle_action(const std::string& action, const std::vector<std::string>& params) {
	// TODO: implement action handler
	return AHS_OK;	
}

}

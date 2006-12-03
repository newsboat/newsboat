#ifndef CONFIGCONTAINER_H_
#define CONFIGCONTAINER_H_

#include <configparser.h>

namespace noos
{

class configcontainer : public config_action_handler
{
public:
	configcontainer();
	virtual ~configcontainer();
	void register_commands(configparser& cfgparser);
	virtual action_handler_status handle_action(const std::string& action, const std::vector<std::string>& params);
private:
	// TODO: store private data
};

}

#endif /*CONFIGCONTAINER_H_*/

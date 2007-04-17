#ifndef CONFIGCONTAINER_H_
#define CONFIGCONTAINER_H_

#include <configparser.h>

namespace newsbeuter
{

class configcontainer : public config_action_handler
{
public:
	configcontainer();
	virtual ~configcontainer();
	void register_commands(configparser& cfgparser);
	virtual action_handler_status handle_action(const std::string& action, const std::vector<std::string>& params);
	bool get_configvalue_as_bool(const std::string& key);
	int get_configvalue_as_int(const std::string& key);
	std::string get_configvalue(const std::string& key);
	void set_configvalue(const std::string& key, const std::string& value);
private:
	std::map<std::string,std::string> config_data;
	
	bool is_bool(const std::string& s);
	bool is_int(const std::string& s);
};

}

#endif /*CONFIGCONTAINER_H_*/

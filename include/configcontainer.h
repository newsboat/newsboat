#ifndef CONFIGCONTAINER_H_
#define CONFIGCONTAINER_H_

#include <configparser.h>

namespace newsbeuter
{


struct configdata
{
	enum configdata_type { INVALID, BOOL, INT, STR, PATH, ALIAS, ENUM };
	configdata(std::string v = "", configdata_type t = INVALID, bool m = false) : value(v), default_value(v), type(t), multi_option(m) { }
	configdata(std::string v, ...);
	std::string value;
	std::string default_value;
	configdata_type type;
	std::set<std::string> enum_values;
	bool multi_option;
};

class configcontainer : public config_action_handler
{
public:
	configcontainer();
	virtual ~configcontainer();
	void register_commands(configparser& cfgparser);
	virtual void handle_action(const std::string& action, const std::vector<std::string>& params);
	virtual void dump_config(std::vector<std::string>& config_output);

	bool get_configvalue_as_bool(const std::string& key);
	int get_configvalue_as_int(const std::string& key);
	std::string get_configvalue(const std::string& key);
	void set_configvalue(const std::string& key, const std::string& value);
	void reset_to_default(const std::string& key);
	void toggle(const std::string& key);
	std::vector<std::string> get_suggestions(const std::string& fragment);
private:
	std::map<std::string,configdata> config_data;
	
	bool is_bool(const std::string& s);
	bool is_int(const std::string& s);
	std::string lookup_alias(const std::string& s);
};

}

#endif /*CONFIGCONTAINER_H_*/

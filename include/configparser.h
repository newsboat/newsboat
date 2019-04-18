#ifndef NEWSBOAT_CONFIGPARSER_H_
#define NEWSBOAT_CONFIGPARSER_H_

#include <map>
#include <set>
#include <string>
#include <vector>

namespace newsboat {

enum class ActionHandlerStatus {
	VALID = 0,
	INVALID_PARAMS,
	TOO_FEW_PARAMS,
	INVALID_COMMAND,
	FILENOTFOUND
};

struct ConfigActionHandler {
	virtual void handle_action(const std::string& action,
		const std::vector<std::string>& params) = 0;
	virtual void dump_config(std::vector<std::string>& config_output) = 0;
	ConfigActionHandler() {}
	virtual ~ConfigActionHandler() {}
};

class ConfigParser : public ConfigActionHandler {
public:
	ConfigParser();
	~ConfigParser() override;
	void register_handler(const std::string& cmd,
		ConfigActionHandler* handler);
	void unregister_handler(const std::string& cmd);
	void handle_action(const std::string& action,
		const std::vector<std::string>& params) override;
	void dump_config(std::vector<std::string>&) override
	{
		/* nothing because ConfigParser itself only handles include */
	}
	bool parse(const std::string& filename);
	static std::string evaluate_backticks(std::string token);

private:
	void evaluate_backticks(std::vector<std::string>& tokens);
	static std::string evaluate_cmd(const std::string& cmd);
	std::vector<std::vector<std::string>> parsed_content;
	std::map<std::string, ConfigActionHandler*> action_handlers;
	std::vector<std::string> included_files;
};

class NullConfigActionHandler : public ConfigActionHandler {
public:
	NullConfigActionHandler() {}
	~NullConfigActionHandler() override {}
	void handle_action(const std::string&,
		const std::vector<std::string>&) override
	{
	}
	void dump_config(std::vector<std::string>&) override {}
};

} // namespace newsboat

#endif /* NEWSBOAT_CONFIGPARSER_H_ */

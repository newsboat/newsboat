#ifndef NEWSBOAT_CONFIGPARSER_H_
#define NEWSBOAT_CONFIGPARSER_H_

#include <map>
#include <set>
#include <string>
#include <vector>

namespace newsboat {

enum class action_handler_status {
	OK = 0,
	INVALID_PARAMS,
	TOO_FEW_PARAMS,
	INVALID_COMMAND,
	FILENOTFOUND
};

struct config_action_handler {
	virtual void handle_action(
		const std::string& action,
		const std::vector<std::string>& params) = 0;
	virtual void dump_config(std::vector<std::string>& config_output) = 0;
	config_action_handler() {}
	virtual ~config_action_handler() {}
};

class configparser : public config_action_handler {
public:
	configparser();
	~configparser() override;
	void register_handler(
		const std::string& cmd,
		config_action_handler* handler);
	void unregister_handler(const std::string& cmd);
	void handle_action(
		const std::string& action,
		const std::vector<std::string>& params) override;
	void dump_config(std::vector<std::string>&) override
	{
		/* nothing because configparser itself only handles include */
	}
	bool parse(const std::string& filename, bool double_include = true);
	static std::string evaluate_backticks(std::string token);

private:
	void evaluate_backticks(std::vector<std::string>& tokens);
	static std::string evaluate_cmd(const std::string& cmd);
	std::vector<std::vector<std::string>> parsed_content;
	std::map<std::string, config_action_handler*> action_handlers;
	std::set<std::string> included_files;
};

class null_config_action_handler : public config_action_handler {
public:
	null_config_action_handler() {}
	~null_config_action_handler() override {}
	void handle_action(const std::string&, const std::vector<std::string>&)
		override
	{
	}
	void dump_config(std::vector<std::string>&) override {}
};

} // namespace newsboat

#endif /* NEWSBOAT_CONFIGPARSER_H_ */

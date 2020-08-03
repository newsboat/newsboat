#ifndef NEWSBOAT_CONFIGPARSER_H_
#define NEWSBOAT_CONFIGPARSER_H_

#include <functional>
#include <map>

#include "configactionhandler.h"

namespace newsboat {

enum class ActionHandlerStatus {
	VALID = 0,
	INVALID_PARAMS,
	TOO_FEW_PARAMS,
	INVALID_COMMAND,
	FILENOTFOUND
};

class ConfigParser : public ConfigActionHandler {
public:
	ConfigParser();
	~ConfigParser() override;
	void register_handler(const std::string& cmd,
		ConfigActionHandler& handler);
	void handle_action(const std::string& action,
		const std::vector<std::string>& params) override;
	void dump_config(std::vector<std::string>&) override
	{
		/* nothing because ConfigParser itself only handles include */
	}
	bool parse_file(const std::string& filename);
	static std::string evaluate_backticks(std::string token);

private:
	static std::string evaluate_cmd(const std::string& cmd);
	std::vector<std::vector<std::string>> parsed_content;
	std::map<std::string, std::reference_wrapper<ConfigActionHandler>>
		action_handlers;
	std::vector<std::string> included_files;
};

} // namespace newsboat

#endif /* NEWSBOAT_CONFIGPARSER_H_ */

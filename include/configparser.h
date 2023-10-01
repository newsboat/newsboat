#ifndef NEWSBOAT_CONFIGPARSER_H_
#define NEWSBOAT_CONFIGPARSER_H_

#include <functional>
#include <map>

#include "configactionhandler.h"
#include "filepath.h"

namespace newsboat {

enum class ActionHandlerStatus {
	INVALID_PARAMS,
	TOO_FEW_PARAMS,
	TOO_MANY_PARAMS,
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
	void dump_config(std::vector<std::string>&) const override
	{
		/* nothing because ConfigParser itself only handles include */
	}

	/// Processes configuration commands from a file at \a filename.
	///
	/// Returns:
	/// - `false` if the file couldn't be opened;
	/// - `true` if the whole file was processed completely.
	///
	/// If the file contains any errors, throws `ConfigException` with
	/// a message explaining the problem.
	bool parse_file(const Filepath& filename);

	void parse_line(const std::string& line, const std::string& location);
	static std::string evaluate_backticks(std::string token);

private:
	static std::string evaluate_cmd(const std::string& cmd);
	std::vector<std::vector<std::string>> parsed_content;
	std::map<std::string, std::reference_wrapper<ConfigActionHandler>>
		action_handlers;
	std::vector<Filepath> included_files;
};

} // namespace newsboat

#endif /* NEWSBOAT_CONFIGPARSER_H_ */

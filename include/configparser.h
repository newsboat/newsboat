#ifndef NEWSBOAT_CONFIGPARSER_H_
#define NEWSBOAT_CONFIGPARSER_H_

#include <functional>
#include <map>

#include "configactionhandler.h"
#include "utf8string.h"

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
	void register_handler(const Utf8String& cmd,
		ConfigActionHandler& handler);
	void handle_action(const Utf8String& action,
		const std::vector<Utf8String>& params) override;
	void dump_config(std::vector<Utf8String>&) const override
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
	bool parse_file(const Utf8String& filename);

	void parse_line(const Utf8String& line, const Utf8String& location);
	static Utf8String evaluate_backticks(Utf8String token);

private:
	static Utf8String evaluate_cmd(const Utf8String& cmd);
	std::vector<std::vector<Utf8String>> parsed_content;
	std::map<Utf8String, std::reference_wrapper<ConfigActionHandler>>
		action_handlers;
	std::vector<Utf8String> included_files;
};

} // namespace newsboat

#endif /* NEWSBOAT_CONFIGPARSER_H_ */

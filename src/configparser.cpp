#include "configparser.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <pwd.h>
#include <sys/types.h>

#include "config.h"
#include "exceptions.h"
#include "logger.h"
#include "strprintf.h"
#include "tagsouppullparser.h"
#include "utils.h"

namespace newsboat {

ConfigParser::ConfigParser()
{
	register_handler("include", this);
}

ConfigParser::~ConfigParser() {}

void ConfigParser::handle_action(const std::string& action,
	const std::vector<std::string>& params)
{
	/*
	 * ConfigParser also acts as ConfigActionHandler to implement
	 * recursive "include" command.
	 */
	if (action == "include") {
		if (params.size() < 1) {
			throw ConfigHandlerException(
				ActionHandlerStatus::TOO_FEW_PARAMS);
		}

		if (!this->parse(Utils::resolve_tilde(params[0])))
			throw ConfigHandlerException(
				ActionHandlerStatus::FILENOTFOUND);
	} else
		throw ConfigHandlerException(
			ActionHandlerStatus::INVALID_COMMAND);
}

bool ConfigParser::parse(const std::string& filename, bool double_include)
{
	/*
	 * this function parses a config file.
	 *
	 * First, it checks whether this file has already been included, and if
	 * not, it does the following:
	 *   - open the file
	 *   - read it line by line
	 *   - tokenize every line
	 *   - if there is at least one token, look up a registered
	 * ConfigActionHandler to handle the command (which is specified in
	 * the first token)
	 *   - hand over the tokenize results to the ConfigActionHandler
	 *   - if an error happens, react accordingly.
	 */
	if (!double_include &&
		included_files.find(filename) != included_files.end()) {
		LOG(Level::WARN,
			"ConfigParser::parse: file %s has already been "
			"included",
			filename);
		return true;
	}
	included_files.insert(included_files.begin(), filename);

	unsigned int linecounter = 0;
	std::ifstream f(filename.c_str());
	std::string line;
	if (!f.is_open()) {
		LOG(Level::WARN,
			"ConfigParser::parse: file %s couldn't be opened",
			filename);
		return false;
	}
	while (f.is_open() && !f.eof()) {
		getline(f, line);
		++linecounter;
		LOG(Level::DEBUG, "ConfigParser::parse: tokenizing %s", line);
		std::vector<std::string> tokens = Utils::tokenize_quoted(line);
		if (!tokens.empty()) {
			std::string cmd = tokens[0];
			ConfigActionHandler* handler = action_handlers[cmd];
			if (handler) {
				tokens.erase(
					tokens.begin()); // delete first element
				try {
					evaluate_backticks(tokens);
					handler->handle_action(cmd, tokens);
				} catch (const ConfigHandlerException& e) {
					throw ConfigException(StrPrintf::fmt(
						_("Error while processing "
						  "command `%s' (%s line %u): "
						  "%s"),
						line,
						filename,
						linecounter,
						e.what()));
				}
			} else {
				throw ConfigException(StrPrintf::fmt(
					_("unknown command `%s'"), cmd));
			}
		}
	}
	return true;
}

void ConfigParser::register_handler(const std::string& cmd,
	ConfigActionHandler* handler)
{
	action_handlers[cmd] = handler;
}

void ConfigParser::unregister_handler(const std::string& cmd)
{
	action_handlers[cmd] = 0;
}

void ConfigParser::evaluate_backticks(std::vector<std::string>& tokens)
{
	for (auto& token : tokens) {
		token = evaluate_backticks(token);
	}
}

/* Note that this function not only finds next backtick that isn't prefixed
 * with a backslash, but also un-escapes all the escaped backticks it finds in
 * the process */
std::string::size_type find_non_escaped_backtick(std::string& input,
	const std::string::size_type startpos)
{
	if (startpos == std::string::npos)
		return startpos;

	std::string::size_type result = startpos;
	result = input.find_first_of("`", result);

	while (result != std::string::npos && result > 0 &&
		input[result - 1] == '\\') {
		// remove the backslash
		input.erase(result - 1, 1);

		// *not* adding one to start position as we already shortened
		// the input by one character
		result = input.find_first_of("`", result);
	}

	return result;
}

std::string ConfigParser::evaluate_backticks(std::string token)
{
	std::string::size_type pos1 = find_non_escaped_backtick(token, 0);
	std::string::size_type pos2 =
		find_non_escaped_backtick(token, pos1 + 1);

	while (pos1 != std::string::npos && pos2 != std::string::npos) {
		std::string cmd = token.substr(pos1 + 1, pos2 - pos1 - 1);
		token.erase(pos1, pos2 - pos1 + 1);
		std::string result = Utils::get_command_output(cmd);
		Utils::trim_end(result);
		token.insert(pos1, result);

		pos1 = find_non_escaped_backtick(
			token, pos1 + result.length() + 1);
		pos2 = find_non_escaped_backtick(token, pos1 + 1);
	}

	return token;
}

} // namespace newsboat

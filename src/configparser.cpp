#include <configparser.h>
#include <tagsouppullparser.h>
#include <exceptions.h>
#include <utils.h>
#include <strprintf.h>
#include <logger.h>
#include <fstream>
#include <config.h>
#include <cstdlib>
#include <cstring>

#include <sys/types.h>
#include <pwd.h>

#include <cstring>
#include <cstdlib>


namespace newsbeuter {

configparser::configparser() {
	register_handler("include", this);
}

configparser::~configparser() { }

void configparser::handle_action(const std::string& action, const std::vector<std::string>& params) {
	/*
	 * configparser also acts as config_action_handler to implement to recursive "include" command.
	 */
	if (action == "include") {
		if (params.size() < 1) {
			throw confighandlerexception(action_handler_status::TOO_FEW_PARAMS);
		}

		if (!this->parse(utils::resolve_tilde(params[0])))
			throw confighandlerexception(action_handler_status::FILENOTFOUND);
	} else
		throw confighandlerexception(action_handler_status::INVALID_COMMAND);
}

bool configparser::parse(const std::string& filename, bool double_include) {
	/*
	 * this function parses a config file.
	 *
	 * First, it checks whether this file has already been included, and if not,
	 * it does the following:
	 *   - open the file
	 *   - read if line by line
	 *   - tokenize every line
	 *   - if there is at least one token, look up a registered config_action_handler to handle to command (which is specified in the first token)
	 *   - hand over the tokenize results to the config_action_handler
	 *   - if an error happens, react accordingly.
	 */
	if (!double_include && included_files.find(filename) != included_files.end()) {
		LOG(level::WARN, "configparser::parse: file %s has already been included", filename);
		return true;
	}
	included_files.insert(included_files.begin(), filename);

	unsigned int linecounter = 1;
	std::ifstream f(filename.c_str());
	std::string line;
	std::getline(f,line);
	if (!f.is_open()) {
		LOG(level::WARN, "configparser::parse: file %s couldn't be opened", filename);
		return false;
	}
	while (f.is_open() && !f.eof()) {
		LOG(level::DEBUG,"configparser::parse: tokenizing %s",line);
		std::vector<std::string> tokens = utils::tokenize_quoted(line);
		if (!tokens.empty()) {
			std::string cmd = tokens[0];
			config_action_handler * handler = action_handlers[cmd];
			if (handler) {
				tokens.erase(tokens.begin()); // delete first element
				try {
					evaluate_backticks(tokens);
					handler->handle_action(cmd,tokens);
				} catch (const confighandlerexception& e) {
					throw configexception(strprintf::fmt(_("Error while processing command `%s' (%s line %u): %s"), line, filename, linecounter, e.what()));
				}
			} else {
				throw configexception(strprintf::fmt(_("unknown command `%s'"), cmd));
			}
		}
		getline(f,line);
		++linecounter;
	}
	return true;
}

void configparser::register_handler(const std::string& cmd, config_action_handler * handler) {
	action_handlers[cmd] = handler;
}

void configparser::unregister_handler(const std::string& cmd) {
	action_handlers[cmd] = 0;
}

void configparser::evaluate_backticks(std::vector<std::string>& tokens) {
	for (auto& token : tokens) {
		token = evaluate_backticks(token);
	}
}

/* Note that this function not only finds next backtick that isn't prefixed
 * with a backslash, but also un-escapes all the escaped backticks it finds in
 * the process */
std::string::size_type find_non_escaped_backtick(
		std::string& input,
		const std::string::size_type startpos)
{
	if (startpos == std::string::npos)
		return startpos;

	std::string::size_type result = startpos;
	result = input.find_first_of("`", result);

	while (
			result != std::string::npos &&
			result > 0 &&
			input[result-1] == '\\')
	{
		// remove the backslash
		input.erase(result-1, 1);

		// *not* adding one to start position as we already shortened the input
		// by one character
		result = input.find_first_of("`", result);
	}

	return result;
}

std::string configparser::evaluate_backticks(std::string token) {
	std::string::size_type pos1 = find_non_escaped_backtick(token, 0);
	std::string::size_type pos2 = find_non_escaped_backtick(token, pos1+1);

	while (pos1 != std::string::npos && pos2 != std::string::npos) {
		std::string cmd = token.substr(pos1+1, pos2-pos1-1);
		token.erase(pos1, pos2-pos1+1);
		std::string result = utils::get_command_output(cmd);
		utils::trim_end(result);
		token.insert(pos1, result);

		pos1 = find_non_escaped_backtick(token, pos1+result.length()+1);
		pos2 = find_non_escaped_backtick(token, pos1+1);
	}

	return token;
}

}

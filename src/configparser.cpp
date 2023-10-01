#include "configparser.h"

#include <algorithm>
#include <pwd.h>
#include <sys/types.h>

#include "config.h"
#include "configexception.h"
#include "confighandlerexception.h"
#include "filepath.h"
#include "logger.h"
#include "strprintf.h"
#include "utils.h"

namespace newsboat {

ConfigParser::ConfigParser()
{
	register_handler("include", *this);
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
			throw ConfigHandlerException(ActionHandlerStatus::TOO_FEW_PARAMS);
		}

		const Filepath tilde_expanded = utils::resolve_tilde(params[0]);
		const Filepath current_fpath = included_files.back().clone();
		if (!this->parse_file(utils::resolve_relative(current_fpath, tilde_expanded))) {
			throw ConfigHandlerException(ActionHandlerStatus::FILENOTFOUND);
		}
	} else {
		throw ConfigHandlerException(ActionHandlerStatus::INVALID_COMMAND);
	}
}

bool ConfigParser::parse_file(const Filepath& tmp_filename)
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

	// It would be nice if this function was only give absolute paths, but the
	// tests are easier as relative paths
	const Filepath filename = (tmp_filename.to_locale_string().front() ==
			NEWSBEUTER_PATH_SEP) ?
		tmp_filename.clone() :
		Filepath::from_locale_string(utils::getcwd().to_locale_string() +
			NEWSBEUTER_PATH_SEP + tmp_filename.to_locale_string());

	if (std::find(included_files.begin(), included_files.end(),
			filename) != included_files.end()) {
		LOG(Level::WARN,
			"ConfigParser::parse_file: file %s has already been "
			"included",
			filename);
		return true;
	}
	included_files.push_back(filename.clone());

	const auto lines = utils::read_text_file(filename);
	if (!lines) {
		const auto error = lines.error();

		switch (error.kind) {
		case utils::ReadTextFileErrorKind::CantOpen:
			LOG(Level::WARN,
				"ConfigParser::parse_file: file %s couldn't be opened",
				filename);
			return false;

		case utils::ReadTextFileErrorKind::LineError:
			throw ConfigException(error.message);
		}
	}

	unsigned int linecounter = 0;
	std::string multi_line_buffer{};
	for (const auto& line : lines.value()) {
		linecounter++;
		if (!line.empty() && line.back() == '\\') {
			multi_line_buffer.append(line.substr(0, line.size()-1));
		} else {
			const std::string location = strprintf::fmt(_("%s line %u"), filename, linecounter);
			if (!multi_line_buffer.empty()) {
				multi_line_buffer.append(line);
				LOG(Level::DEBUG, "ConfigParser::parse_file: tokenizing %s", multi_line_buffer);
				parse_line(multi_line_buffer, location);
				multi_line_buffer.clear();
			} else {
				LOG(Level::DEBUG, "ConfigParser::parse_file: tokenizing %s", line);
				parse_line(line, location);
			}
		}
	}
	included_files.pop_back();
	return true;
}

void ConfigParser::parse_line(const std::string& line,
	const std::string& location)
{
	auto stripped = utils::strip_comments(line);
	auto evaluated = evaluate_backticks(std::move(stripped));
	const auto token = utils::extract_token_quoted(evaluated);
	if (token.has_value()) {
		const std::string cmd = token.value();
		const std::string params = evaluated;

		if (action_handlers.count(cmd) < 1) {
			throw ConfigException(strprintf::fmt(_("unknown command `%s'"), cmd));
		}
		ConfigActionHandler& handler = action_handlers.at(cmd);
		try {
			handler.handle_action(cmd, params);
		} catch (const ConfigHandlerException& e) {
			throw ConfigException(strprintf::fmt(
					_("Error while processing command `%s' (%s): %s"),
					line,
					location,
					e.what()));
		}
	}
}

void ConfigParser::register_handler(const std::string& cmd,
	ConfigActionHandler& handler)
{
	action_handlers.erase(cmd);
	action_handlers.insert({cmd, handler});
}

/* Note that this function not only finds next backtick that isn't prefixed
 * with a backslash, but also un-escapes all the escaped backticks it finds in
 * the process */
std::string::size_type find_non_escaped_backtick(std::string& input,
	const std::string::size_type startpos)
{
	if (startpos == std::string::npos) {
		return startpos;
	}

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
		const std::string cmd = token.substr(pos1 + 1, pos2 - pos1 - 1);
		token.erase(pos1, pos2 - pos1 + 1);
		std::string result = utils::get_command_output(cmd);
		utils::trim_end(result);
		token.insert(pos1, result);

		pos1 = find_non_escaped_backtick(
				token, pos1 + result.length() + 1);
		pos2 = find_non_escaped_backtick(token, pos1 + 1);
	}

	return token;
}

} // namespace newsboat

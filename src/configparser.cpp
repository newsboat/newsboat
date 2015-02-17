#include <configparser.h>
#include <tagsouppullparser.h>
#include <exceptions.h>
#include <utils.h>
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
			throw confighandlerexception(AHS_TOO_FEW_PARAMS);
		}

		if (!this->parse(utils::resolve_tilde(params[0])))
			throw confighandlerexception(AHS_FILENOTFOUND);
	} else
		throw confighandlerexception(AHS_INVALID_COMMAND);
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
		LOG(LOG_WARN, "configparser::parse: file %s has already been included", filename.c_str());
		return true;
	}
	included_files.insert(included_files.begin(), filename);

	unsigned int linecounter = 1;
	std::ifstream f(filename.c_str());
	std::string line;
	getline(f,line);
	if (!f.is_open()) {
		LOG(LOG_WARN, "configparser::parse: file %s couldn't be opened", filename.c_str());
		return false;
	}
	while (f.is_open() && !f.eof()) {
		LOG(LOG_DEBUG,"configparser::parse: tokenizing %s",line.c_str());
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
					throw configexception(utils::strprintf(_("Error while processing command `%s' (%s line %u): %s"), line.c_str(), filename.c_str(), linecounter, e.what()));
				}
			} else {
				throw configexception(utils::strprintf(_("unknown command `%s'"), cmd.c_str()));
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

std::string configparser::evaluate_backticks(std::string token) {
	std::string::size_type pos1 = token.find_first_of("`", 0);
	std::string::size_type pos2 = 0;
	while (pos1 != std::string::npos && pos2 != std::string::npos) {
		pos2 = token.find_first_of("`", pos1+1);
		if (pos2 != std::string::npos) {
			std::string cmd = token.substr(pos1+1, pos2-pos1-1);
			token.erase(pos1, pos2-pos1+1);
			std::string result = utils::get_command_output(cmd);
			utils::trim_end(result);
			token.insert(pos1, result);
			pos1 = token.find_first_of("`", pos1+result.length()+1);
		}
	}
	return token;
}

}

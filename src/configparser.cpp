#include <configparser.h>
#include <xmlpullparser.h>
#include <exceptions.h>
#include <utils.h>
#include <logger.h>
#include <fstream>
#include <sstream>
#include <config.h>

#include <sys/types.h>
#include <pwd.h>


namespace newsbeuter {

configparser::configparser() {
	register_handler("include", this);
}

configparser::~configparser() { }

action_handler_status configparser::handle_action(const std::string& action, const std::vector<std::string>& params) {
	/*
	 * configparser also acts as config_action_handler to implement to recursive "include" command.
	 */
	if (action == "include") {
		if (params.size() < 1) {
			return AHS_TOO_FEW_PARAMS;
		}

		const char * homedir;
		std::string filepath;

		if (!(homedir = ::getenv("HOME"))) {
			struct passwd * spw = ::getpwuid(::getuid());
			if (spw) {
					homedir = spw->pw_dir;
			} else {
					homedir = "";
			}
		}

		if (strcmp(homedir,"")!=0 && params[0].substr(0,2) == "~/") {
			filepath.append(homedir);
			filepath.append(1,'/');
			filepath.append(params[0].substr(2,params[0].length()-2));
		} else {
			filepath.append(params[0]);
		}
		if (this->parse(filepath)) {
			return AHS_OK;
		} else {
			return AHS_FILENOTFOUND;
		}
	}
	return AHS_INVALID_COMMAND;
}

bool configparser::parse(const std::string& filename) {
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
	char buf[1024];
	if (included_files.find(filename) != included_files.end()) {
		GetLogger().log(LOG_WARN, "configparser::parse: file %s has already been included", filename.c_str());
		return true;
	}
	included_files.insert(included_files.begin(), filename);

	unsigned int linecounter = 1;
	std::ifstream f(filename.c_str());
	std::string line;
	getline(f,line);
	if (!f.is_open()) {
		GetLogger().log(LOG_WARN, "configparser::parse: file %s couldn't be opened", filename.c_str());
		return false;
	}
	while (f.is_open() && !f.eof()) {
		GetLogger().log(LOG_DEBUG,"configparser::parse: tokenizing %s",line.c_str());
		std::vector<std::string> tokens = utils::tokenize_quoted(line);
		if (tokens.size() > 0) {
			std::string cmd = tokens[0];
			config_action_handler * handler = action_handlers[cmd];
			if (handler) {
				tokens.erase(tokens.begin()); // delete first element
				action_handler_status status = handler->handle_action(cmd,tokens);
				if (status != AHS_OK) {
					char * errmsg = NULL;
					switch (status) {
						case AHS_INVALID_PARAMS:
							errmsg = _("invalid parameters.");
							break;
						case AHS_TOO_FEW_PARAMS:
							errmsg = _("too few parameters.");
							break;
						case AHS_INVALID_COMMAND:
							errmsg = _("unknown command (bug).");
							break;
						case AHS_FILENOTFOUND:
							errmsg = _("file couldn't be opened.");
							break;
						default:
							errmsg = _("unknown error (bug).");
					}
					snprintf(buf, sizeof(buf), _("Error while processing command `%s' (%s line %u): %s"), line.c_str(), filename.c_str(), linecounter, errmsg);
					throw configexception(buf);
				}
			} else {
				char buf[1024];
				snprintf(buf, sizeof(buf), _("unknown command `%s'"), cmd.c_str());
				throw configexception(buf);
			}
		}
		getline(f,line);
		++linecounter;
	}
	return true;
}

void configparser::register_handler(const std::string& cmd, config_action_handler * handler) {
	GetLogger().log(LOG_DEBUG,"configparser::register_handler: cmd = %s handler = %p", cmd.c_str(), handler);
	action_handlers[cmd] = handler;
}

void configparser::unregister_handler(const std::string& cmd) {
	GetLogger().log(LOG_DEBUG,"configparser::unregister_handler: cmd = %s", cmd.c_str());
	action_handlers[cmd] = 0;
}

}

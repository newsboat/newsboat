#include <configparser.h>
#include <xmlpullparser.h>
#include <exceptions.h>
#include <utils.h>
#include <logger.h>
#include <fstream>
#include <sstream>

namespace newsbeuter {

configparser::configparser(const char * file) : filename(file) { }

configparser::~configparser() { }

void configparser::parse() {
	unsigned int linecounter = 1;
	std::fstream f(filename.c_str());
	std::string line;
	getline(f,line);
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
					std::ostringstream os;
					os << "Error while processing command `" << cmd << "' (" << filename << " line " << linecounter << "): ";
					if (status == AHS_INVALID_PARAMS) {
						os << "invalid parameters.";
					} else if (status == AHS_TOO_FEW_PARAMS) {
						os << "too few parameters.";
					} else if (status == AHS_INVALID_COMMAND) {
						os << "unknown command (bug).";
					} else {
						os << "unknown error (bug).";
					}
					throw configexception(os.str());
				}
			} else {
				std::ostringstream os;
				os << "unknown command `" << cmd << "'";
				throw configexception(os.str());
			}
		}
		getline(f,line);	
		++linecounter;
	}
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

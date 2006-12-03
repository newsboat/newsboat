#include <configparser.h>
#include <xmlpullparser.h>
#include <exceptions.h>
#include <fstream>
#include <sstream>

namespace noos {

configparser::configparser(const char * file) : filename(file) { }

configparser::~configparser() { }

void configparser::parse() {
	std::fstream f(filename.c_str());
	std::string line;
	getline(f,line);
	while (!f.eof()) {
		std::vector<std::string> tokens = xmlpullparser::tokenize(line); // TODO: write other tokenizer
		if (tokens.size() > 0) {
			std::string cmd = tokens[0];
			config_action_handler * handler = action_handlers[cmd];
			if (handler) {
				tokens.erase(tokens.begin()); // delete first element
				action_handler_status status = handler->handle_action(cmd,tokens);
				if (status != AHS_OK) {
					std::ostringstream os;
					os << "Error while processing command `" << cmd << "': ";
					if (status == AHS_INVALID_PARAMS) {
						os << "invalid parameters.";
					} else if (status == AHS_TOO_FEW_PARAMS) {
						os << "too few parameters.";
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
	}
}

void configparser::register_handler(const std::string& cmd, config_action_handler * handler) {
	action_handlers[cmd] = handler;
}

void configparser::unregister_handler(const std::string& cmd) {
	action_handlers[cmd] = 0;
}

}

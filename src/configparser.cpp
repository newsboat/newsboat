#include <configparser.h>

using namespace noos;

configparser::configparser(const char * file) filename(file) { }

configparser::~configparser() { }

void configparser::parse() {
	// TODO: open file, read line by line and tokenize
}

void register_handler(const std::string& cmd, config_action_handler * handler) {
	action_handlers[cmd] = handler;
}

void unregister_handler(const std::string& cmd) {
	action_handlers[cmd] = 0;
}

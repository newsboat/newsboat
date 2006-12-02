#ifndef NOOS_CONFIGPARSER__H
#define NOOS_CONFIGPARSER__H

#include <vector>
#include <string>

namespace noos {

	enum action_handler_status { AHS_OK = 0, AHS_INVALID_PARAMS, AHS_TOO_FEW_PARAMS };

	struct config_action_handler {
		virtual action_handler_status handle_action(const std::string& action, const std::vector<std::string>& params) = 0;
	};

	class configparser {
		public:
			configparser(const char * file);
			~configparser();
			void parse();
			void register_action_handler(const std::string& action, config_action_handler * handler);
			int get_numeric_config_value(const std::string& key);
			std::string get_config_value(const std::string& key);
			void register_handler(const std::string& cmd, config_action_handler * handler);
			void unregister_handler(const std::string& cmd);
		private:
			std::string filename;
			std::vector<std::vector<std::string> > parsed_content;
			std::map<std::string,config_action_handler *> action_handlers;
	};

}

#endif

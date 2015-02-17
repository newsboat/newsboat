#ifndef NEWSBEUTER_CONFIGPARSER__H
#define NEWSBEUTER_CONFIGPARSER__H

#include <vector>
#include <string>
#include <map>
#include <set>

namespace newsbeuter {

	enum action_handler_status { AHS_OK = 0, AHS_INVALID_PARAMS, AHS_TOO_FEW_PARAMS, AHS_INVALID_COMMAND, AHS_FILENOTFOUND };

	struct config_action_handler {
		virtual void handle_action(const std::string& action, const std::vector<std::string>& params) = 0;
		virtual void dump_config(std::vector<std::string>& config_output) = 0;
		config_action_handler() { }
		virtual ~config_action_handler() { }
	};

	class configparser : public config_action_handler {
		public:
			configparser();
			virtual ~configparser();
			void register_handler(const std::string& cmd, config_action_handler * handler);
			void unregister_handler(const std::string& cmd);
			virtual void handle_action(const std::string& action, const std::vector<std::string>& params);
			virtual void dump_config(std::vector<std::string>& ) { /* nothing because configparser itself only handles include */ }
			bool parse(const std::string& filename, bool double_include = true);
			static std::string evaluate_backticks(std::string token);
		private:
			void evaluate_backticks(std::vector<std::string>& tokens);
			static std::string evaluate_cmd(const std::string& cmd);
			std::vector<std::vector<std::string>> parsed_content;
			std::map<std::string,config_action_handler *> action_handlers;
			std::set<std::string> included_files;
	};

	class null_config_action_handler : public config_action_handler {
		public:
			null_config_action_handler() { }
			virtual ~null_config_action_handler() { }
			virtual void handle_action(const std::string& , const std::vector<std::string>& ) { }
			virtual void dump_config(std::vector<std::string>& ) { }
	};

}

#endif

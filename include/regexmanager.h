#ifndef REGEXMANAGER__H
#define REGEXMANAGER__H

#include <configparser.h>
#include <vector>
#include <sys/types.h>
#include <regex.h>

namespace newsbeuter {

class regexmanager : public config_action_handler {
	public:
		regexmanager();
		~regexmanager();
		virtual action_handler_status handle_action(const std::string& action, const std::vector<std::string>& params);
		inline std::vector<std::string>& get_attrs() { return colors; }
		inline std::vector<regex_t *>& get_regexes() { return regexes; }
		void quote_and_highlight(std::string& str);
	private:
		std::vector<regex_t *> regexes;
		std::vector<std::string> colors;
};

}

#endif

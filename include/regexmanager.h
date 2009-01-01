#ifndef REGEXMANAGER__H
#define REGEXMANAGER__H

#include <configparser.h>
#include <vector>
#include <sys/types.h>
#include <regex.h>
#include <utility>

namespace newsbeuter {

class regexmanager : public config_action_handler {
	public:
		regexmanager();
		~regexmanager();
		virtual action_handler_status handle_action(const std::string& action, const std::vector<std::string>& params);
		void quote_and_highlight(std::string& str, const std::string& location);
		void remove_last_regex(const std::string& location);
	private:
		typedef std::pair<std::vector<regex_t *>, std::vector<std::string> > rc_pair;
		std::map<std::string, rc_pair> locations;
	public:
		inline std::vector<std::string>& get_attrs(const std::string& loc) { return locations[loc].second; }
		inline std::vector<regex_t *>& get_regexes(const std::string& loc) { return locations[loc].first; }
};

}

#endif

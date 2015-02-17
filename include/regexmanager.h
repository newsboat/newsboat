#ifndef REGEXMANAGER__H
#define REGEXMANAGER__H

#include <configparser.h>
#include <vector>
#include <sys/types.h>
#include <regex.h>
#include <matcher.h>
#include <utility>
#include <memory>

namespace newsbeuter {

class regexmanager : public config_action_handler {
	public:
		regexmanager();
		~regexmanager();
		virtual void handle_action(const std::string& action, const std::vector<std::string>& params);
		virtual void dump_config(std::vector<std::string>& config_output);
		void quote_and_highlight(std::string& str, const std::string& location);
		void remove_last_regex(const std::string& location);
		int article_matches(matchable * item);
	private:
		typedef std::pair<std::vector<regex_t *>, std::vector<std::string>> rc_pair;
		std::map<std::string, rc_pair> locations;
		std::vector<std::string> cheat_store_for_dump_config;
		std::vector<std::pair<std::shared_ptr<matcher>, int>> matchers;
		std::string extract_initial_marker(const std::string& str);
	public:
		inline std::vector<std::string>& get_attrs(const std::string& loc) { return locations[loc].second; }
		inline std::vector<regex_t *>& get_regexes(const std::string& loc) { return locations[loc].first; }
};

}

#endif

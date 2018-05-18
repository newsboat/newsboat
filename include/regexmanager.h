#ifndef NEWSBOAT_REGEXMANAGER_H_
#define NEWSBOAT_REGEXMANAGER_H_

#include <memory>
#include <regex.h>
#include <sys/types.h>
#include <utility>
#include <vector>

#include "configparser.h"
#include "matcher.h"

namespace newsboat {

class regexmanager : public config_action_handler {
public:
	regexmanager();
	~regexmanager() override;
	void handle_action(const std::string& action,
		const std::vector<std::string>& params) override;
	void dump_config(std::vector<std::string>& config_output) override;
	void quote_and_highlight(std::string& str, const std::string& location);
	void remove_last_regex(const std::string& location);
	int article_matches(matchable* item);

private:
	typedef std::pair<std::vector<regex_t*>, std::vector<std::string>>
		rc_pair;
	std::map<std::string, rc_pair> locations;
	std::vector<std::string> cheat_store_for_dump_config;
	std::vector<std::pair<std::shared_ptr<matcher>, int>> matchers;
	std::string extract_initial_marker(const std::string& str);

public:
	std::vector<std::string>& get_attrs(const std::string& loc)
	{
		return locations[loc].second;
	}
	std::vector<regex_t*>& get_regexes(const std::string& loc)
	{
		return locations[loc].first;
	}
};

} // namespace newsboat

#endif /* NEWSBOAT_REGEXMANAGER_H_ */

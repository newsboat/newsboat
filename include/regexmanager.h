#ifndef NEWSBOAT_REGEXMANAGER_H_
#define NEWSBOAT_REGEXMANAGER_H_

#include <map>
#include <memory>
#include <regex.h>
#include <string>
#include <sys/types.h>
#include <utility>
#include <vector>

#include "configactionhandler.h"
#include "matcher.h"
#include "regexowner.h"
#include "stflrichtext.h"

namespace Newsboat {

class RegexManager : public ConfigActionHandler {
public:
	RegexManager();
	void handle_action(const std::string& action,
		const std::vector<std::string>& params) override;
	void dump_config(std::vector<std::string>& config_output) const override;
	void quote_and_highlight(StflRichText& stflString, const std::string& location);
	void remove_last_regex(const std::string& location);
	int article_matches(Matchable* item);
	int feed_matches(Matchable* feed);
	std::string get_attrs_stfl_string(const std::string& location, bool hasFocus);

private:
	typedef std::vector<std::pair<std::shared_ptr<Regex>, std::string>>
		RegexStyleVector;
	std::map<std::string, RegexStyleVector> locations;
	std::vector<std::string> cheat_store_for_dump_config;
	std::vector<std::pair<std::shared_ptr<Matcher>, int>> matchers_article;
	std::vector<std::pair<std::shared_ptr<Matcher>, int>> matchers_feed;

	void handle_highlight_action(const std::vector<std::string>& params);
	void handle_highlight_item_action(const std::string& action,
		const std::vector<std::string>& params);
};

} // namespace Newsboat

#endif /* NEWSBOAT_REGEXMANAGER_H_ */

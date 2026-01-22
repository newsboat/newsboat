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
#include "dialog.h"
#include "matcher.h"
#include "regexowner.h"
#include "stflrichtext.h"
#include "textstyle.h"

namespace newsboat {

class RegexManager : public ConfigActionHandler {
public:
	void handle_action(const std::string& action,
		const std::vector<std::string>& params) override;
	void dump_config(std::vector<std::string>& config_output) const override;
	void quote_and_highlight(StflRichText& stflString, Dialog location) const;
	void remove_last_regex(Dialog location);
	int article_matches(Matchable* item) const;
	int feed_matches(Matchable* feed) const;
	std::string get_attrs_stfl_string(Dialog location, bool hasFocus) const;

private:
	using RegexStyleVector = std::vector<std::pair<std::shared_ptr<Regex>, TextStyle>>;
	std::map<Dialog, RegexStyleVector> locations;
	std::vector<std::string> cheat_store_for_dump_config;
	std::vector<std::pair<std::shared_ptr<Matcher>, int>> matchers_article;
	std::vector<std::pair<std::shared_ptr<Matcher>, int>> matchers_feed;

	void handle_highlight_action(const std::vector<std::string>& params);
	void handle_highlight_item_action(const std::string& action,
		const std::vector<std::string>& params);
};

} // namespace newsboat

#endif /* NEWSBOAT_REGEXMANAGER_H_ */

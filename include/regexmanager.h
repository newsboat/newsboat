#ifndef NEWSBOAT_REGEXMANAGER_H_
#define NEWSBOAT_REGEXMANAGER_H_

#include <memory>
#include <regex>
#include <regex.h>
#include <sys/types.h>
#include <utility>
#include <vector>

#include "configparser.h"
#include "matcher.h"

namespace newsboat {

class RegexManager : public ConfigActionHandler {
public:
	RegexManager();
	~RegexManager() override;
	void handle_action(const std::string& action,
		const std::vector<std::string>& params) override;
	void dump_config(std::vector<std::string>& config_output) override;
	void quote_and_highlight(std::string& str, const std::string& location);
	void remove_last_regex(const std::string& location);
	int article_matches(Matchable* item);
	std::map<size_t, std::string> extract_style_tags(std::string& str);
	void insert_style_tags(std::string& str, std::map<size_t, std::string>& tags);
	void merge_style_tag(std::map<size_t, std::string>& tags,
		const std::string& tag,
		size_t start, size_t end);

private:
	typedef std::pair<std::vector<regex_t*>, std::vector<std::string>>
		RcPair;
	std::map<std::string, RcPair> locations;
	std::vector<std::string> cheat_store_for_dump_config;
	std::vector<std::pair<std::shared_ptr<Matcher>, int>> matchers;

	void handle_highlight_action(const std::vector<std::string>& params);
	void handle_highlight_article_action(
		const std::vector<std::string>& params);

public:
	std::vector<std::string>& get_attrs(const std::string& loc)
	{
		return locations[loc].second;
	}
};

} // namespace newsboat

#endif /* NEWSBOAT_REGEXMANAGER_H_ */

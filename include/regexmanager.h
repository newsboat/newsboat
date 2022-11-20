#ifndef NEWSBOAT_REGEXMANAGER_H_
#define NEWSBOAT_REGEXMANAGER_H_

#include <map>
#include <memory>
#include <regex.h>
#include <regex>
#include <sys/types.h>
#include <utility>
#include <vector>

#include "configactionhandler.h"
#include "matcher.h"
#include "regexowner.h"
#include "utf8string.h"

namespace newsboat {

class RegexManager : public ConfigActionHandler {
public:
	RegexManager();
	void handle_action(const Utf8String& action,
		const std::vector<Utf8String>& params) override;
	void dump_config(std::vector<Utf8String>& config_output) const override;
	void quote_and_highlight(Utf8String& str, const Utf8String& location);
	void remove_last_regex(const Utf8String& location);
	int article_matches(Matchable* item);
	int feed_matches(Matchable* feed);
	std::map<size_t, Utf8String> extract_style_tags(Utf8String& str);
	void insert_style_tags(Utf8String& str, std::map<size_t, Utf8String>& tags);
	void merge_style_tag(std::map<size_t, Utf8String>& tags,
		const Utf8String& tag,
		size_t start, size_t end);
	Utf8String get_attrs_stfl_string(const Utf8String& location, bool hasFocus);

private:
	typedef std::vector<std::pair<std::shared_ptr<Regex>, Utf8String>>
		RegexStyleVector;
	std::map<Utf8String, RegexStyleVector> locations;
	std::vector<Utf8String> cheat_store_for_dump_config;
	std::vector<std::pair<std::shared_ptr<Matcher>, int>> matchers_article;
	std::vector<std::pair<std::shared_ptr<Matcher>, int>> matchers_feed;

	void handle_highlight_action(const std::vector<Utf8String>& params);
	void handle_highlight_item_action(const Utf8String& action,
		const std::vector<Utf8String>& params);
};

} // namespace newsboat

#endif /* NEWSBOAT_REGEXMANAGER_H_ */

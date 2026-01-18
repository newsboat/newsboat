#include "regexmanager.h"

#include "config.h"
#include "confighandlerexception.h"
#include "configparser.h"
#include "dialog.h"
#include "logger.h"
#include "stflrichtext.h"
#include "strprintf.h"
#include "utils.h"

namespace newsboat {

void RegexManager::dump_config(std::vector<std::string>& config_output) const
{
	for (const auto& foo : cheat_store_for_dump_config) {
		config_output.push_back(foo);
	}
}

void RegexManager::handle_action(const std::string& action,
	const std::vector<std::string>& params)
{
	if (action == "highlight") {
		handle_highlight_action(params);
	} else if (action == "highlight-article" || action == "highlight-feed") {
		handle_highlight_item_action(action, params);
	} else {
		throw ConfigHandlerException(
			ActionHandlerStatus::INVALID_COMMAND);
	}
	std::string line = action;
	for (const auto& param : params) {
		line.append(" ");
		line.append(utils::quote(param));
	}
	cheat_store_for_dump_config.push_back(line);
}

int RegexManager::article_matches(Matchable* item) const
{
	for (const auto& Matcher : matchers_article) {
		if (Matcher.first->matches(item)) {
			return Matcher.second;
		}
	}
	return -1;
}

int RegexManager::feed_matches(Matchable* feed) const
{
	for (const auto& Matcher : matchers_feed) {
		if (Matcher.first->matches(feed)) {
			return Matcher.second;
		}
	}
	return -1;
}

void RegexManager::remove_last_regex(Dialog location)
{
	const auto location_regexes = locations.find(location);
	if (location_regexes == locations.end()) {
		return;
	}

	auto& regexes = location_regexes->second;
	if (regexes.empty()) {
		return;
	}

	regexes.pop_back();
}

void RegexManager::quote_and_highlight(StflRichText& stflString, Dialog location) const
{
	const auto location_regexes = locations.find(location);
	if (location_regexes == locations.end()) {
		return;
	}

	const auto& regexes = location_regexes->second;

	const std::string text = stflString.plaintext();

	for (unsigned int i = 0; i < regexes.size(); ++i) {
		const auto& regex = regexes[i].first;
		if (regex == nullptr) {
			continue;
		}
		unsigned int offset = 0;
		int eflags = 0;
		while (offset < text.length()) {
			const auto matches = regex->matches(text.substr(offset), 1, eflags);
			eflags |= REG_NOTBOL; // Don't match beginning-of-line operator (^) in following checks
			if (matches.empty()) {
				break;
			}
			const auto& match = matches[0];
			if (match.first != match.second) {
				const std::string marker = strprintf::fmt("<%u>", i);
				const int match_start = offset + match.first;
				const int match_end = offset + match.second;
				stflString.apply_style_tag(marker, match_start, match_end);
				offset = match_end;
			} else {
				offset++;
			}
		}
	}
}

void RegexManager::handle_highlight_action(const std::vector<std::string>&
	params)
{
	if (params.size() < 3) {
		throw ConfigHandlerException(ActionHandlerStatus::TOO_FEW_PARAMS);
	}

	std::string location_str = params[0];

	std::vector<Dialog> applicable_locations;
	if (location_str == "all") {
		applicable_locations = {
			Dialog::Article,
			Dialog::ArticleList,
			Dialog::FeedList,
		};
	} else {
		const auto location = dialog_from_name(location_str);
		if (location.has_value() &&
			(location.value() == Dialog::FeedList || location.value() == Dialog::ArticleList
				|| location.value() == Dialog::Article)) {
			applicable_locations.push_back(location.value());
		} else {
			throw ConfigHandlerException(strprintf::fmt(
					_("`%s' is not a valid context"), location_str));
		}
	}

	std::string errorMessage;
	auto regex = Regex::compile(params[1], REG_EXTENDED | REG_ICASE, errorMessage);
	if (regex == nullptr) {
		throw ConfigHandlerException(strprintf::fmt(
				_("`%s' is not a valid regular expression: %s"),
				params[1],
				errorMessage));
	}

	const std::string fgcolor = params[2];
	const std::string bgcolor =
		params.size() >= 4
		? params[3]
		: "default";
	const std::vector<std::string> attributes =
		params.size() >= 5
		? std::vector<std::string>(params.begin() + 4, params.end())
		: std::vector<std::string>();

	StflStyle stfl_style(fgcolor, bgcolor, attributes);

	std::shared_ptr<Regex> sharedRegex(std::move(regex));
	for (auto& l : applicable_locations) {
		locations[l].push_back({sharedRegex, stfl_style});
	}
}

void RegexManager::handle_highlight_item_action(const std::string& action,
	const std::vector<std::string>& params)
{
	if (params.size() < 3) {
		throw ConfigHandlerException(ActionHandlerStatus::TOO_FEW_PARAMS);
	}

	const std::string expr = params[0];
	const std::string fgcolor = params[1];
	const std::string bgcolor = params[2];
	const std::vector<std::string> attributes(params.begin() + 3, params.end());

	StflStyle stfl_style(fgcolor, bgcolor, attributes);

	std::shared_ptr<Matcher> m(new Matcher());
	if (!m->parse(params[0])) {
		throw ConfigHandlerException(strprintf::fmt(
				_("couldn't parse filter expression `%s': %s"),
				params[0],
				m->get_parse_error()));
	}

	if (action == "highlight-article") {
		int pos = locations[Dialog::ArticleList].size();
		locations[Dialog::ArticleList].push_back({nullptr, stfl_style});
		matchers_article.push_back(
			std::pair<std::shared_ptr<Matcher>, int>(m, pos));
	} else if (action == "highlight-feed") {
		int pos = locations[Dialog::FeedList].size();
		locations[Dialog::FeedList].push_back({nullptr, stfl_style});
		matchers_feed.push_back(
			std::pair<std::shared_ptr<Matcher>, int>(m, pos));
	} else {
		throw ConfigHandlerException(
			ActionHandlerStatus::INVALID_COMMAND);
	}
}

std::string RegexManager::get_attrs_stfl_string(Dialog location,
	bool hasFocus) const
{
	const auto location_regexes = locations.find(location);
	if (location_regexes == locations.end()) {
		return "";
	}

	const auto& text_styles = location_regexes->second;
	std::string attrstr;
	for (unsigned int i = 0; i < text_styles.size(); ++i) {
		const StflStyle& stfl_style = text_styles[i].second;
		const std::string stfl_style_string = stfl_style.get_stfl_style_string();
		attrstr.append(strprintf::fmt("@style_%u_normal:%s ", i, stfl_style_string));
		if (hasFocus) {
			attrstr.append(strprintf::fmt("@style_%u_focus:%s ", i, stfl_style_string));
		}
	}
	LOG(Level::DEBUG, "RegexManager::get_attrs_stfl_string(%s, %s): %s", dialog_name(location),
		hasFocus ? "true" : "false", attrstr);
	return attrstr;
}

RegexManager::StflStyle::StflStyle(const std::string& fgcolor, const std::string& bgcolor,
	const std::vector<std::string>& attributes)
	: fgcolor(fgcolor)
	, bgcolor(bgcolor)
	, attributes(attributes)
{
	if (!utils::is_valid_color(fgcolor)) {
		throw ConfigHandlerException(strprintf::fmt(
				_("`%s' is not a valid color"),
				fgcolor));
	}
	if (!utils::is_valid_color(bgcolor)) {
		throw ConfigHandlerException(strprintf::fmt(
				_("`%s' is not a valid color"),
				bgcolor));
	}

	for (const auto& attribute : attributes) {
		if (!utils::is_valid_attribute(attribute)) {
			throw ConfigHandlerException(
				strprintf::fmt(
					_("`%s' is not a valid attribute"),
					attribute));
		}
	}
}

std::string RegexManager::StflStyle::get_stfl_style_string() const
{
	std::string stfl_style;
	if (fgcolor != "default") {
		stfl_style.append("fg=" + fgcolor);
	}

	if (bgcolor != "default") {
		if (!stfl_style.empty()) {
			stfl_style.append(",");
		}
		stfl_style.append("bg=" + bgcolor);
	}

	for (const auto& attribute : attributes) {
		if (attribute != "default") {
			if (!stfl_style.empty()) {
				stfl_style.append(",");
			}
			stfl_style.append("attr=" + attribute);
		}
	}
	return stfl_style;
}

} // namespace newsboat

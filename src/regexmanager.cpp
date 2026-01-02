#include "regexmanager.h"

#include "config.h"
#include "confighandlerexception.h"
#include "configparser.h"
#include "logger.h"
#include "stflrichtext.h"
#include "strprintf.h"
#include "utils.h"

namespace newsboat {

RegexManager::RegexManager()
{
	// this creates the entries in the map. we need them there to have the
	// "all" location work.
	locations[Dialog::Article];
	locations[Dialog::ArticleList];
	locations[Dialog::FeedList];
}

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

int RegexManager::article_matches(Matchable* item)
{
	for (const auto& Matcher : matchers_article) {
		if (Matcher.first->matches(item)) {
			return Matcher.second;
		}
	}
	return -1;
}

int RegexManager::feed_matches(Matchable* feed)
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
	auto& regexes = locations[location];
	if (regexes.empty()) {
		return;
	}

	regexes.pop_back();
}

void RegexManager::quote_and_highlight(StflRichText& stflString, Dialog location)
{
	auto& regexes = locations[location];

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
	const auto location = dialog_from_name(location_str);

	std::vector<Dialog> applicable_locations;
	if (location_str == "all") {
		for (const auto& l : locations) {
			applicable_locations.push_back(l.first);
		}
	} else if (location.has_value() &&
		(location.value() == Dialog::FeedList || location.value() == Dialog::ArticleList
			|| location.value() == Dialog::Article)) {
		applicable_locations.push_back(location.value());
	} else {
		throw ConfigHandlerException(strprintf::fmt(
				_("`%s' is not a valid context"), location_str));
	}

	std::string errorMessage;
	auto regex = Regex::compile(params[1], REG_EXTENDED | REG_ICASE, errorMessage);
	if (regex == nullptr) {
		throw ConfigHandlerException(strprintf::fmt(
				_("`%s' is not a valid regular expression: %s"),
				params[1],
				errorMessage));
	}
	std::string colorstr;
	if (params[2] != "default") {
		colorstr.append("fg=");
		if (!utils::is_valid_color(params[2])) {
			throw ConfigHandlerException(strprintf::fmt(
					_("`%s' is not a valid color"),
					params[2]));
		}
		colorstr.append(params[2]);
	}
	if (params.size() > 3) {
		if (params[3] != "default") {
			if (colorstr.length() > 0) {
				colorstr.append(",");
			}
			colorstr.append("bg=");
			if (!utils::is_valid_color(params[3])) {
				throw ConfigHandlerException(
					strprintf::fmt(
						_("`%s' is not a valid "
							"color"),
						params[3]));
			}
			colorstr.append(params[3]);
		}
		for (unsigned int i = 4; i < params.size(); ++i) {
			if (params[i] != "default") {
				if (!colorstr.empty()) {
					colorstr.append(",");
				}
				colorstr.append("attr=");
				if (!utils::is_valid_attribute(
						params[i])) {
					throw ConfigHandlerException(
						strprintf::fmt(
							_("`%s' is not "
								"a valid "
								"attribute"),
							params[i]));
				}
				colorstr.append(params[i]);
			}
		}
	}
	std::shared_ptr<Regex> sharedRegex(std::move(regex));
	for (auto& l : applicable_locations) {
		LOG(Level::DEBUG,
			"RegexManager::handle_action: adding rx = %s colorstr = %s to location %s",
			params[1],
			colorstr,
			dialog_name(l));
		locations[l].push_back({sharedRegex, colorstr});
	}
}

void RegexManager::handle_highlight_item_action(const std::string& action,
	const std::vector<std::string>& params)
{
	if (params.size() < 3) {
		throw ConfigHandlerException(ActionHandlerStatus::TOO_FEW_PARAMS);
	}

	std::string expr = params[0];
	std::string fgcolor = params[1];
	std::string bgcolor = params[2];

	std::string colorstr;
	if (fgcolor != "default") {
		colorstr.append("fg=");
		if (!utils::is_valid_color(fgcolor)) {
			throw ConfigHandlerException(strprintf::fmt(
					_("`%s' is not a valid color"),
					fgcolor));
		}
		colorstr.append(fgcolor);
	}
	if (bgcolor != "default") {
		if (!colorstr.empty()) {
			colorstr.append(",");
		}
		colorstr.append("bg=");
		if (!utils::is_valid_color(bgcolor)) {
			throw ConfigHandlerException(strprintf::fmt(
					_("`%s' is not a valid color"),
					bgcolor));
		}
		colorstr.append(bgcolor);
	}

	for (unsigned int i = 3; i < params.size(); i++) {
		if (params[i] != "default") {
			if (!colorstr.empty()) {
				colorstr.append(",");
			}
			colorstr.append("attr=");
			if (!utils::is_valid_attribute(params[i])) {
				throw ConfigHandlerException(
					strprintf::fmt(
						_("`%s' is not a valid "
							"attribute"),
						params[i]));
			}
			colorstr.append(params[i]);
		}
	}

	std::shared_ptr<Matcher> m(new Matcher());
	if (!m->parse(params[0])) {
		throw ConfigHandlerException(strprintf::fmt(
				_("couldn't parse filter expression `%s': %s"),
				params[0],
				m->get_parse_error()));
	}

	if (action == "highlight-article") {
		int pos = locations[Dialog::ArticleList].size();
		locations[Dialog::ArticleList].push_back({nullptr, colorstr});
		matchers_article.push_back(
			std::pair<std::shared_ptr<Matcher>, int>(m, pos));
	} else if (action == "highlight-feed") {
		int pos = locations[Dialog::FeedList].size();
		locations[Dialog::FeedList].push_back({nullptr, colorstr});
		matchers_feed.push_back(
			std::pair<std::shared_ptr<Matcher>, int>(m, pos));
	} else {
		throw ConfigHandlerException(
			ActionHandlerStatus::INVALID_COMMAND);
	}
}

std::string RegexManager::get_attrs_stfl_string(Dialog location,
	bool hasFocus)
{
	const auto& attributes = locations[location];
	std::string attrstr;
	for (unsigned int i = 0; i < attributes.size(); ++i) {
		const std::string& attribute = attributes[i].second;
		attrstr.append(strprintf::fmt("@style_%u_normal:%s ", i, attribute));
		if (hasFocus) {
			attrstr.append(strprintf::fmt("@style_%u_focus:%s ", i, attribute));
		}
	}
	return attrstr;
}

} // namespace newsboat

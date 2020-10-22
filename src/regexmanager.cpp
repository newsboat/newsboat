#include "regexmanager.h"

#include <cstring>
#include <iostream>
#include <stack>

#include "config.h"
#include "confighandlerexception.h"
#include "logger.h"
#include "strprintf.h"
#include "utils.h"

namespace newsboat {

RegexManager::RegexManager()
{
	// this creates the entries in the map. we need them there to have the
	// "all" location work.
	locations["article"];
	locations["articlelist"];
	locations["feedlist"];
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
	} else if (action == "highlight-article") {
		handle_highlight_article_action(params);
	} else {
		throw ConfigHandlerException(
			ActionHandlerStatus::INVALID_COMMAND);
	}
}

int RegexManager::article_matches(Matchable* item)
{
	for (const auto& Matcher : matchers) {
		if (Matcher.first->matches(item)) {
			return Matcher.second;
		}
	}
	return -1;
}

void RegexManager::remove_last_regex(const std::string& location)
{
	auto& regexes = locations[location];
	if (regexes.empty()) {
		return;
	}

	regexes.pop_back();
}

std::map<size_t, std::string> RegexManager::extract_style_tags(std::string& str)
{
	std::map<size_t, std::string> tags;

	size_t pos = 0;
	while (pos < str.size()) {
		auto tag_start = str.find_first_of("<>", pos);
		if (tag_start == std::string::npos) {
			break;
		}
		if (str[tag_start] == '>') {
			// Keep unmatched '>' (stfl way of encoding a literal '>')
			pos = tag_start + 1;
			continue;
		}
		auto tag_end = str.find_first_of("<>", tag_start + 1);
		if (tag_end == std::string::npos) {
			break;
		}
		if (str[tag_end] == '<') {
			// First '<' bracket is unmatched, ignoring it
			pos = tag_start + 1;
			continue;
		}
		if (tag_end - tag_start == 1) {
			// Convert "<>" into "<" (stfl way of encoding a literal '<')
			str.erase(tag_end, 1);
			pos = tag_start + 1;
			continue;
		}
		tags[tag_start] = str.substr(tag_start, tag_end - tag_start + 1);
		str.erase(tag_start, tag_end - tag_start + 1);
		pos = tag_start;
	}
	return tags;
}

void RegexManager::insert_style_tags(std::string& str,
	std::map<size_t, std::string>& tags)
{
	// Expand "<" into "<>" (reverse of what happened in extract_style_tags()
	size_t pos = 0;
	while (pos < str.size()) {
		auto bracket = str.find_first_of("<", pos);
		if (bracket == std::string::npos) {
			break;
		}
		pos = bracket + 1;
		// Add to strings in the `tags` map so we don't have to shift all the positions in that map
		// (would be necessary if inserting directly into `str`
		tags[pos] = ">" + tags[pos];
	}

	for (auto it = tags.rbegin(); it != tags.rend(); ++it) {
		if (it->first > str.length()) {
			// Ignore tags outside of string
			continue;
		}
		str.insert(it->first, it->second);
	}
}

void RegexManager::merge_style_tag(std::map<size_t, std::string>& tags,
	const std::string& tag, size_t start, size_t end)
{
	if (end <= start) {
		return;
	}

	// Find the latest tag occurring before `end`.
	// It is important that looping executes in ascending order of location.
	std::string latest_tag = "</>";
	for (const auto& location_tag : tags) {
		size_t location = location_tag.first;
		if (location > end) {
			break;
		}
		latest_tag = location_tag.second;
	}
	tags[start] = tag;
	tags[end] = latest_tag;

	// Remove any old tags between the start and end marker
	for (auto it = tags.begin(); it != tags.end(); ) {
		if (it->first > start && it->first < end) {
			it = tags.erase(it);
		} else {
			++it;
		}
	}
}

void RegexManager::quote_and_highlight(std::string& str,
	const std::string& location)
{
	auto& regexes = locations[location];

	auto tag_locations = extract_style_tags(str);

	for (unsigned int i = 0; i < regexes.size(); ++i) {
		const auto& regex = regexes[i].first;
		if (regex == nullptr) {
			continue;
		}
		unsigned int offset = 0;
		int eflags = 0;
		while (offset < str.length()) {
			const auto matches = regex->matches(str.substr(offset), 1, eflags);
			eflags |= REG_NOTBOL; // Don't match beginning-of-line operator (^) in following checks
			if (matches.empty()) {
				break;
			}
			const auto& match = matches[0];
			if (match.first != match.second) {
				const std::string marker = strprintf::fmt("<%u>", i);
				const int match_start = offset + match.first;
				const int match_end = offset + match.second;
				merge_style_tag(tag_locations, marker, match_start, match_end);
				offset = match_end;
			} else {
				offset++;
			}
		}
	}

	insert_style_tags(str, tag_locations);
}

void RegexManager::handle_highlight_action(const std::vector<std::string>&
	params)
{
	if (params.size() < 3) {
		throw ConfigHandlerException(
			ActionHandlerStatus::TOO_FEW_PARAMS);
	}

	std::string location = params[0];
	if (location != "all" && location != "article" &&
		location != "articlelist" && location != "feedlist") {
		throw ConfigHandlerException(strprintf::fmt(
				_("`%s' is an invalid dialog type"), location));
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
	if (location != "all") {
		LOG(Level::DEBUG,
			"RegexManager::handle_action: adding rx = %s "
			"colorstr = %s to location %s",
			params[1],
			colorstr,
			location);
		locations[location].push_back({std::move(regex), colorstr});
	} else {
		std::shared_ptr<Regex> sharedRegex(std::move(regex));
		for (auto& location : locations) {
			LOG(Level::DEBUG,
				"RegexManager::handle_action: adding "
				"rx = "
				"%s colorstr = %s to location %s",
				params[1],
				colorstr,
				location.first);
			location.second.push_back({sharedRegex, colorstr});
		}
	}
	std::string line = "highlight";
	for (const auto& param : params) {
		line.append(" ");
		line.append(utils::quote(param));
	}
	cheat_store_for_dump_config.push_back(line);
}

void RegexManager::handle_highlight_article_action(const
	std::vector<std::string>& params)
{
	if (params.size() < 3) {
		throw ConfigHandlerException(
			ActionHandlerStatus::TOO_FEW_PARAMS);
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

	int pos = locations["articlelist"].size();

	locations["articlelist"].push_back({nullptr, colorstr});

	matchers.push_back(
		std::pair<std::shared_ptr<Matcher>, int>(m, pos));
}

std::string RegexManager::get_attrs_stfl_string(const std::string& location,
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

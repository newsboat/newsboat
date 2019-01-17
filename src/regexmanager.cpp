#include "regexmanager.h"

#include <cstring>
#include <iostream>
#include <stack>

#include "config.h"
#include "exceptions.h"
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

RegexManager::~RegexManager()
{
	for (const auto& location : locations) {
		if (location.second.first.size() > 0) {
			for (const auto& regex : location.second.first) {
				delete regex;
			}
		}
	}
}

void RegexManager::dump_config(std::vector<std::string>& config_output)
{
	for (const auto& foo : cheat_store_for_dump_config) {
		config_output.push_back(foo);
	}
}

void RegexManager::handle_action(const std::string& action,
	const std::vector<std::string>& params)
{
	if (action == "highlight") {
		if (params.size() < 3)
			throw ConfigHandlerException(
				ActionHandlerStatus::TOO_FEW_PARAMS);

		std::string location = params[0];
		if (location != "all" && location != "article" &&
			location != "articlelist" && location != "feedlist")
			throw ConfigHandlerException(strprintf::fmt(
				_("`%s' is an invalid dialog type"), location));

		regex_t* rx = new regex_t;
		int err;
		if ((err = regcomp(rx,
			     params[1].c_str(),
			     REG_EXTENDED | REG_ICASE)) != 0) {
			char buf[1024];
			regerror(err, rx, buf, sizeof(buf));
			delete rx;
			throw ConfigHandlerException(strprintf::fmt(
				_("`%s' is not a valid regular expression: %s"),
				params[1],
				buf));
		}
		std::string colorstr;
		if (params[2] != "default") {
			colorstr.append("fg=");
			if (!utils::is_valid_color(params[2]))
				throw ConfigHandlerException(strprintf::fmt(
					_("`%s' is not a valid color"),
					params[2]));
			colorstr.append(params[2]);
		}
		if (params.size() > 3) {
			if (params[3] != "default") {
				if (colorstr.length() > 0)
					colorstr.append(",");
				colorstr.append("bg=");
				if (!utils::is_valid_color(params[3]))
					throw ConfigHandlerException(
						strprintf::fmt(
							_("`%s' is not a valid "
							  "color"),
							params[3]));
				colorstr.append(params[3]);
			}
			for (unsigned int i = 4; i < params.size(); ++i) {
				if (params[i] != "default") {
					if (colorstr.length() > 0)
						colorstr.append(",");
					colorstr.append("attr=");
					if (!utils::is_valid_attribute(
						    params[i]))
						throw ConfigHandlerException(
							strprintf::fmt(
								_("`%s' is not "
								  "a valid "
								  "attribute"),
								params[i]));
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
			locations[location].first.push_back(rx);
			locations[location].second.push_back(colorstr);
		} else {
			delete rx;
			for (auto& location : locations) {
				LOG(Level::DEBUG,
					"RegexManager::handle_action: adding "
					"rx = "
					"%s colorstr = %s to location %s",
					params[1],
					colorstr,
					location.first);
				rx = new regex_t;
				// we need to create a new one for each
				// push_back, otherwise we'd have double frees.
				regcomp(rx,
					params[1].c_str(),
					REG_EXTENDED | REG_ICASE);
				location.second.first.push_back(rx);
				location.second.second.push_back(colorstr);
			}
		}
		std::string line = "highlight";
		for (const auto& param : params) {
			line.append(" ");
			line.append(utils::quote(param));
		}
		cheat_store_for_dump_config.push_back(line);
	} else if (action == "highlight-article") {
		if (params.size() < 3)
			throw ConfigHandlerException(
				ActionHandlerStatus::TOO_FEW_PARAMS);

		std::string expr = params[0];
		std::string fgcolor = params[1];
		std::string bgcolor = params[2];

		std::string colorstr;
		if (fgcolor != "default") {
			colorstr.append("fg=");
			if (!utils::is_valid_color(fgcolor))
				throw ConfigHandlerException(strprintf::fmt(
					_("`%s' is not a valid color"),
					fgcolor));
			colorstr.append(fgcolor);
		}
		if (bgcolor != "default") {
			if (colorstr.length() > 0)
				colorstr.append(",");
			colorstr.append("bg=");
			if (!utils::is_valid_color(bgcolor))
				throw ConfigHandlerException(strprintf::fmt(
					_("`%s' is not a valid color"),
					bgcolor));
			colorstr.append(bgcolor);
		}

		for (unsigned int i = 3; i < params.size(); i++) {
			if (params[i] != "default") {
				if (colorstr.length() > 0)
					colorstr.append(",");
				colorstr.append("attr=");
				if (!utils::is_valid_attribute(params[i]))
					throw ConfigHandlerException(
						strprintf::fmt(
							_("`%s' is not a valid "
							  "attribute"),
							params[i]));
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

		int pos = locations["articlelist"].first.size();

		locations["articlelist"].first.push_back(nullptr);
		locations["articlelist"].second.push_back(colorstr);

		matchers.push_back(
			std::pair<std::shared_ptr<Matcher>, int>(m, pos));

	} else
		throw ConfigHandlerException(
			ActionHandlerStatus::INVALID_COMMAND);
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
	std::vector<regex_t*>& regexes = locations[location].first;

	auto it = regexes.begin() + regexes.size() - 1;
	delete *it;
	regexes.erase(it);
}

std::string RegexManager::extract_outer_marker(std::string str, const int index)
{
	// Non-zero number of non-angle bracket characters, enclosed in angle brackets
	std::regex regex("<[^<>]+>", std::regex::extended);
	const std::string close = "</>";
	std::string tmptag;
	std::stack<std::string> tagstack;
	int offset = 0;

	if (str.length() == 0) {
		return "";
	}

	std::smatch sm;
	while ( std::regex_search( str, sm, regex )) {
		// Get found tag
		tmptag = sm.str();
		// If the found tag is after the spot we're looking for
		if (sm.position(0) + offset > index ){
			if (!tagstack.empty() ) {
				return tagstack.top();
			} else {
				return "";
			}
		}
		if (tmptag == close) {
			//If a tag is closed without a partner error out
			if (tagstack.empty()) {
				return "";
			} else {
				tagstack.pop();
			}
		} else {
			tagstack.push(tmptag);
		}
		offset += sm.position(0) + sm.length(0);
		str = sm.suffix().str();
	}

	if (!tagstack.empty()) {
		return tagstack.top();
	}

	return "";
}

void RegexManager::quote_and_highlight(std::string& str,
	const std::string& location)
{
	std::vector<regex_t*>& regexes = locations[location].first;

	unsigned int i = 0;
	for (const auto& regex : regexes) {
		if (!regex) {
			continue;
		}
		regmatch_t pmatch;
		unsigned int offset = 0;
		int err = regexec(regex, str.c_str(), 1, &pmatch, 0);
		while (err == 0) {
			std::string outer_marker = "";
			if (pmatch.rm_so != pmatch.rm_eo) {
				outer_marker = extract_outer_marker(str, offset + pmatch.rm_so);
				const std::string marker = strprintf::fmt("<%u>", i);
				str.insert(offset + pmatch.rm_eo,
						std::string("</>") + outer_marker);
				str.insert(offset + pmatch.rm_so, marker);
				offset += pmatch.rm_eo + marker.length() +
					strlen("</>") + outer_marker.length();
			}
			else {
				offset++;
			}
			if (offset >= str.length()) {
				break;
			}
			err = regexec(
				regex, str.c_str() + offset, 1, &pmatch, 0);
		}
		i++;
	}
}

} // namespace newsboat

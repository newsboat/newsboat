#include "regexmanager.h"

#include <cstring>

#include "config.h"
#include "exceptions.h"
#include "logger.h"
#include "strprintf.h"
#include "utils.h"

namespace newsboat {

regexmanager::regexmanager()
{
	// this creates the entries in the map. we need them there to have the
	// "all" location work.
	locations["article"];
	locations["articlelist"];
	locations["feedlist"];
}

regexmanager::~regexmanager()
{
	for (auto location : locations) {
		if (location.second.first.size() > 0) {
			for (auto regex : location.second.first) {
				delete regex;
			}
		}
	}
}

void regexmanager::dump_config(std::vector<std::string>& config_output)
{
	for (auto foo : cheat_store_for_dump_config) {
		config_output.push_back(foo);
	}
}

void regexmanager::handle_action(const std::string& action,
	const std::vector<std::string>& params)
{
	if (action == "highlight") {
		if (params.size() < 3)
			throw confighandlerexception(
				action_handler_status::TOO_FEW_PARAMS);

		std::string location = params[0];
		if (location != "all" && location != "article" &&
			location != "articlelist" && location != "feedlist")
			throw confighandlerexception(strprintf::fmt(
				_("`%s' is an invalid dialog type"), location));

		regex_t* rx = new regex_t;
		int err;
		if ((err = regcomp(rx,
			     params[1].c_str(),
			     REG_EXTENDED | REG_ICASE)) != 0) {
			char buf[1024];
			regerror(err, rx, buf, sizeof(buf));
			delete rx;
			throw confighandlerexception(strprintf::fmt(
				_("`%s' is not a valid regular expression: %s"),
				params[1],
				buf));
		}
		std::string colorstr;
		if (params[2] != "default") {
			colorstr.append("fg=");
			if (!utils::is_valid_color(params[2]))
				throw confighandlerexception(strprintf::fmt(
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
					throw confighandlerexception(
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
						throw confighandlerexception(
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
			LOG(level::DEBUG,
				"regexmanager::handle_action: adding rx = %s "
				"colorstr = %s to location %s",
				params[1],
				colorstr,
				location);
			locations[location].first.push_back(rx);
			locations[location].second.push_back(colorstr);
		} else {
			delete rx;
			for (auto& location : locations) {
				LOG(level::DEBUG,
					"regexmanager::handle_action: adding "
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
			throw confighandlerexception(
				action_handler_status::TOO_FEW_PARAMS);

		std::string expr = params[0];
		std::string fgcolor = params[1];
		std::string bgcolor = params[2];

		std::string colorstr;
		if (fgcolor != "default") {
			colorstr.append("fg=");
			if (!utils::is_valid_color(fgcolor))
				throw confighandlerexception(strprintf::fmt(
					_("`%s' is not a valid color"),
					fgcolor));
			colorstr.append(fgcolor);
		}
		if (bgcolor != "default") {
			if (colorstr.length() > 0)
				colorstr.append(",");
			colorstr.append("bg=");
			if (!utils::is_valid_color(bgcolor))
				throw confighandlerexception(strprintf::fmt(
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
					throw confighandlerexception(
						strprintf::fmt(
							_("`%s' is not a valid "
							  "attribute"),
							params[i]));
				colorstr.append(params[i]);
			}
		}

		std::shared_ptr<matcher> m(new matcher());
		if (!m->parse(params[0])) {
			throw confighandlerexception(strprintf::fmt(
				_("couldn't parse filter expression `%s': %s"),
				params[0],
				m->get_parse_error()));
		}

		int pos = locations["articlelist"].first.size();

		locations["articlelist"].first.push_back(nullptr);
		locations["articlelist"].second.push_back(colorstr);

		matchers.push_back(
			std::pair<std::shared_ptr<matcher>, int>(m, pos));

	} else
		throw confighandlerexception(
			action_handler_status::INVALID_COMMAND);
}

int regexmanager::article_matches(matchable* item)
{
	for (auto matcher : matchers) {
		if (matcher.first->matches(item)) {
			return matcher.second;
		}
	}
	return -1;
}

void regexmanager::remove_last_regex(const std::string& location)
{
	std::vector<regex_t*>& regexes = locations[location].first;

	auto it = regexes.begin() + regexes.size() - 1;
	delete *it;
	regexes.erase(it);
}

std::string regexmanager::extract_initial_marker(const std::string& str)
{
	if (str.length() == 0)
		return "";

	if (str[0] == '<') {
		std::string::size_type pos = str.find_first_of(">", 0);
		if (pos != std::string::npos) {
			return str.substr(0, pos + 1);
		}
	}
	return "";
}

void regexmanager::quote_and_highlight(std::string& str,
	const std::string& location)
{
	std::vector<regex_t*>& regexes = locations[location].first;

	unsigned int i = 0;
	for (auto regex : regexes) {
		if (!regex)
			continue;
		std::string initial_marker = extract_initial_marker(str);
		regmatch_t pmatch;
		unsigned int offset = 0;
		int err = regexec(regex, str.c_str(), 1, &pmatch, 0);
		while (err == 0) {
			std::string marker = strprintf::fmt("<%u>", i);
			str.insert(offset + pmatch.rm_eo,
				std::string("</>") + initial_marker);
			str.insert(offset + pmatch.rm_so, marker);
			offset += pmatch.rm_eo + marker.length() +
				strlen("</>") + initial_marker.length();
			err = regexec(
				regex, str.c_str() + offset, 1, &pmatch, 0);
		}
		i++;
	}
}

} // namespace newsboat

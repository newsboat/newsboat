#include "rssignores.h"

#include <algorithm>
#include <curl/curl.h>
#include <langinfo.h>
#include <sys/utsname.h>

#include "cache.h"
#include "config.h"
#include "confighandlerexception.h"
#include "configparser.h"
#include "logger.h"
#include "regexowner.h"
#include "strprintf.h"
#include "utils.h"

namespace Newsboat {

const std::string RssIgnores::REGEX_PREFIX = "regex:";

void RssIgnores::handle_action(const std::string& action,
	const std::vector<std::string>& params)
{
	if (action == "ignore-article") {
		if (params.size() < 2) {
			throw ConfigHandlerException(ActionHandlerStatus::TOO_FEW_PARAMS);
		}
		std::string ignore_rssurl = params[0];
		std::string ignore_expr = params[1];
		auto m = std::make_shared<Matcher>();
		if (!m->parse(ignore_expr)) {
			throw ConfigHandlerException(strprintf::fmt(
					_("couldn't parse filter expression `%s': %s"),
					ignore_expr,
					m->get_parse_error()));
		}

		const int prefix_len = REGEX_PREFIX.length();
		if (!ignore_rssurl.compare(0, prefix_len, REGEX_PREFIX)) {
			std::string errorMessage;
			const std::string pattern = ignore_rssurl.substr(prefix_len,
					ignore_rssurl.length() - prefix_len);
			const auto regex = Regex::compile(pattern, REG_EXTENDED | REG_ICASE, errorMessage);
			if (regex == nullptr) {
				throw ConfigHandlerException(strprintf::fmt(
						_("`%s' is not a valid regular expression: %s"),
						pattern, errorMessage));
			}

			regex_ignores.push_back(FeedUrlExprPair(pattern, m));
		} else {
			non_regex_ignores.insert({ignore_rssurl, m});
		}
	} else if (action == "always-download") {
		if (params.empty()) {
			throw ConfigHandlerException(ActionHandlerStatus::TOO_FEW_PARAMS);
		}

		for (const auto& param : params) {
			ignores_lastmodified.push_back(param);
		}
	} else if (action == "reset-unread-on-update") {
		if (params.empty()) {
			throw ConfigHandlerException(ActionHandlerStatus::TOO_FEW_PARAMS);
		}

		for (const auto& param : params) {
			resetflag.push_back(param);
		}
	} else {
		throw ConfigHandlerException(ActionHandlerStatus::INVALID_COMMAND);
	}
}

void RssIgnores::dump_config(std::vector<std::string>& config_output) const
{
	for (const auto& ign : non_regex_ignores) {
		std::string configline = "ignore-article ";
		if (ign.first == "*") {
			configline.append("*");
		} else {
			configline.append(utils::quote(ign.first));
		}
		configline.append(" ");
		configline.append(utils::quote(ign.second->get_expression()));
		config_output.push_back(configline);
	}
	for (const auto& ign : regex_ignores) {
		std::string configline = "ignore-article ";
		configline.append(utils::quote(REGEX_PREFIX + ign.first));
		configline.append(" ");
		configline.append(utils::quote(ign.second->get_expression()));
		config_output.push_back(configline);
	}
	for (const auto& ign_lm : ignores_lastmodified) {
		config_output.push_back(strprintf::fmt(
				"always-download %s", utils::quote(ign_lm)));
	}
	for (const auto& rf : resetflag) {
		config_output.push_back(strprintf::fmt(
				"reset-unread-on-update %s", utils::quote(rf)));
	}
}

bool RssIgnores::matches_expr(std::shared_ptr<Matcher> expr, RssItem* item)
{
	if (expr->matches(item)) {
		LOG(Level::DEBUG,
			"RssIgnores::matches_expr: found match");
		return true;
	}

	return false;
}

bool RssIgnores::matches(RssItem* item)
{
	auto search = non_regex_ignores.equal_range(item->feedurl());
	for (auto itr = search.first; itr != search.second; itr++) {
		if (matches_expr(itr->second, item)) {
			return true;
		}
	}

	search = non_regex_ignores.equal_range("*");
	for (auto itr = search.first; itr != search.second; itr++) {
		if (matches_expr(itr->second, item)) {
			return true;
		}
	}

	for (const auto& ign : regex_ignores) {
		const std::string pattern = ign.first;

		LOG(Level::DEBUG,
			"RssIgnores::matches: ign.first = `%s' item->feedurl = `%s'",
			pattern,
			item->feedurl());

		std::string errorMessage;
		const auto regex = Regex::compile(pattern, REG_EXTENDED | REG_ICASE, errorMessage);
		const auto matches = regex->matches(item->feedurl(), 1, 0);
		if (!matches.empty() && matches_expr(ign.second, item)) {
			return true;
		}
	}

	return false;
}

bool RssIgnores::matches_lastmodified(const std::string& url)
{
	return std::find_if(ignores_lastmodified.begin(),
			ignores_lastmodified.end(),
	[&](const std::string& u) {
		return u == url;
	}) !=
	ignores_lastmodified.end();
}

bool RssIgnores::matches_resetunread(const std::string& url)
{
	return std::find_if(resetflag.begin(),
			resetflag.end(),
	[&](const std::string& u) {
		return u == url;
	}) !=
	resetflag.end();
}

} // namespace Newsboat

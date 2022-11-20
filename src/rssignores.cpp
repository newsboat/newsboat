#include "rssignores.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <curl/curl.h>
#include <functional>
#include <iostream>
#include <langinfo.h>
#include <sstream>
#include <sys/utsname.h>
#include <string.h>
#include <time.h>

#include "cache.h"
#include "config.h"
#include "configcontainer.h"
#include "confighandlerexception.h"
#include "configparser.h"
#include "dbexception.h"
#include "htmlrenderer.h"
#include "logger.h"
#include "rssfeed.h"
#include "regexowner.h"
#include "strprintf.h"
#include "tagsouppullparser.h"
#include "utils.h"

namespace newsboat {

const Utf8String RssIgnores::REGEX_PREFIX = "regex:";

void RssIgnores::handle_action(const Utf8String& action,
	const std::vector<Utf8String>& params)
{
	if (action == "ignore-article") {
		if (params.size() < 2) {
			throw ConfigHandlerException(ActionHandlerStatus::TOO_FEW_PARAMS);
		}
		auto ignore_rssurl = params[0].utf8();
		const auto& ignore_expr = params[1];
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
			const auto pattern = ignore_rssurl.substr(prefix_len,
					ignore_rssurl.length() - prefix_len);
			const auto regex = Regex::compile(pattern, REG_EXTENDED | REG_ICASE, errorMessage);
			if (regex == nullptr) {
				throw ConfigHandlerException(strprintf::fmt(
						_("`%s' is not a valid regular expression: %s"),
						pattern, errorMessage));
			}
		}

		ignores.push_back(FeedUrlExprPair(ignore_rssurl, m));
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

void RssIgnores::dump_config(std::vector<Utf8String>& config_output) const
{
	for (const auto& ign : ignores) {
		Utf8String configline = "ignore-article ";
		if (ign.first == "*") {
			configline.append("*");
		} else {
			configline.append(utils::quote(ign.first));
		}
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

bool RssIgnores::matches(RssItem* item)
{
	const int prefix_len = REGEX_PREFIX.length();
	for (const auto& ign : ignores) {
		bool matched = false;
		LOG(Level::DEBUG,
			"RssIgnores::matches: ign.first = `%s' item->feedurl = `%s'",
			ign.first,
			item->feedurl());

		if (utils::starts_with(REGEX_PREFIX, ign.first)) {
			const auto pattern = ign.first.map([&](std::string s) {
				return s.substr(prefix_len, ign.first.length() - prefix_len);
			});
			std::string errorMessage;
			const auto regex = Regex::compile(pattern, REG_EXTENDED | REG_ICASE, errorMessage);
			const auto matches = regex->matches(item->feedurl(), 1, 0);
			matched = !matches.empty();
		} else {
			matched = ign.first == "*" || item->feedurl() == ign.first;
		}

		if (matched && ign.second->matches(item)) {
			LOG(Level::DEBUG,
				"RssIgnores::matches: found match");
			return true;
		}
	}
	return false;
}

bool RssIgnores::matches_lastmodified(const Utf8String& url)
{
	return std::find_if(ignores_lastmodified.begin(),
			ignores_lastmodified.end(),
	[&](const Utf8String& u) {
		return u == url;
	}) !=
	ignores_lastmodified.end();
}

bool RssIgnores::matches_resetunread(const Utf8String& url)
{
	return std::find_if(resetflag.begin(),
			resetflag.end(),
	[&](const Utf8String& u) {
		return u == url;
	}) !=
	resetflag.end();
}

} // namespace newsboat

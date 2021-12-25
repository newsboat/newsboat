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
#include <regex>

#include "cache.h"
#include "config.h"
#include "configcontainer.h"
#include "confighandlerexception.h"
#include "configparser.h"
#include "dbexception.h"
#include "htmlrenderer.h"
#include "logger.h"
#include "rssfeed.h"
#include "strprintf.h"
#include "tagsouppullparser.h"
#include "utils.h"

namespace newsboat {

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

void RssIgnores::dump_config(std::vector<std::string>& config_output) const
{
	for (const auto& ign : ignores) {
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
	for (const auto& ign : ignores) {
		auto matched = regex_match (item->feedurl(), std::regex(ign.first));
		LOG(Level::DEBUG,
			"RssIgnores::matches: ign.first = `%s' item->feedurl = `%s': %d",
			ign.first,
			item->feedurl(), matched);
		if (matched) {
			if (ign.second->matches(item)) {
				LOG(Level::DEBUG,
					"RssIgnores::matches: found match");
				return true;
			}
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

} // namespace newsboat

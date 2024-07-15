#include "configcontainer.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <pwd.h>
#include <sstream>
#include <sys/types.h>

#include "config.h"
#include "configparser.h"
#include "configdata.h"
#include "confighandlerexception.h"
#include "logger.h"
#include "strprintf.h"
#include "utils.h"

namespace newsboat {

const std::string ConfigContainer::PARTIAL_FILE_SUFFIX = ".part";

ConfigContainer::ConfigContainer()
// create the config options and set their resp. default value and type
	: config_data{{"always-display-description",
		ConfigData("false", ConfigDataType::BOOL)},
	{
		"article-sort-order",
		ConfigData("date-asc", ConfigDataType::STR)},
	{
		"articlelist-format",
		ConfigData("%4i %f %D %6L  %?T?|%-17T|  &?%t",
			ConfigDataType::STR)},
	{"auto-reload", ConfigData("no", ConfigDataType::BOOL)},
	{
		"bookmark-autopilot",
		ConfigData("false", ConfigDataType::BOOL)},
	{"bookmark-cmd", ConfigData("", ConfigDataType::STR)},
	{
		"bookmark-interactive",
		ConfigData("false", ConfigDataType::BOOL)},
	{
		"browser",
		ConfigData(utils::get_default_browser().to_locale_string(),
			ConfigDataType::PATH)},
	{"cache-file", ConfigData("", ConfigDataType::PATH)},
	{
		"cleanup-on-quit",
		ConfigData("nudge",
		std::unordered_set<std::string>({
			"yes",
			"no",
			"nudge",
			// true/false only added for backwards compatiblity as this option was previously a bool
			"true",
			"false"}))},
	{"confirm-delete-all-articles", ConfigData("yes", ConfigDataType::BOOL)},
	{"confirm-mark-all-feeds-read", ConfigData("yes", ConfigDataType::BOOL)},
	{"confirm-mark-feed-read", ConfigData("yes", ConfigDataType::BOOL)},
	{"confirm-exit", ConfigData("no", ConfigDataType::BOOL)},
	{"cookie-cache", ConfigData("", ConfigDataType::PATH)},
	{"datetime-format", ConfigData("%b %d", ConfigDataType::STR)},
	{
		"delete-read-articles-on-quit",
		ConfigData("false", ConfigDataType::BOOL)},
	{"delete-played-files", ConfigData("false", ConfigDataType::BOOL)},
	{
		"display-article-progress",
		ConfigData("yes", ConfigDataType::BOOL)},
	{
		"download-filename-format",
		ConfigData("%?u?%u&%Y-%b-%d-%H%M%S.unknown?",
			ConfigDataType::STR)},
	{
		"download-full-page",
		ConfigData("false", ConfigDataType::BOOL)},
	{"download-path", ConfigData("~/", ConfigDataType::PATH)},
	{"download-retries", ConfigData("1", ConfigDataType::INT)},
	{"download-timeout", ConfigData("30", ConfigDataType::INT)},
	{"error-log", ConfigData("", ConfigDataType::PATH)},
	{"external-url-viewer", ConfigData("", ConfigDataType::PATH)},
	{
		"feed-sort-order",
		ConfigData("none-desc", ConfigDataType::STR)},
	{"feedhq-flag-share", ConfigData("", ConfigDataType::STR)},
	{"feedhq-flag-star", ConfigData("", ConfigDataType::STR)},
	{"feedhq-login", ConfigData("", ConfigDataType::STR)},
	{"feedhq-min-items", ConfigData("20", ConfigDataType::INT)},
	{"feedhq-password", ConfigData("", ConfigDataType::STR)},
	{"feedhq-passwordfile", ConfigData("", ConfigDataType::PATH)},
	{"feedhq-passwordeval", ConfigData("", ConfigDataType::STR)},
	{
		"feedhq-show-special-feeds",
		ConfigData("true", ConfigDataType::BOOL)},
	{
		"feedhq-url",
		ConfigData("https://feedhq.org/", ConfigDataType::STR)},
	{"feedbin-url", ConfigData("https://api.feedbin.com", ConfigDataType::STR)},
	{"feedbin-login", ConfigData("", ConfigDataType::STR)},
	{"feedbin-password", ConfigData("", ConfigDataType::STR)},
	{"feedbin-passwordfile", ConfigData("", ConfigDataType::PATH)},
	{"feedbin-passwordeval", ConfigData("", ConfigDataType::STR)},
	{"feedbin-flag-star", ConfigData("", ConfigDataType::STR)},
	{"freshrss-flag-star", ConfigData("", ConfigDataType::STR)},
	{"freshrss-login", ConfigData("", ConfigDataType::STR)},
	{"freshrss-min-items", ConfigData("20", ConfigDataType::INT)},
	{"freshrss-password", ConfigData("", ConfigDataType::STR)},
	{"freshrss-passwordfile", ConfigData("", ConfigDataType::PATH)},
	{"freshrss-passwordeval", ConfigData("", ConfigDataType::STR)},
	{
		"freshrss-show-special-feeds",
		ConfigData("true", ConfigDataType::BOOL)},
	{
		"freshrss-url",
		ConfigData("", ConfigDataType::STR)},
	{
		"feedlist-format",
		ConfigData("%4i %n %11u %t", ConfigDataType::STR)},
	{"goto-first-unread", ConfigData("true", ConfigDataType::BOOL)},
	{"goto-next-feed", ConfigData("yes", ConfigDataType::BOOL)},
	{"history-limit", ConfigData("100", ConfigDataType::INT)},
	{"html-renderer", ConfigData("internal", ConfigDataType::PATH)},
	{
		"http-auth-method",
		ConfigData("any",
		std::unordered_set<std::string>({"any",
			"basic",
			"digest",
			"digest_ie",
			"gssnegotiate",
			"ntlm",
			"anysafe"}))},
	{
		"ignore-mode",
		ConfigData("download",
			std::unordered_set<std::string>(
		{"download", "display"}))},
	{"inoreader-app-id", ConfigData("", ConfigDataType::STR)},
	{"inoreader-app-key", ConfigData("", ConfigDataType::STR)},
	{"inoreader-login", ConfigData("", ConfigDataType::STR)},
	{"inoreader-password", ConfigData("", ConfigDataType::STR)},
	{
		"inoreader-passwordfile",
		ConfigData("", ConfigDataType::PATH)},
	{"inoreader-passwordeval", ConfigData("", ConfigDataType::STR)},
	{
		"inoreader-show-special-feeds",
		ConfigData("true", ConfigDataType::BOOL)},
	{"inoreader-flag-share", ConfigData("", ConfigDataType::STR)},
	{"inoreader-flag-star", ConfigData("", ConfigDataType::STR)},
	{"inoreader-min-items", ConfigData("20", ConfigDataType::INT)},
	{"keep-articles-days", ConfigData("0", ConfigDataType::INT)},
	{
		"mark-as-read-on-hover",
		ConfigData("false", ConfigDataType::BOOL)},
	{"max-browser-tabs", ConfigData("10", ConfigDataType::INT)},
	{
		"markfeedread-jumps-to-next-unread",
		ConfigData("false", ConfigDataType::BOOL)},
	{"max-download-speed", ConfigData("0", ConfigDataType::INT)},
	{"max-downloads", ConfigData("1", ConfigDataType::INT)},
	{"max-items", ConfigData("0", ConfigDataType::INT)},
	{"newsblur-login", ConfigData("", ConfigDataType::STR)},
	{"newsblur-min-items", ConfigData("20", ConfigDataType::INT)},
	{"newsblur-password", ConfigData("", ConfigDataType::STR)},
	{"newsblur-passwordfile", ConfigData("", ConfigDataType::PATH)},
	{"newsblur-passwordeval", ConfigData("", ConfigDataType::STR)},
	{
		"newsblur-url",
		ConfigData("https://newsblur.com",
			ConfigDataType::STR)},
	{"notify-always", ConfigData("no", ConfigDataType::BOOL)},
	{"notify-beep", ConfigData("no", ConfigDataType::BOOL)},
	{
		"notify-format",
		ConfigData(_("Newsboat: finished reload, %f unread "
				"feeds (%n unread articles total)"),
			ConfigDataType::STR)},
	{"notify-program", ConfigData("", ConfigDataType::PATH)},
	{"notify-screen", ConfigData("no", ConfigDataType::BOOL)},
	{"notify-xterm", ConfigData("no", ConfigDataType::BOOL)},
	{"oldreader-flag-share", ConfigData("", ConfigDataType::STR)},
	{"oldreader-flag-star", ConfigData("", ConfigDataType::STR)},
	{"oldreader-login", ConfigData("", ConfigDataType::STR)},
	{"oldreader-min-items", ConfigData("20", ConfigDataType::INT)},
	{"oldreader-password", ConfigData("", ConfigDataType::STR)},
	{
		"oldreader-passwordfile",
		ConfigData("", ConfigDataType::PATH)},
	{"oldreader-passwordeval", ConfigData("", ConfigDataType::STR)},
	{
		"oldreader-show-special-feeds",
		ConfigData("true", ConfigDataType::BOOL)},
	{
		"openbrowser-and-mark-jumps-to-next-unread",
		ConfigData("false", ConfigDataType::BOOL)},
	{"opml-url", ConfigData("", ConfigDataType::STR, true)},
	{"pager", ConfigData("internal", ConfigDataType::PATH)},
	{"player", ConfigData("", ConfigDataType::PATH)},
	{
		"podcast-auto-enqueue",
		ConfigData("no", ConfigDataType::BOOL)},
	{
		"podlist-format",
		ConfigData( _("%4i [%6dMB/%6tMB] [%5p %%] [%12K] %-20S %u -> %F"), ConfigDataType::STR)},
	{
		"prepopulate-query-feeds",
		ConfigData("false", ConfigDataType::BOOL)},
	{"ssl-verifyhost", ConfigData("true", ConfigDataType::BOOL)},
	{"ssl-verifypeer", ConfigData("true", ConfigDataType::BOOL)},
	{"proxy", ConfigData("", ConfigDataType::STR)},
	{"proxy-auth", ConfigData("", ConfigDataType::STR)},
	{
		"proxy-auth-method",
		ConfigData("any",
		std::unordered_set<std::string>({"any",
			"basic",
			"digest",
			"digest_ie",
			"gssnegotiate",
			"ntlm",
			"anysafe"}))},
	{
		"proxy-type",
		ConfigData("http",
		std::unordered_set<std::string>({"http",
			"socks4",
			"socks4a",
			"socks5",
			"socks5h"}))},
	{"refresh-on-startup", ConfigData("no", ConfigDataType::BOOL)},
	{
		"reload-only-visible-feeds",
		ConfigData("false", ConfigDataType::BOOL)},
	{"reload-threads", ConfigData("1", ConfigDataType::INT)},
	{"reload-time", ConfigData("60", ConfigDataType::INT)},
	{"restrict-filename", ConfigData("yes", ConfigDataType::BOOL)},
	{"save-path", ConfigData("~/", ConfigDataType::PATH)},
	{"scrolloff", ConfigData("0", ConfigDataType::INT)},
	{
		"search-highlight-colors",
		ConfigData("black yellow bold",
			ConfigDataType::STR,
			true)},
	{
		"selecttag-format",
		ConfigData("%4i  %T (%u)", ConfigDataType::STR)},
	{"show-keymap-hint", ConfigData("yes", ConfigDataType::BOOL)},
	{"show-title-bar", ConfigData("yes", ConfigDataType::BOOL)},
	{"show-read-articles", ConfigData("yes", ConfigDataType::BOOL)},
	{"show-read-feeds", ConfigData("yes", ConfigDataType::BOOL)},
	{
		"suppress-first-reload",
		ConfigData("no", ConfigDataType::BOOL)},
	{
		"swap-title-and-hints",
		ConfigData("no", ConfigDataType::BOOL)},
	{"text-width", ConfigData("0", ConfigDataType::INT)},
	{
		"toggleitemread-jumps-to-next-unread",
		ConfigData("false", ConfigDataType::BOOL)},
	{"ttrss-flag-publish", ConfigData("", ConfigDataType::STR)},
	{"ttrss-flag-star", ConfigData("", ConfigDataType::STR)},
	{"ttrss-login", ConfigData("", ConfigDataType::STR)},
	{
		"ttrss-mode",
		ConfigData("multi",
			std::unordered_set<std::string>(
		{"single", "multi"}))},
	{"ttrss-password", ConfigData("", ConfigDataType::STR)},
	{"ttrss-passwordfile", ConfigData("", ConfigDataType::PATH)},
	{"ttrss-passwordeval", ConfigData("", ConfigDataType::STR)},
	{"ttrss-url", ConfigData("", ConfigDataType::STR)},
	{"ocnews-login", ConfigData("", ConfigDataType::STR)},
	{"ocnews-password", ConfigData("", ConfigDataType::STR)},
	{"ocnews-passwordfile", ConfigData("", ConfigDataType::PATH)},
	{"ocnews-passwordeval", ConfigData("", ConfigDataType::STR)},
	{"ocnews-flag-star", ConfigData("", ConfigDataType::STR)},
	{"ocnews-url", ConfigData("", ConfigDataType::STR)},
	{"miniflux-flag-star", ConfigData("", ConfigDataType::STR)},
	{"miniflux-login", ConfigData("", ConfigDataType::STR)},
	{"miniflux-min-items", ConfigData("100", ConfigDataType::INT)},
	{"miniflux-password", ConfigData("", ConfigDataType::STR)},
	{"miniflux-passwordfile", ConfigData("", ConfigDataType::PATH)},
	{"miniflux-passwordeval", ConfigData("", ConfigDataType::STR)},
	{
		"miniflux-show-special-feeds",
		ConfigData("yes", ConfigDataType::BOOL)},
	{"miniflux-token", ConfigData("", ConfigDataType::STR)},
	{"miniflux-tokenfile", ConfigData("", ConfigDataType::PATH)},
	{"miniflux-tokeneval", ConfigData("", ConfigDataType::STR)},
	{"miniflux-url", ConfigData("", ConfigDataType::STR)},
	{
		"urls-source",
		ConfigData("local",
		std::unordered_set<std::string>({"local",
			"opml",
			"oldreader",
			"ttrss",
			"newsblur",
			"feedhq",
			"feedbin",
			"freshrss",
			"ocnews",
			"miniflux",
			"inoreader"}))},
	{"use-proxy", ConfigData("no", ConfigDataType::BOOL)},
	{"user-agent", ConfigData("", ConfigDataType::STR)},

	/* title formats: */
	{
		"articlelist-title-format",
		ConfigData(_("%N %V - Articles in feed '%T' (%u unread, %t total)%?F? matching filter '%F'&? - %U"),
			ConfigDataType::STR)},
	{
		"dialogs-title-format",
		ConfigData(_("%N %V - Dialogs"), ConfigDataType::STR)},
	{
		"feedlist-title-format",
		ConfigData(_("%N %V - %?F?Feeds&Your feeds? (%u unread, %t total)%?F? matching filter '%F'&?%?T? - tag '%T'&?"),
			ConfigDataType::STR)},
	{
		"filebrowser-title-format",
		ConfigData(_("%N %V - %?O?Open File&Save File? - %f"),
			ConfigDataType::STR)},
	{
		"dirbrowser-title-format",
		ConfigData(_("%N %V - %?O?Open Directory&Save File? - %f"),
			ConfigDataType::STR)},
	{
		"help-title-format",
		ConfigData(_("%N %V - Help"), ConfigDataType::STR)},
	{
		"itemview-title-format",
		ConfigData(_("%N %V - Article '%T' (%u unread, %t total)"),
			ConfigDataType::STR)},
	{
		"searchresult-title-format",
		ConfigData(_("%N %V - Search results for '%s' (%u unread, %t total)%?F? matching filter '%F'&?"),
			ConfigDataType::STR)},
	{
		"selectfilter-title-format",
		ConfigData(_("%N %V - Select Filter"),
			ConfigDataType::STR)},
	{
		"selecttag-title-format",
		ConfigData(_("%N %V - Select Tag"),
			ConfigDataType::STR)},
	{
		"urlview-title-format",
		ConfigData(_("%N %V - URLs"), ConfigDataType::STR)},
	{
		"wrap-scroll",
		ConfigData("no", ConfigDataType::BOOL)},
}
{
}

ConfigContainer::~ConfigContainer() = default;

void ConfigContainer::register_commands(ConfigParser& cfgparser)
{
	// this registers the config options defined above in the configuration
	// parser
	// -> if the resp. config option is encountered, it is passed to the
	// ConfigContainer
	std::lock_guard<std::recursive_mutex> guard(config_data_mtx);
	for (const auto& cfg : config_data) {
		cfgparser.register_handler(cfg.first, *this);
	}
}

void ConfigContainer::handle_action(const std::string& action,
	const std::vector<std::string>& params)
{
	std::lock_guard<std::recursive_mutex> guard(config_data_mtx);
	ConfigData& cfgdata = config_data[action];

	// ConfigDataType::INVALID indicates that the action didn't exist, and
	// that the returned object was created ad-hoc.
	if (cfgdata.type() == ConfigDataType::INVALID) {
		LOG(Level::WARN,
			"ConfigContainer::handle_action: unknown action %s",
			action);
		throw ConfigHandlerException(ActionHandlerStatus::INVALID_COMMAND);
	}

	LOG(Level::DEBUG,
		"ConfigContainer::handle_action: action = %s, type = %u",
		action,
		static_cast<unsigned int>(cfgdata.type()));

	if (params.size() == 0) {
		throw ConfigHandlerException(ActionHandlerStatus::TOO_FEW_PARAMS);
	} else if (params.size() > 1 && !cfgdata.multi_option()) {
		throw ConfigHandlerException(ActionHandlerStatus::TOO_MANY_PARAMS);
	}

	switch (cfgdata.type()) {
	case ConfigDataType::BOOL:
	case ConfigDataType::INT:
	case ConfigDataType::ENUM: {
		const auto result = cfgdata.set_value(params[0]);
		if (!result) {
			throw ConfigHandlerException(result.error());
		}
	}
	break;

	case ConfigDataType::STR:
	case ConfigDataType::PATH:
		if (cfgdata.multi_option()) {
			cfgdata.set_value(utils::join(params, " "));
		} else {
			cfgdata.set_value(params[0]);
		}
		break;

	case ConfigDataType::INVALID:
		// we already handled this at the beginning of the function
		break;
	}
}

std::string ConfigContainer::get_configvalue(const std::string& key) const
{
	std::lock_guard<std::recursive_mutex> guard(config_data_mtx);
	auto it = config_data.find(key);
	if (it != config_data.cend()) {
		const auto& entry = it->second;
		std::string value = entry.value();
		if (entry.type() == ConfigDataType::PATH) {
			value = utils::resolve_tilde(Filepath::from_locale_string(value)).to_locale_string();
		}
		return value;
	}

	return {};
}

int ConfigContainer::get_configvalue_as_int(const std::string& key) const
{
	std::lock_guard<std::recursive_mutex> guard(config_data_mtx);
	auto it = config_data.find(key);
	if (it != config_data.cend()) {
		const auto& value = it->second.value();
		std::istringstream is(value);
		int i;
		is >> i;
		return i;
	}

	return 0;
}

bool ConfigContainer::get_configvalue_as_bool(const std::string& key) const
{
	std::lock_guard<std::recursive_mutex> guard(config_data_mtx);
	auto it = config_data.find(key);
	if (it != config_data.cend()) {
		const auto& value = it->second.value();
		return (value == "true" || value == "yes");
	}

	return false;
}

nonstd::expected<void, std::string> ConfigContainer::set_configvalue(
	const std::string& key,
	const std::string& value)
{
	LOG(Level::DEBUG,
		"ConfigContainer::set_configvalue(%s, %s) called",
		key,
		value);
	std::lock_guard<std::recursive_mutex> guard(config_data_mtx);
	auto config_option = config_data.find(key);
	if (config_option != config_data.end()) {
		return config_option->second.set_value(value);
	} else {
		return nonstd::make_unexpected(strprintf::fmt(
					_("unknown config option: %s"),
					key));
	}
}

void ConfigContainer::reset_to_default(const std::string& key)
{
	std::lock_guard<std::recursive_mutex> guard(config_data_mtx);
	config_data[key].reset_to_default();
}

void ConfigContainer::toggle(const std::string& key)
{
	std::lock_guard<std::recursive_mutex> guard(config_data_mtx);
	if (config_data[key].type() == ConfigDataType::BOOL) {
		set_configvalue(key,
			std::string(get_configvalue_as_bool(key) ? "false"
				: "true"));
	}
}

void ConfigContainer::dump_config(std::vector<std::string>& config_output) const
{
	std::lock_guard<std::recursive_mutex> guard(config_data_mtx);
	for (const auto& cfg : config_data) {
		std::string configline = cfg.first + " ";
		assert(cfg.second.type() != ConfigDataType::INVALID);
		switch (cfg.second.type()) {
		case ConfigDataType::BOOL:
		case ConfigDataType::INT:
			configline.append(cfg.second.value());
			if (cfg.second.value() != cfg.second.default_value()) {
				configline.append(
					strprintf::fmt(
						" # default: %s",
						cfg.second.default_value()));
			}
			break;
		case ConfigDataType::ENUM:
		case ConfigDataType::STR:
		case ConfigDataType::PATH:
			if (cfg.second.multi_option()) {
				const std::vector<std::string> tokens = utils::tokenize(cfg.second.value(),
						" ");
				for (const auto& token : tokens) {
					configline.append(utils::quote(token) + " ");
				}
			} else {
				configline.append(utils::quote(cfg.second.value()));
				if (cfg.second.value() != cfg.second.default_value()) {
					configline.append(strprintf::fmt(
							" # default: %s",
							cfg.second.default_value()));
				}
			}
			break;
		case ConfigDataType::INVALID:
			// can't happen because we already checked this case
			// before the `switch`
			break;
		}
		config_output.push_back(configline);
	}
}

std::vector<std::string> ConfigContainer::get_suggestions(
	const std::string& fragment) const
{
	std::vector<std::string> result;
	std::lock_guard<std::recursive_mutex> guard(config_data_mtx);
	for (const auto& cfg : config_data) {
		if (cfg.first.substr(0, fragment.length()) == fragment) {
			result.push_back(cfg.first);
		}
	}
	std::sort(result.begin(), result.end());
	return result;
}

FeedSortStrategy ConfigContainer::get_feed_sort_strategy() const
{
	FeedSortStrategy ss;

	const auto setting = get_configvalue("feed-sort-order");
	if (setting.empty()) {
		return ss;
	}

	const auto sortmethod_info = utils::tokenize(setting, "-");
	const std::string sortmethod = sortmethod_info[0];

	if (sortmethod == "none") {
		ss.sm = FeedSortMethod::NONE;
	} else if (sortmethod == "firsttag") {
		ss.sm = FeedSortMethod::FIRST_TAG;
	} else if (sortmethod == "title") {
		ss.sm = FeedSortMethod::TITLE;
	} else if (sortmethod == "articlecount") {
		ss.sm = FeedSortMethod::ARTICLE_COUNT;
	} else if (sortmethod == "unreadarticlecount") {
		ss.sm = FeedSortMethod::UNREAD_ARTICLE_COUNT;
	} else if (sortmethod == "lastupdated") {
		ss.sm = FeedSortMethod::LAST_UPDATED;
	} else if (sortmethod == "latestunread") {
		ss.sm = FeedSortMethod::LATEST_UNREAD;
	}

	std::string direction = "desc";
	if (sortmethod_info.size() > 1) {
		direction = sortmethod_info[1];
	}

	if (direction == "asc") {
		ss.sd = SortDirection::ASC;
	} else if (direction == "desc") {
		ss.sd = SortDirection::DESC;
	}

	return ss;
}

ArticleSortStrategy ConfigContainer::get_article_sort_strategy() const
{
	ArticleSortStrategy ss;
	const auto methods =
		utils::tokenize(get_configvalue("article-sort-order"), "-");

	if (!methods.empty() &&
		methods[0] == "date") { // date is descending by default
		ss.sm = ArtSortMethod::DATE;
		ss.sd = SortDirection::DESC;
		if (methods.size() > 1 && methods[1] == "asc") {
			ss.sd = SortDirection::ASC;
		}
	} else { // all other sort methods are ascending by default
		ss.sd = SortDirection::ASC;
		if (methods.size() > 1 && methods[1] == "desc") {
			ss.sd = SortDirection::DESC;
		}
	}

	if (!methods.empty()) {
		if (methods[0] == "title") {
			ss.sm = ArtSortMethod::TITLE;
		} else if (methods[0] == "flags") {
			ss.sm = ArtSortMethod::FLAGS;
		} else if (methods[0] == "author") {
			ss.sm = ArtSortMethod::AUTHOR;
		} else if (methods[0] == "link") {
			ss.sm = ArtSortMethod::LINK;
		} else if (methods[0] == "guid") {
			ss.sm = ArtSortMethod::GUID;
		} else if (methods[0] == "random") {
			ss.sm = ArtSortMethod::RANDOM;
		}
	}

	return ss;
}

} // namespace newsboat

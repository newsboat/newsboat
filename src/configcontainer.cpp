#include "configcontainer.h"

#include <sstream>
#include <iostream>
#include <algorithm>
#include <cassert>
#include <sys/types.h>
#include <pwd.h>

#include "config.h"
#include "configparser.h"
#include "exceptions.h"
#include "logger.h"
#include "utils.h"
#include "strprintf.h"

namespace newsboat {

const std::string configcontainer::PARTIAL_FILE_SUFFIX = ".part";

configcontainer::configcontainer()
	// create the config options and set their resp. default value and type
	: config_data {
		{ "always-display-description",
		    configdata("false", configdata_t::BOOL) },
		{ "article-sort-order", configdata("date-asc", configdata_t::STR) },
		{ "articlelist-format",
		    configdata("%4i %f %D %6L  %?T?|%-17T|  &?%t", configdata_t::STR) },
		{ "auto-reload", configdata("no", configdata_t::BOOL) },
		{ "bookmark-autopilot", configdata("false", configdata_t::BOOL) },
		{ "bookmark-cmd", configdata("", configdata_t::STR) },
		{ "bookmark-interactive", configdata("false", configdata_t::BOOL) },
		{ "browser", configdata("lynx", configdata_t::PATH) },
		{ "cache-file", configdata("", configdata_t::PATH) },
		{ "cleanup-on-quit", configdata("yes", configdata_t::BOOL) },
		{ "confirm-exit", configdata("no", configdata_t::BOOL) },
		{ "cookie-cache", configdata("", configdata_t::PATH) },
		{ "datetime-format", configdata("%b %d", configdata_t::STR) },
		{ "delete-read-articles-on-quit",
		    configdata("false", configdata_t::BOOL) },
		{ "display-article-progress", configdata("yes", configdata_t::BOOL) },
		{ "download-full-page", configdata("false", configdata_t::BOOL) },
		{ "download-path", configdata("~/", configdata_t::PATH) },
		{ "download-retries", configdata("1", configdata_t::INT) },
		{ "download-timeout", configdata("30", configdata_t::INT) },
		{ "error-log", configdata("", configdata_t::PATH) },
		{ "external-url-viewer", configdata("", configdata_t::PATH) },
		{ "feed-sort-order", configdata("none-desc", configdata_t::STR) },
		{ "feedhq-flag-share", configdata("", configdata_t::STR) },
		{ "feedhq-flag-star", configdata("", configdata_t::STR) },
		{ "feedhq-login", configdata("", configdata_t::STR) },
		{ "feedhq-min-items", configdata("20", configdata_t::INT) },
		{ "feedhq-password", configdata("", configdata_t::STR) },
		{ "feedhq-passwordfile", configdata("", configdata_t::PATH) },
		{ "feedhq-passwordeval", configdata("", configdata_t::STR) },
		{ "feedhq-show-special-feeds", configdata("true", configdata_t::BOOL) },
		{ "feedhq-url", configdata("https://feedhq.org/", configdata_t::STR) },
		{ "feedlist-format", configdata("%4i %n %11u %t", configdata_t::STR) },
		{ "goto-first-unread", configdata("true", configdata_t::BOOL) },
		{ "goto-next-feed", configdata("yes", configdata_t::BOOL) },
		{ "history-limit", configdata("100", configdata_t::INT) },
		{ "html-renderer", configdata("internal", configdata_t::PATH) },
		{ "http-auth-method",
		    configdata("any", std::unordered_set<std::string>({
		        "any", "basic", "digest", "digest_ie", "gssnegotiate", "ntlm",
		        "anysafe" })) },
		{ "ignore-mode",
		    configdata("download", std::unordered_set<std::string>({
		        "download", "display" })) },
		{ "inoreader-login", configdata("", configdata_t::STR) },
		{ "inoreader-password", configdata("", configdata_t::STR) },
		{ "inoreader-passwordfile", configdata("", configdata_t::PATH) },
		{ "inoreader-passwordeval", configdata("", configdata_t::STR) },
		{ "inoreader-show-special-feeds", configdata("true", configdata_t::BOOL) },
		{ "inoreader-flag-share", configdata("", configdata_t::STR) },
		{ "inoreader-flag-star", configdata("", configdata_t::STR) },
		{ "inoreader-min-items", configdata("20", configdata_t::INT) },
		{ "keep-articles-days", configdata("0", configdata_t::INT) },
		{ "mark-as-read-on-hover", configdata("false", configdata_t::BOOL) },
		{ "max-browser-tabs", configdata("10", configdata_t::INT) },
		{ "markfeedread-jumps-to-next-unread",
		    configdata("false", configdata_t::BOOL) },
		{ "max-download-speed", configdata("0", configdata_t::INT) },
		{ "max-downloads", configdata("1", configdata_t::INT) },
		{ "max-items", configdata("0", configdata_t::INT) },
		{ "newsblur-login", configdata("", configdata_t::STR) },
		{ "newsblur-min-items", configdata("20", configdata_t::INT) },
		{ "newsblur-password", configdata("", configdata_t::STR) },
		{ "newsblur-passwordfile", configdata("", configdata_t::PATH) },
		{ "newsblur-passwordeval", configdata("", configdata_t::STR) },
		{ "newsblur-url",
		    configdata("https://newsblur.com", configdata_t::STR) },
		{ "notify-always", configdata("no", configdata_t::BOOL) },
		{ "notify-beep", configdata("no", configdata_t::BOOL) },
		{ "notify-format",
		    configdata(
		        _("newsboat: finished reload, %f unread "
		            "feeds (%n unread articles total)"),
		        configdata_t::STR) },
		{ "notify-program", configdata("", configdata_t::PATH) },
		{ "notify-screen", configdata("no", configdata_t::BOOL) },
		{ "notify-xterm", configdata("no", configdata_t::BOOL) },
		{ "oldreader-flag-share", configdata("", configdata_t::STR) },
		{ "oldreader-flag-star", configdata("", configdata_t::STR) },
		{ "oldreader-login", configdata("", configdata_t::STR) },
		{ "oldreader-min-items", configdata("20", configdata_t::INT) },
		{ "oldreader-password", configdata("", configdata_t::STR) },
		{ "oldreader-passwordfile", configdata("", configdata_t::PATH) },
		{ "oldreader-passwordeval", configdata("", configdata_t::STR) },
		{ "oldreader-show-special-feeds",
		    configdata("true", configdata_t::BOOL) },
		{ "openbrowser-and-mark-jumps-to-next-unread",
		    configdata("false", configdata_t::BOOL) },
		{ "opml-url", configdata("", configdata_t::STR, true) },
		{ "pager", configdata("internal", configdata_t::PATH) },
		{ "player", configdata("", configdata_t::PATH) },
		{ "podcast-auto-enqueue", configdata("no", configdata_t::BOOL) },
		{ "prepopulate-query-feeds", configdata("false", configdata_t::BOOL) },
		{ "ssl-verifyhost", configdata("true", configdata_t::BOOL) },
		{ "ssl-verifypeer", configdata("true", configdata_t::BOOL) },
		{ "proxy", configdata("", configdata_t::STR) },
		{ "proxy-auth", configdata("", configdata_t::STR) },
		{ "proxy-auth-method",
		    configdata("any", std::unordered_set<std::string>({
		        "any", "basic", "digest", "digest_ie", "gssnegotiate", "ntlm",
		        "anysafe" })) },
		{ "proxy-type",
		    configdata("http", std::unordered_set<std::string>({
		        "http", "socks4", "socks4a", "socks5", "socks5h" })) },
		{ "refresh-on-startup", configdata("no", configdata_t::BOOL) },
		{ "reload-only-visible-feeds", configdata("false", configdata_t::BOOL) },
		{ "reload-threads", configdata("1", configdata_t::INT) },
		{ "reload-time", configdata("60", configdata_t::INT) },
		{ "save-path", configdata("~/", configdata_t::PATH) },
		{ "search-highlight-colors",
		    configdata("black yellow bold", configdata_t::STR, true) },
		{ "show-keymap-hint", configdata("yes", configdata_t::BOOL) },
		{ "show-read-articles", configdata("yes", configdata_t::BOOL) },
		{ "show-read-feeds", configdata("yes", configdata_t::BOOL) },
		{ "suppress-first-reload", configdata("no", configdata_t::BOOL) },
		{ "swap-title-and-hints", configdata("no", configdata_t::BOOL) },
		{ "text-width", configdata("0", configdata_t::INT) },
		{ "toggleitemread-jumps-to-next-unread",
		    configdata("false", configdata_t::BOOL) },
		{ "ttrss-flag-publish", configdata("", configdata_t::STR) },
		{ "ttrss-flag-star", configdata("", configdata_t::STR) },
		{ "ttrss-login", configdata("", configdata_t::STR) },
		{ "ttrss-mode",
		    configdata("multi", std::unordered_set<std::string>({
		        "single", "multi" })) },
		{ "ttrss-password", configdata("", configdata_t::STR) },
		{ "ttrss-passwordfile", configdata("", configdata_t::PATH) },
		{ "ttrss-passwordeval", configdata("", configdata_t::STR) },
		{ "ttrss-url", configdata("", configdata_t::STR) },
		{ "ocnews-login", configdata("", configdata_t::STR) },
		{ "ocnews-password", configdata("", configdata_t::STR) },
		{ "ocnews-passwordfile", configdata("", configdata_t::PATH) },
		{ "ocnews-passwordeval", configdata("", configdata_t::STR) },
		{ "ocnews-flag-star", configdata("", configdata_t::STR) },
		{ "ocnews-url", configdata("", configdata_t::STR) },
		{ "urls-source",
		    configdata("local", std::unordered_set<std::string>({
		        "local", "opml", "oldreader", "ttrss", "newsblur",
		        "feedhq", "ocnews", "inoreader" })) },
		{ "use-proxy", configdata("no", configdata_t::BOOL) },
		{ "user-agent", configdata("", configdata_t::STR) },

		/* title formats: */
		{ "articlelist-title-format",
		    configdata(
		        _("%N %V - Articles in feed '%T' (%u unread, %t total) - %U"),
		        configdata_t::STR) },
		{ "dialogs-title-format",
		    configdata(_("%N %V - Dialogs"), configdata_t::STR) },
		{ "feedlist-title-format",
		    configdata(
		        _("%N %V - Your feeds (%u unread, %t total)%?T? - tag `%T'&?"),
		        configdata_t::STR) },
		{ "filebrowser-title-format",
		    configdata(
		        _("%N %V - %?O?Open File&Save File? - %f"),
		        configdata_t::STR) },
		{ "help-title-format",
		    configdata(_("%N %V - Help"), configdata_t::STR) },
		{ "itemview-title-format",
		    configdata(
		        _("%N %V - Article '%T' (%u unread, %t total)"),
		        configdata_t::STR) },
		{ "searchresult-title-format",
		    configdata(
		        _("%N %V - Search result (%u unread, %t total)"),
		        configdata_t::STR) },
		{ "selectfilter-title-format",
		    configdata(_("%N %V - Select Filter"), configdata_t::STR) },
		{ "selecttag-title-format",
		    configdata(_("%N %V - Select Tag"), configdata_t::STR) },
		{ "urlview-title-format",
		    configdata(_("%N %V - URLs"), configdata_t::STR) }
	}
{
}

configcontainer::~configcontainer() {
}

void configcontainer::register_commands(configparser& cfgparser) {
	// this registers the config options defined above in the configuration parser
	// -> if the resp. config option is encountered, it is passed to the configcontainer
	for (auto cfg : config_data) {
		cfgparser.register_handler(cfg.first, this);
	}
}

void configcontainer::handle_action(const std::string& action, const std::vector<std::string>& params) {
	configdata& cfgdata = config_data[action];

	// configdata_t::INVALID indicates that the action didn't exist, and that the returned object was created ad-hoc.
	if (cfgdata.type == configdata_t::INVALID) {
		LOG(level::WARN, "configcontainer::handle_action: unknown action %s", action);
		throw confighandlerexception(action_handler_status::INVALID_COMMAND);
	}

	LOG(level::DEBUG, "configcontainer::handle_action: action = %s, type = %u", action, cfgdata.type);

	if (params.size() < 1) {
		throw confighandlerexception(action_handler_status::TOO_FEW_PARAMS);
	}

	switch (cfgdata.type) {
	case configdata_t::BOOL:
		if (!is_bool(params[0]))
			throw confighandlerexception(strprintf::fmt(_("expected boolean value, found `%s' instead"), params[0]));
		cfgdata.value = params[0];
		break;

	case configdata_t::INT:
		if (!is_int(params[0]))
			throw confighandlerexception(strprintf::fmt(_("expected integer value, found `%s' instead"), params[0]));
		cfgdata.value = params[0];
		break;

	case configdata_t::ENUM:
		if (cfgdata.enum_values.find(params[0]) == cfgdata.enum_values.end())
			throw confighandlerexception(strprintf::fmt(_("invalid configuration value `%s'"), params[0]));
	// fall-through
	case configdata_t::STR:
	case configdata_t::PATH:
		if (cfgdata.multi_option)
			cfgdata.value = utils::join(params, " ");
		else
			cfgdata.value = params[0];
		break;

	case configdata_t::INVALID:
		// we already handled this at the beginning of the function
		break;
	}
}

bool configcontainer::is_bool(const std::string& s) {
	const char * bool_values[] = { "yes", "no", "true", "false", 0 };
	for (int i=0; bool_values[i] ; ++i) {
		if (s == bool_values[i])
			return true;
	}
	return false;
}

bool configcontainer::is_int(const std::string& s) {
	const char * s1 = s.c_str();
	for (; *s1; s1++) {
		if (!isdigit(*s1))
			return false;
	}
	return true;
}

std::string configcontainer::get_configvalue(const std::string& key) {
	std::string retval = config_data[key].value;
	if (config_data[key].type == configdata_t::PATH) {
		retval = utils::resolve_tilde(retval);
	}

	return retval;
}

int configcontainer::get_configvalue_as_int(const std::string& key) {
	std::istringstream is(config_data[key].value);
	int i;
	is >> i;
	return i;
}

bool configcontainer::get_configvalue_as_bool(const std::string& key) {
	std::string value = config_data[key].value;
	if (value == "true" || value == "yes")
		return true;
	return false;
}

void configcontainer::set_configvalue(const std::string& key, const std::string& value) {
	LOG(level::DEBUG,"configcontainer::set_configvalue(%s, %s) called", key, value);
	config_data[key].value = value;
}

void configcontainer::reset_to_default(const std::string& key) {
	config_data[key].value = config_data[key].default_value;
}

void configcontainer::toggle(const std::string& key) {
	if (config_data[key].type == configdata_t::BOOL) {
		set_configvalue(key, std::string(get_configvalue_as_bool(key) ? "false" : "true"));
	}
}

void configcontainer::dump_config(std::vector<std::string>& config_output) {
	for (auto cfg : config_data) {
		std::string configline = cfg.first + " ";
		assert(cfg.second.type != configdata_t::INVALID);
		switch (cfg.second.type) {
		case configdata_t::BOOL:
		case configdata_t::INT:
			configline.append(cfg.second.value);
			if (cfg.second.value != cfg.second.default_value)
				configline.append(strprintf::fmt(" # default: %s", cfg.second.default_value));
			break;
		case configdata_t::ENUM:
		case configdata_t::STR:
		case configdata_t::PATH:
			if (cfg.second.multi_option) {
				std::vector<std::string> tokens = utils::tokenize(cfg.second.value, " ");
				for (auto token : tokens) {
					configline.append(utils::quote(token) + " ");
				}
			} else {
				configline.append(utils::quote(cfg.second.value));
				if (cfg.second.value != cfg.second.default_value) {
					configline.append(strprintf::fmt(" # default: %s", cfg.second.default_value));
				}
			}
			break;
		case configdata_t::INVALID:
			// can't happen because we already checked this case before the `switch`
			break;
		}
		config_output.push_back(configline);
	}
}

std::vector<std::string> configcontainer::get_suggestions(const std::string& fragment) {
	std::vector<std::string> result;
	for (auto cfg : config_data) {
		if (cfg.first.substr(0, fragment.length()) == fragment)
			result.push_back(cfg.first);
	}
	std::sort(result.begin(), result.end());
	return result;
}

}

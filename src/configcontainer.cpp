#include <config.h>
#include <configcontainer.h>
#include <configparser.h>
#include <exceptions.h>
#include <logger.h>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <utils.h>
#include <cassert>

#include <sys/types.h>
#include <pwd.h>


namespace newsbeuter {

configcontainer::configcontainer()
	// create the config options and set their resp. default value and type
	: config_data {
		{ "always-display-description",
		    configdata("false", configdata::BOOL) },
		{ "article-sort-order", configdata("date-asc", configdata::STR) },
		{ "articlelist-format",
		    configdata("%4i %f %D %6L  %?T?|%-17T|  &?%t", configdata::STR) },
		{ "auto-reload", configdata("no", configdata::BOOL) },
		{ "bookmark-autopilot", configdata("false", configdata::BOOL) },
		{ "bookmark-cmd", configdata("", configdata::STR) },
		{ "bookmark-interactive", configdata("false", configdata::BOOL) },
		{ "browser", configdata("lynx", configdata::PATH) },
		{ "cache-file", configdata("", configdata::PATH) },
		{ "cleanup-on-quit", configdata("yes", configdata::BOOL) },
		{ "confirm-exit", configdata("no", configdata::BOOL) },
		{ "cookie-cache", configdata("", configdata::PATH) },
		{ "datetime-format", configdata("%b %d", configdata::STR) },
		{ "delete-read-articles-on-quit",
		    configdata("false", configdata::BOOL) },
		{ "display-article-progress", configdata("yes", configdata::BOOL) },
		{ "download-full-page", configdata("false", configdata::BOOL) },
		{ "download-path", configdata("~/", configdata::PATH) },
		{ "download-retries", configdata("1", configdata::INT) },
		{ "download-timeout", configdata("30", configdata::INT) },
		{ "error-log", configdata("", configdata::PATH) },
		{ "external-url-viewer", configdata("", configdata::PATH) },
		{ "feed-sort-order", configdata("none-desc", configdata::STR) },
		{ "feedhq-flag-share", configdata("", configdata::STR) },
		{ "feedhq-flag-star", configdata("", configdata::STR) },
		{ "feedhq-login", configdata("", configdata::STR) },
		{ "feedhq-min-items", configdata("20", configdata::INT) },
		{ "feedhq-password", configdata("", configdata::STR) },
		{ "feedhq-passwordfile", configdata("", configdata::PATH) },
		{ "feedhq-show-special-feeds", configdata("true", configdata::BOOL) },
		{ "feedhq-url", configdata("https://feedhq.org/", configdata::STR) },
		{ "feedlist-format", configdata("%4i %n %11u %t", configdata::STR) },
		{ "goto-first-unread", configdata("true", configdata::BOOL) },
		{ "goto-next-feed", configdata("yes", configdata::BOOL) },
		{ "history-limit", configdata("100", configdata::INT) },
		{ "html-renderer", configdata("internal", configdata::PATH) },
		{ "http-auth-method",
		    configdata("any", std::unordered_set<std::string>({
		        "any", "basic", "digest", "digest_ie", "gssnegotiate", "ntlm",
		        "anysafe" })) },
		{ "ignore-mode",
		    configdata("download", std::unordered_set<std::string>({
		        "download", "display" })) },
		{ "keep-articles-days", configdata("0", configdata::INT) },
		{ "mark-as-read-on-hover", configdata("false", configdata::BOOL) },
		{ "max-browser-tabs", configdata("10", configdata::INT) },
		{ "markfeedread-jumps-to-next-unread",
		    configdata("false", configdata::BOOL) },
		{ "max-download-speed", configdata("0", configdata::INT) },
		{ "max-downloads", configdata("1", configdata::INT) },
		{ "max-items", configdata("0", configdata::INT) },
		{ "newsblur-login", configdata("", configdata::STR) },
		{ "newsblur-min-items", configdata("20", configdata::INT) },
		{ "newsblur-password", configdata("", configdata::STR) },
		{ "newsblur-url",
		    configdata("https://newsblur.com", configdata::STR) },
		{ "notify-always", configdata("no", configdata::BOOL) },
		{ "notify-beep", configdata("no", configdata::BOOL) },
		{ "notify-format",
		    configdata(
		        _("newsbeuter: finished reload, %f unread "
		            "feeds (%n unread articles total)"),
		        configdata::STR) },
		{ "notify-program", configdata("", configdata::PATH) },
		{ "notify-screen", configdata("no", configdata::BOOL) },
		{ "notify-xterm", configdata("no", configdata::BOOL) },
		{ "oldreader-flag-share", configdata("", configdata::STR) },
		{ "oldreader-flag-star", configdata("", configdata::STR) },
		{ "oldreader-login", configdata("", configdata::STR) },
		{ "oldreader-min-items", configdata("20", configdata::INT) },
		{ "oldreader-password", configdata("", configdata::STR) },
		{ "oldreader-passwordfile", configdata("", configdata::PATH) },
		{ "oldreader-show-special-feeds",
		    configdata("true", configdata::BOOL) },
		{ "openbrowser-and-mark-jumps-to-next-unread",
		    configdata("false", configdata::BOOL) },
		{ "opml-url", configdata("", configdata::STR, true) },
		{ "pager", configdata("internal", configdata::PATH) },
		{ "player", configdata("", configdata::PATH) },
		{ "podcast-auto-enqueue", configdata("no", configdata::BOOL) },
		{ "prepopulate-query-feeds", configdata("false", configdata::BOOL) },
		{ "ssl-verify", configdata("true", configdata::BOOL) },
		{ "proxy", configdata("", configdata::STR) },
		{ "proxy-auth", configdata("", configdata::STR) },
		{ "proxy-auth-method",
		    configdata("any", std::unordered_set<std::string>({
		        "any", "basic", "digest", "digest_ie", "gssnegotiate", "ntlm",
		        "anysafe" })) },
		{ "proxy-type",
		    configdata("http", std::unordered_set<std::string>({
		        "http", "socks4", "socks4a", "socks5" })) },
		{ "refresh-on-startup", configdata("no", configdata::BOOL) },
		{ "reload-only-visible-feeds", configdata("false", configdata::BOOL) },
		{ "reload-threads", configdata("1", configdata::INT) },
		{ "reload-time", configdata("60", configdata::INT) },
		{ "save-path", configdata("~/", configdata::PATH) },
		{ "search-highlight-colors",
		    configdata("black yellow bold", configdata::STR, true) },
		{ "show-keymap-hint", configdata("yes", configdata::BOOL) },
		{ "show-read-articles", configdata("yes", configdata::BOOL) },
		{ "show-read-feeds", configdata("yes", configdata::BOOL) },
		{ "suppress-first-reload", configdata("no", configdata::BOOL) },
		{ "swap-title-and-hints", configdata("no", configdata::BOOL) },
		{ "text-width", configdata("0", configdata::INT) },
		{ "toggleitemread-jumps-to-next-unread",
		    configdata("false", configdata::BOOL) },
		{ "ttrss-flag-publish", configdata("", configdata::STR) },
		{ "ttrss-flag-star", configdata("", configdata::STR) },
		{ "ttrss-login", configdata("", configdata::STR) },
		{ "ttrss-mode",
		    configdata("multi", std::unordered_set<std::string>({
		        "single", "multi" })) },
		{ "ttrss-password", configdata("", configdata::STR) },
		{ "ttrss-passwordfile", configdata("", configdata::PATH) },
		{ "ttrss-url", configdata("", configdata::STR) },
		{ "ocnews-login", configdata("", configdata::STR) },
		{ "ocnews-password", configdata("", configdata::STR) },
		{ "ocnews-flag-star", configdata("", configdata::STR) },
		{ "ocnews-url", configdata("", configdata::STR) },
		{ "ocnews-verifyhost",
		    configdata("yes", configdata::BOOL) },
		{ "urls-source",
		    configdata("local", std::unordered_set<std::string>({
		        "local", "opml", "oldreader", "ttrss", "newsblur",
		        "feedhq", "ocnews" })) },
		{ "use-proxy", configdata("no", configdata::BOOL) },
		{ "user-agent", configdata("", configdata::STR) },

		/* title formats: */
		{ "articlelist-title-format",
		    configdata(
		        _("%N %V - Articles in feed '%T' (%u unread, %t total) - %U"),
		        configdata::STR) },
		{ "dialogs-title-format",
		    configdata(_("%N %V - Dialogs"), configdata::STR) },
		{ "feedlist-title-format",
		    configdata(
		        _("%N %V - Your feeds (%u unread, %t total)%?T? - tag `%T'&?"),
		        configdata::STR) },
		{ "filebrowser-title-format",
		    configdata(
		        _("%N %V - %?O?Open File&Save File? - %f"),
		        configdata::STR) },
		{ "help-title-format",
		    configdata(_("%N %V - Help"), configdata::STR) },
		{ "itemview-title-format",
		    configdata(
		        _("%N %V - Article '%T' (%u unread, %t total)"),
		        configdata::STR) },
		{ "searchresult-title-format",
		    configdata(
		        _("%N %V - Search result (%u unread, %t total)"),
		        configdata::STR) },
		{ "selectfilter-title-format",
		    configdata(_("%N %V - Select Filter"), configdata::STR) },
		{ "selecttag-title-format",
		    configdata(_("%N %V - Select Tag"), configdata::STR) },
		{ "urlview-title-format",
		    configdata(_("%N %V - URLs"), configdata::STR) }
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
	std::string resolved_action = lookup_alias(action);

	configdata& cfgdata = config_data[resolved_action];

	// configdata::INVALID indicates that the action didn't exist, and that the returned object was created ad-hoc.
	if (cfgdata.type == configdata::INVALID) {
		LOG(LOG_WARN, "configcontainer::handler_action: unknown action %s", action.c_str());
		throw confighandlerexception(AHS_INVALID_COMMAND);
	}

	LOG(LOG_DEBUG, "configcontainer::handle_action: action = %s, type = %u", action.c_str(), cfgdata.type);

	if (params.size() < 1) {
		throw confighandlerexception(AHS_TOO_FEW_PARAMS);
	}

	switch (cfgdata.type) {
	case configdata::BOOL:
		if (!is_bool(params[0]))
			throw confighandlerexception(utils::strprintf(_("expected boolean value, found `%s' instead"), params[0].c_str()));
		cfgdata.value = params[0];
		break;

	case configdata::INT:
		if (!is_int(params[0]))
			throw confighandlerexception(utils::strprintf(_("expected integer value, found `%s' instead"), params[0].c_str()));
		cfgdata.value = params[0];
		break;

	case configdata::ENUM:
		if (cfgdata.enum_values.find(params[0]) == cfgdata.enum_values.end())
			throw confighandlerexception(utils::strprintf(_("invalid configuration value `%s'"), params[0].c_str()));
	// fall-through
	case configdata::STR:
	case configdata::PATH:
		if (cfgdata.multi_option)
			cfgdata.value = utils::join(params, " ");
		else
			cfgdata.value = params[0];
		break;

	default:
		// should not happen
		throw confighandlerexception(AHS_INVALID_COMMAND);
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

std::string configcontainer::lookup_alias(const std::string& s) {
	// this assumes that the config_data table is consistent.
	std::string alias = s;
	while (alias != "" && config_data[alias].type == configdata::ALIAS) {
		alias = config_data[alias].default_value;
	}
	return alias;
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
	std::string retval = config_data[lookup_alias(key)].value;
	if (config_data[key].type == configdata::PATH) {
		retval = utils::resolve_tilde(retval);
	}

	return retval;
}

int configcontainer::get_configvalue_as_int(const std::string& key) {
	std::istringstream is(config_data[lookup_alias(key)].value);
	int i;
	is >> i;
	return i;
}

bool configcontainer::get_configvalue_as_bool(const std::string& key) {
	std::string value = config_data[lookup_alias(key)].value;
	if (value == "true" || value == "yes")
		return true;
	return false;
}

void configcontainer::set_configvalue(const std::string& key, const std::string& value) {
	LOG(LOG_DEBUG,"configcontainer::set_configvalue(%s [resolved: %s],%s) called", key.c_str(), lookup_alias(key).c_str(), value.c_str());
	config_data[lookup_alias(key)].value = value;
}

void configcontainer::reset_to_default(const std::string& key) {
	std::string resolved_key = lookup_alias(key);
	config_data[resolved_key].value = config_data[resolved_key].default_value;
}

void configcontainer::toggle(const std::string& key) {
	std::string resolved_key = lookup_alias(key);
	if (config_data[resolved_key].type == configdata::BOOL) {
		set_configvalue(resolved_key, std::string(get_configvalue_as_bool(resolved_key) ? "false" : "true"));
	}
}

void configcontainer::dump_config(std::vector<std::string>& config_output) {
	for (auto cfg : config_data) {
		std::string configline = cfg.first + " ";
		assert(cfg.second.type != configdata::INVALID);
		switch (cfg.second.type) {
		case configdata::BOOL:
		case configdata::INT:
			configline.append(cfg.second.value);
			if (cfg.second.value != cfg.second.default_value)
				configline.append(utils::strprintf(" # default: %s", cfg.second.default_value.c_str()));
			break;
		case configdata::ENUM:
		case configdata::STR:
		case configdata::PATH:
			if (cfg.second.multi_option) {
				std::vector<std::string> tokens = utils::tokenize(cfg.second.value, " ");
				for (auto token : tokens) {
					configline.append(utils::quote(token) + " ");
				}
			} else {
				configline.append(utils::quote(cfg.second.value));
				if (cfg.second.value != cfg.second.default_value) {
					configline.append(utils::strprintf(" # default: %s", cfg.second.default_value.c_str()));
				}
			}
			break;
		case configdata::ALIAS:
			// skip entry, generate no output
			continue;
		case configdata::INVALID:
		default:
			assert(0);
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

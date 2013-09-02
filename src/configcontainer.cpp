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


namespace newsbeuter
{

configdata::configdata(const std::string& v, ...) : value(v), default_value(v), type(ENUM) {
	va_list ap;
	va_start(ap, v);

	const char * arg;

	do {
		arg = va_arg(ap, const char *);
		if (arg) {
			enum_values.insert(arg);
		}
	} while (arg != NULL);

	va_end(ap);
}

configcontainer::configcontainer()
{
	// create the config options and set their resp. default value and type
	config_data["show-read-feeds"] = configdata("yes", configdata::BOOL);
	config_data["browser"]         = configdata("lynx", configdata::PATH);
	config_data["use-proxy"]       = configdata("no", configdata::BOOL);
	config_data["auto-reload"]     = configdata("no", configdata::BOOL);
	config_data["reload-time"]     = configdata("60", configdata::INT);
	config_data["max-items"]       = configdata("0", configdata::INT);
	config_data["save-path"]       = configdata("~/", configdata::PATH);
	config_data["download-path"]   = configdata("~/", configdata::PATH);
	config_data["max-downloads"]   = configdata("1", configdata::INT);
	config_data["podcast-auto-enqueue"] = configdata("no", configdata::BOOL);
	config_data["player"]          = configdata("", configdata::PATH);
	config_data["cleanup-on-quit"] = configdata("yes", configdata::BOOL);
	config_data["user-agent"]      = configdata("", configdata::STR);
	config_data["refresh-on-startup"] = configdata("no", configdata::BOOL);
	config_data["suppress-first-reload"] = configdata("no", configdata::BOOL);
	config_data["cache-file"]      = configdata("", configdata::PATH);
	config_data["proxy"]           = configdata("", configdata::STR);
	config_data["proxy-auth"]      = configdata("", configdata::STR);
	config_data["proxy-auth-method"] = configdata("any", "any", "basic", "digest", "digest_ie", "gssnegotiate", "ntlm", "anysafe", NULL);
	config_data["http-auth-method"] = configdata("any", "any", "basic", "digest", "digest_ie", "gssnegotiate", "ntlm", "anysafe", NULL);
	config_data["confirm-exit"]    = configdata("no", configdata::BOOL);
	config_data["error-log"]       = configdata("", configdata::PATH);
	config_data["notify-screen"]   = configdata("no", configdata::BOOL);
	config_data["notify-always"]   = configdata("no", configdata::BOOL);
	config_data["notify-xterm"]    = configdata("no", configdata::BOOL);
	config_data["notify-beep"]     = configdata("no", configdata::BOOL);
	config_data["notify-program"]  = configdata("", configdata::PATH);
	config_data["notify-format"]   = configdata(_("newsbeuter: finished reload, %f unread feeds (%n unread articles total)"), configdata::STR);
	config_data["datetime-format"] = configdata("%b %d", configdata::STR);
	config_data["urls-source"]     = configdata("local", "local", "opml", "googlereader", "ttrss", NULL); // enum
	config_data["bookmark-cmd"]    = configdata("", configdata::STR);
	config_data["opml-url"]        = configdata("", configdata::STR, true);
	config_data["html-renderer"]   = configdata("internal", configdata::PATH);
	config_data["feedlist-format"] = configdata("%4i %n %11u %t", configdata::STR);
	config_data["articlelist-format"] = configdata("%4i %f %D %6L  %?T?|%-17T|  &?%t", configdata::STR);
	config_data["text-width"]      = configdata("0", configdata::INT);
	config_data["always-display-description"] = configdata("false", configdata::BOOL);
	config_data["reload-only-visible-feeds"] = configdata("false", configdata::BOOL);
	config_data["article-sort-order"] = configdata("date-asc", configdata::STR);
	config_data["show-read-articles"] = configdata("yes", configdata::BOOL);
	config_data["goto-next-feed"] = configdata("yes", configdata::BOOL);
	config_data["display-article-progress"] = configdata("yes", configdata::BOOL);
	config_data["swap-title-and-hints"] = configdata("no", configdata::BOOL);
	config_data["show-keymap-hint"] = configdata("yes", configdata::BOOL);
	config_data["download-timeout"] = configdata("30", configdata::INT);
	config_data["download-retries"] = configdata("1", configdata::INT);
	config_data["feed-sort-order"] = configdata("none-desc", configdata::STR);
	config_data["reload-threads"] = configdata("1", configdata::INT);
	config_data["keep-articles-days"] = configdata("0", configdata::INT);
	config_data["bookmark-interactive"] = configdata("false", configdata::BOOL);
	config_data["bookmark-autopilot"] = configdata("false", configdata::BOOL);
	config_data["mark-as-read-on-hover"] = configdata("false", configdata::BOOL);
	config_data["search-highlight-colors"] = configdata("black yellow bold", configdata::STR, true);
	config_data["pager"] = configdata("internal", configdata::PATH);
	config_data["history-limit"] = configdata("100", configdata::INT);
	config_data["prepopulate-query-feeds"] = configdata("false", configdata::BOOL);
	config_data["goto-first-unread"] = configdata("true", configdata::BOOL);
	config_data["proxy-type"] = configdata("http", "http", "socks4", "socks4a", "socks5", NULL); // enum
	config_data["googlereader-login"] = configdata("", configdata::STR);
	config_data["googlereader-password"] = configdata("", configdata::STR);
	config_data["googlereader-passwordfile"] = configdata("", configdata::PATH);
	config_data["googlereader-flag-share"] = configdata("", configdata::STR);
	config_data["googlereader-flag-star"] = configdata("", configdata::STR);
	config_data["googlereader-show-special-feeds"] = configdata("true", configdata::BOOL);
	config_data["googlereader-min-items"] = configdata("20", configdata::INT);
	config_data["ignore-mode"] = configdata("download", "download", "display", NULL); // enum
	config_data["max-download-speed"] = configdata("0", configdata::INT);
	config_data["cookie-cache"] = configdata("", configdata::STR);
	config_data["download-full-page"] = configdata("false", configdata::BOOL);
	config_data["external-url-viewer"] = configdata("", configdata::STR);
	config_data["ttrss-login"] = configdata("", configdata::STR);
	config_data["ttrss-password"] = configdata("", configdata::STR);
	config_data["ttrss-passwordfile"] = configdata("", configdata::PATH);
	config_data["ttrss-url"] = configdata("", configdata::STR);
	config_data["ttrss-mode"] = configdata("multi", "single", "multi", NULL); // enum
	config_data["ttrss-flag-star"] = configdata("", configdata::STR);
	config_data["ttrss-flag-publish"] = configdata("", configdata::STR);
	config_data["delete-read-articles-on-quit"] = configdata("false", configdata::BOOL);
	config_data["openbrowser-and-mark-jumps-to-next-unread"] = configdata("false", configdata::BOOL);
	config_data["toggleitemread-jumps-to-next-unread"] = configdata("false", configdata::BOOL);
	config_data["markfeedread-jumps-to-next-unread"] = configdata("false", configdata::BOOL);

	/* title formats: */
	config_data["feedlist-title-format"] = configdata(_("%N %V - Your feeds (%u unread, %t total)%?T? - tag `%T'&?"), configdata::STR);
	config_data["articlelist-title-format"] = configdata(_("%N %V - Articles in feed '%T' (%u unread, %t total) - %U"), configdata::STR);
	config_data["searchresult-title-format"] = configdata(_("%N %V - Search result (%u unread, %t total)"), configdata::STR);
	config_data["filebrowser-title-format"] = configdata(_("%N %V - %?O?Open File&Save File? - %f"), configdata::STR);
	config_data["help-title-format"] = configdata(_("%N %V - Help"), configdata::STR);
	config_data["selecttag-title-format"] = configdata(_("%N %V - Select Tag"), configdata::STR);
	config_data["selectfilter-title-format"] = configdata(_("%N %V - Select Filter"), configdata::STR);
	config_data["itemview-title-format"] = configdata(_("%N %V - Article '%T' (%u unread, %t total)"), configdata::STR);
	config_data["urlview-title-format"] = configdata(_("%N %V - URLs"), configdata::STR);
	config_data["dialogs-title-format"] = configdata(_("%N %V - Dialogs"), configdata::STR);
}

configcontainer::~configcontainer()
{
}

void configcontainer::register_commands(configparser& cfgparser)
{
	// this registers the config options defined above in the configuration parser
	// -> if the resp. config option is encountered, it is passed to the configcontainer
	for (std::map<std::string,configdata>::iterator it=config_data.begin();it!=config_data.end();++it) {
		cfgparser.register_handler(it->first, this);
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
	for (;*s1;s1++) {
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
	for (std::map<std::string, configdata>::iterator it=config_data.begin();it!=config_data.end();++it) {
		std::string configline = it->first + " ";
		assert(it->second.type != configdata::INVALID);
		switch (it->second.type) {
		case configdata::BOOL:
		case configdata::INT:
			configline.append(it->second.value);
			if (it->second.value != it->second.default_value)
				configline.append(utils::strprintf(" # default: %s", it->second.default_value.c_str()));
			break;
		case configdata::ENUM:
		case configdata::STR:
		case configdata::PATH:
			if (it->second.multi_option) {
				std::vector<std::string> tokens = utils::tokenize(it->second.value, " ");
				for (std::vector<std::string>::iterator it=tokens.begin();it!=tokens.end();++it) {
					configline.append(utils::quote(*it) + " ");
				}
			} else {
				configline.append(utils::quote(it->second.value));
				if (it->second.value != it->second.default_value) {
					configline.append(utils::strprintf(" # default: %s", it->second.default_value.c_str()));
				}
			}
			break;
		case configdata::ALIAS:
			// skip entry, generate no output
			continue;
		case configdata::INVALID:
			assert(0);
			break;
		}
		config_output.push_back(configline);
	}
}

std::vector<std::string> configcontainer::get_suggestions(const std::string& fragment) {
	std::vector<std::string> result;
	for (std::map<std::string, configdata>::iterator it=config_data.begin();it!=config_data.end();++it) {
		if (it->first.substr(0, fragment.length()) == fragment)
			result.push_back(it->first);
	}
	std::sort(result.begin(), result.end());
	return result;
}

}

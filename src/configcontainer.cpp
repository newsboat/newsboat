#include "configcontainer.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <pwd.h>
#include <sstream>
#include <sys/types.h>

#include "config.h"
#include "configparser.h"
#include "confighandlerexception.h"
#include "logger.h"
#include "strprintf.h"
#include "utils.h"

namespace newsboat {

const std::string ConfigContainer::PARTIAL_FILE_SUFFIX = "part";

ConfigContainer::ConfigContainer()
	: rs_object(newsboat::configcontainer::bridged::create())
{
}

ConfigContainer::~ConfigContainer() = default;

void ConfigContainer::register_commands(ConfigParser& cfgparser)
{
	// Empty string in get_suggestions() returns all available keys which we need to register
	const auto keys = newsboat::configcontainer::bridged::get_suggestions(*rs_object, "");
	for (const auto& key : keys) {
		cfgparser.register_handler(std::string(key), *this);
	}
}

void ConfigContainer::handle_action(const std::string& action,
	const std::vector<std::string>& params)
{
	rust::Vec<rust::String> rs_params;
	for (const auto& p : params) {
		rs_params.push_back(p);
	}

	const auto result = newsboat::configcontainer::bridged::handle_action(*rs_object, action,
			rs_params);

	// Map status codes from Rust (ConfigHandlerStatus) to C++ ActionHandlerStatus
	switch (result.status) {
	case 0: // Success
		break;
	case 1: // InvalidCommand
		throw ConfigHandlerException(ActionHandlerStatus::INVALID_COMMAND);
	case 2: // TooFewParams
		throw ConfigHandlerException(ActionHandlerStatus::TOO_FEW_PARAMS);
	case 3: // TooManyParams
		throw ConfigHandlerException(ActionHandlerStatus::TOO_MANY_PARAMS);
	case 4: // InvalidParams
		throw ConfigHandlerException(std::string(result.msg));
	default:
		throw ConfigHandlerException(std::string("Unknown error from Rust"));
	}
}

std::string ConfigContainer::get_configvalue(const std::string& key) const
{
	return std::string(newsboat::configcontainer::bridged::get_configvalue(*rs_object, key));
}

int ConfigContainer::get_configvalue_as_int(const std::string& key) const
{
	return newsboat::configcontainer::bridged::get_configvalue_as_int(*rs_object, key);
}

Filepath ConfigContainer::get_configvalue_as_filepath(const std::string& key) const
{
	return Filepath(newsboat::configcontainer::bridged::get_configvalue_as_filepath(*rs_object,
				key));
}

bool ConfigContainer::get_configvalue_as_bool(const std::string& key) const
{
	return newsboat::configcontainer::bridged::get_configvalue_as_bool(*rs_object, key);
}

nonstd::expected<void, std::string> ConfigContainer::set_configvalue(
	const std::string& key,
	const std::string& value)
{
	rust::String error_message;
	if (newsboat::configcontainer::bridged::set_configvalue(*rs_object, key, value,
			error_message)) {
		return {};
	} else {
		return nonstd::make_unexpected(std::string(error_message));
	}
}

void ConfigContainer::reset_to_default(const std::string& key)
{
	newsboat::configcontainer::bridged::reset_to_default(*rs_object, key);
}

void ConfigContainer::toggle(const std::string& key)
{
	newsboat::configcontainer::bridged::toggle(*rs_object, key);
}

void ConfigContainer::dump_config(std::vector<std::string>& config_output) const
{
	const auto lines = newsboat::configcontainer::bridged::dump_config(*rs_object);
	for (const auto& line : lines) {
		config_output.push_back(std::string(line));
	}
}

std::vector<std::string> ConfigContainer::get_suggestions(
	const std::string& fragment) const
{
	const auto rs_suggs = newsboat::configcontainer::bridged::get_suggestions(*rs_object,
			fragment);
	std::vector<std::string> result;
	for (const auto& s : rs_suggs) {
		result.push_back(std::string(s));
	}
	return result;
}

FeedSortStrategy ConfigContainer::get_feed_sort_strategy() const
{
	FeedSortStrategy ss;
	const auto values = newsboat::configcontainer::bridged::get_feed_sort_strategy_values(
			*rs_object);
	assert(values.size() >= 2);
	ss.sm = static_cast<FeedSortMethod>(values[0]);
	ss.sd = static_cast<SortDirection>(values[1]);
	return ss;
}

ArticleSortStrategy ConfigContainer::get_article_sort_strategy() const
{
	ArticleSortStrategy ss;
	const auto values = newsboat::configcontainer::bridged::get_article_sort_strategy_values(
			*rs_object);
	assert(values.size() >= 2);
	ss.sm = static_cast<ArtSortMethod>(values[0]);
	ss.sd = static_cast<SortDirection>(values[1]);
	return ss;
}

} // namespace newsboat

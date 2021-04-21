#include "configdata.h"

#include <algorithm>
#include <ctype.h>
#include <vector>

#include "config.h"
#include "strprintf.h"

bool is_bool(const std::string& s)
{
	const auto bool_values = std::vector<std::string>(
	{"yes", "no", "true", "false"});
	return (std::find(bool_values.begin(), bool_values.end(), s) !=
			bool_values.end());
}

bool is_int(const std::string& s)
{
	return std::all_of(s.begin(), s.end(), ::isdigit);
}

namespace newsboat {

ConfigData::ConfigData(const std::string& v, ConfigDataType t,
	bool multi_option)
	: value_(Utf8String::from_utf8(v))
	, default_value_(Utf8String::from_utf8(v))
	, type_(t)
	, multi_option_(multi_option)
{
}

ConfigData::ConfigData(const std::string& v,
	const std::unordered_set<std::string>& values)
	: value_(Utf8String::from_utf8(v))
	, default_value_(Utf8String::from_utf8(v))
	, type_(ConfigDataType::ENUM)
	, multi_option_(false)
{
	for (const auto& value : values) {
		enum_values_.insert(Utf8String::from_utf8(value));
	}
}

nonstd::expected<void, std::string> ConfigData::set_value(
	std::string new_value)
{
	switch (type_) {
	case ConfigDataType::BOOL:
		if (is_bool(new_value)) {
			value_ = Utf8String::from_utf8(std::move(new_value));
		} else {
			return nonstd::make_unexpected(
					strprintf::fmt(
						_("expected boolean value, found `%s' instead"),
						new_value));
		}
		break;

	case ConfigDataType::INT:
		if (is_int(new_value)) {
			value_ = Utf8String::from_utf8(std::move(new_value));
		} else {
			return nonstd::make_unexpected(strprintf::fmt(
						_("expected integer value, found `%s' instead"),
						new_value));
		}
		break;

	case ConfigDataType::ENUM:
		if (enum_values_.find(Utf8String::from_utf8(new_value)) != enum_values_.end()) {
			value_ = Utf8String::from_utf8(std::move(new_value));
		} else {
			return nonstd::make_unexpected(strprintf::fmt(
						_("invalid configuration value `%s'"),
						new_value));
		}
		break;

	case ConfigDataType::STR:
	case ConfigDataType::PATH:
		value_ = Utf8String::from_utf8(std::move(new_value));
		break;

	case ConfigDataType::INVALID:
		assert("unreachable");
		break;
	}


	return {};
}

} // namespace newsboat

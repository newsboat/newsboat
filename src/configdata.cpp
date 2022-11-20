#include "configdata.h"

#include <algorithm>
#include <ctype.h>
#include <vector>

#include "config.h"
#include "strprintf.h"

bool is_bool(const newsboat::Utf8String& s)
{
	const auto bool_values = std::vector<newsboat::Utf8String>(
	{"yes", "no", "true", "false"});
	return (std::find(bool_values.begin(), bool_values.end(), s) !=
			bool_values.end());
}

bool is_int(const newsboat::Utf8String& s)
{
	return std::all_of(s.utf8().begin(), s.utf8().end(), ::isdigit);
}

namespace newsboat {

ConfigData::ConfigData(const Utf8String& v, ConfigDataType t,
	bool multi_option)
	: value_(v)
	, default_value_(v)
	, type_(t)
	, multi_option_(multi_option)
{
}

ConfigData::ConfigData(const Utf8String& v,
	const std::unordered_set<Utf8String>& values)
	: value_(v)
	, default_value_(v)
	, type_(ConfigDataType::ENUM)
	, enum_values_(values)
	, multi_option_(false)
{
}

nonstd::expected<void, Utf8String> ConfigData::set_value(
	Utf8String new_value)
{
	switch (type_) {
	case ConfigDataType::BOOL:
		if (is_bool(new_value)) {
			value_ = std::move(new_value);
		} else {
			return nonstd::make_unexpected(
					strprintf::fmt(
						_("expected boolean value, found `%s' instead"),
						new_value));
		}
		break;

	case ConfigDataType::INT:
		if (is_int(new_value)) {
			value_ = std::move(new_value);
		} else {
			return nonstd::make_unexpected(strprintf::fmt(
						_("expected integer value, found `%s' instead"),
						new_value));
		}
		break;

	case ConfigDataType::ENUM:
		if (enum_values_.find(new_value) != enum_values_.end()) {
			value_ = std::move(new_value);
		} else {
			return nonstd::make_unexpected(strprintf::fmt(
						_("invalid configuration value `%s'"),
						new_value));
		}
		break;

	case ConfigDataType::STR:
	case ConfigDataType::PATH:
		value_ = std::move(new_value);
		break;

	case ConfigDataType::INVALID:
		assert("unreachable");
		break;
	}


	return {};
}

} // namespace newsboat

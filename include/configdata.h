#ifndef NEWSBOAT_CONFIGDATA_H_
#define NEWSBOAT_CONFIGDATA_H_

#include <string>
#include <unordered_set>

#include "3rd-party/expected.hpp"

#include "utf8string.h"

namespace newsboat {

enum class ConfigDataType { INVALID, BOOL, INT, STR, PATH, ENUM };

/// Data about a setting: its default and current values, its type, and its
/// allowed values if it's an enum.
class ConfigData {
public:
	/// Construct a value of type `t` and equal to `v`. If `multi_option` is
	/// true and config file contains this option multiple times, the values
	/// will be combined instead of rewritten.
	ConfigData(const std::string& v = "",
		ConfigDataType t = ConfigDataType::INVALID, bool multi_option = false);

	/// Construct a value that can take any of given `values`, and currently
	/// equal to `v` (which *must* be one of `values`).
	ConfigData(const std::string& v, const std::unordered_set<std::string>& values);

	/// Current value of the setting.
	std::string value() const
	{
		return value_.to_utf8();
	}

	/// Change the setting's value to `new_value`.
	///
	/// On error, returns internationalized error message.
	nonstd::expected<void, std::string> set_value(std::string new_value);

	/// Default value of the setting.
	std::string default_value() const
	{
		return default_value_.to_utf8();
	}

	/// Change the setting's value to the default one.
	void reset_to_default()
	{
		value_ = default_value_;
	}

	/// The type of this setting.
	ConfigDataType type() const
	{
		return type_;
	}

	/// Possible values of this setting if `type` is `ENUM`.
	// FIXME(utf8): change this back to const std::unordered_set<...>&
	std::unordered_set<std::string> enum_values() const
	{
		std::unordered_set<std::string> result;
		for (const auto& value : enum_values_) {
			result.insert(value.to_utf8());
		}
		return result;
	}

	/// If `true`, multiple values of this setting should be combined instead
	/// of rewritten.
	bool multi_option() const
	{
		return multi_option_;
	}

private:
	Utf8String value_;
	Utf8String default_value_;
	ConfigDataType type_;
	std::unordered_set<Utf8String> enum_values_;
	bool multi_option_;
};

} // namespace newsboat

#endif /* NEWSBOAT_CONFIGDATA_H_ */


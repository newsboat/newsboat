#ifndef NEWSBOAT_CONFIGDATA_H_
#define NEWSBOAT_CONFIGDATA_H_

#include <string>
#include <unordered_set>

#include "3rd-party/expected.hpp"

namespace newsboat {

enum class ConfigDataType { BOOL, INT, STR, PATH, ENUM };

/// Data about a setting: its default and current values, its type, and its
/// allowed values if it's an enum.
class ConfigData {
public:
	/// Construct a value of type `t` and equal to `v`. If `multi_option` is
	/// true and config file contains this option multiple times, the values
	/// will be combined instead of rewritten.
	ConfigData(const std::string& v, ConfigDataType t, bool multi_option = false);

	/// Construct a value that can take any of given `values`, and currently
	/// equal to `v` (which *must* be one of `values`).
	ConfigData(const std::string& v, const std::unordered_set<std::string>& values);

	/// Current value of the setting.
	std::string value() const
	{
		return value_;
	}

	/// Change the setting's value to `new_value`.
	///
	/// On error, returns internationalized error message.
	nonstd::expected<void, std::string> set_value(std::string new_value);

	/// Default value of the setting.
	std::string default_value() const
	{
		return default_value_;
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
	const std::unordered_set<std::string>& enum_values() const
	{
		return enum_values_;
	}

	/// If `true`, multiple values of this setting should be combined instead
	/// of rewritten.
	bool multi_option() const
	{
		return multi_option_;
	}

private:
	std::string value_;
	std::string default_value_;
	ConfigDataType type_;
	std::unordered_set<std::string> enum_values_;
	bool multi_option_;
};

} // namespace newsboat

#endif /* NEWSBOAT_CONFIGDATA_H_ */


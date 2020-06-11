#ifndef NEWSBOAT_CONFIGDATA_H_
#define NEWSBOAT_CONFIGDATA_H_

#include <string>
#include <unordered_set>

namespace newsboat {

enum class ConfigDataType { INVALID, BOOL, INT, STR, PATH, ENUM };

/// Data about a setting: its default and current values, its type, and its
/// allowed values if it's an enum.
struct ConfigData {
	/// Construct a value of type `t` and equal to `v`. If `multi_option` is
	/// true and config file contains this option multiple times, the values
	/// will be combined instead of rewritten.
	ConfigData(const std::string& v = "",
		ConfigDataType t = ConfigDataType::INVALID, bool multi_option = false);

	/// Construct a value that can take any of given `values`, and currently
	/// equal to `v` (which *must* be one of `values`).
	ConfigData(const std::string& v, const std::unordered_set<std::string>& values);

	/// Current value of the setting.
	std::string value;

	/// Default value of the setting.
	std::string default_value;

	/// The type of this setting.
	ConfigDataType type;

	/// Possible values of this setting if `type` is `ENUM`.
	const std::unordered_set<std::string> enum_values;

	/// If `true`, multiple values of this setting should be combined instead
	/// of rewritten.
	bool multi_option;
};

} // namespace newsboat

#endif /* NEWSBOAT_CONFIGDATA_H_ */


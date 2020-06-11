#include "configdata.h"

namespace newsboat {

ConfigData::ConfigData(const std::string& v, ConfigDataType t,
	bool multi_option)
	: value(v)
	, default_value(v)
	, type(t)
	, enum_values()
	, multi_option(multi_option)
{
}

ConfigData::ConfigData(const std::string& v,
	const std::unordered_set<std::string>& values)
	: value(v)
	, default_value(v)
	, type(ConfigDataType::ENUM)
	, enum_values(values)
	, multi_option(false)
{
}

} // namespace newsboat

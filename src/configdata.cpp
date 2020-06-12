#include "configdata.h"

namespace newsboat {

ConfigData::ConfigData(const std::string& v, ConfigDataType t,
	bool multi_option)
	: value_(v)
	, default_value_(v)
	, type_(t)
	, multi_option_(multi_option)
{
}

ConfigData::ConfigData(const std::string& v,
	const std::unordered_set<std::string>& values)
	: value_(v)
	, default_value_(v)
	, type_(ConfigDataType::ENUM)
	, enum_values_(values)
	, multi_option_(false)
{
}

} // namespace newsboat

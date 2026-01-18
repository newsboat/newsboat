#include "textstyle.h"

#include "config.h"
#include "confighandlerexception.h"
#include "strprintf.h"
#include "utils.h"

namespace newsboat {

TextStyle::TextStyle(const std::string& fgcolor, const std::string& bgcolor,
	const std::vector<std::string>& attributes)
	: fgcolor(fgcolor)
	, bgcolor(bgcolor)
	, attributes(attributes)
{
	if (!utils::is_valid_color(fgcolor)) {
		throw ConfigHandlerException(strprintf::fmt(
				_("`%s' is not a valid color"),
				fgcolor));
	}
	if (!utils::is_valid_color(bgcolor)) {
		throw ConfigHandlerException(strprintf::fmt(
				_("`%s' is not a valid color"),
				bgcolor));
	}

	for (const auto& attribute : attributes) {
		if (!utils::is_valid_attribute(attribute)) {
			throw ConfigHandlerException(
				strprintf::fmt(
					_("`%s' is not a valid attribute"),
					attribute));
		}
	}
}

const std::string& TextStyle::get_fgcolor() const
{
	return fgcolor;
}

const std::string& TextStyle::get_bgcolor() const
{
	return bgcolor;
}

const std::vector<std::string>& TextStyle::get_attributes() const
{
	return attributes;
}


std::string TextStyle::get_stfl_style_string() const
{
	std::string result;

	if (fgcolor != "default") {
		result.append("fg=");
		result.append(fgcolor);
	}
	if (bgcolor != "default") {
		if (!result.empty()) {
			result.append(",");
		}
		result.append("bg=");
		result.append(bgcolor);
	}
	for (const auto& attribute : attributes) {
		if (attribute != "default") {
			if (!result.empty()) {
				result.append(",");
			}
			result.append("attr=");
			result.append(attribute);
		}
	}

	return result;
}

} // namespace newsboat

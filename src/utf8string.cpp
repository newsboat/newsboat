#include "utf8string.h"

#include "utils.h"

namespace newsboat {

Utf8String::Utf8String(std::string input)
{
	inner = std::move(input);
}

Utf8String Utf8String::from_utf8(std::string input)
{
	return Utf8String(std::move(input));
}

Utf8String Utf8String::from_locale_charset(std::string input)
{
	return Utf8String(utils::locale_to_utf8(std::move(input)));
}

std::string Utf8String::to_locale_charset() const
{
	return utils::utf8_to_locale(inner);
}

} // namespace newsboat

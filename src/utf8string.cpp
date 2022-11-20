#include "utf8string.h"

#include <stfl.h>
#include <wctype.h>

#include "rust/cxx.h"

#include "utils.h"

namespace newsboat {

Utf8String::Utf8String(const rust::String& input)
{
	inner = std::string(input);
}

Utf8String::Utf8String(const rust::Str& input)
{
	inner = std::string(input);
}

Utf8String::operator rust::String() const
{
	return rust::String(inner);
}

Utf8String::operator rust::Str() const
{
	return rust::Str(inner);
}

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
	return Utf8String(utils::locale_to_utf8(input));
}

std::string Utf8String::to_locale_charset() const
{
	return utils::utf8_to_locale(inner);
}

} // namespace newsboat
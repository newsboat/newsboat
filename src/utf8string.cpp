#include "utf8string.h"

#include <stfl.h>
#include <wctype.h>

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

Utf8String Utf8String::to_lowercase() const
{
	struct stfl_ipool* ipool = stfl_ipool_create("UTF-8");
	std::wstring widestr = stfl_ipool_towc(ipool, inner.c_str());
	stfl_ipool_destroy(ipool);

	std::transform(widestr.begin(),
		widestr.end(),
		widestr.begin(),
		::towlower);

	ipool = stfl_ipool_create("UTF-8");
	const std::string result = stfl_ipool_fromwc(ipool, widestr.c_str());
	stfl_ipool_destroy(ipool);

	return Utf8String::from_utf8(result);
}

} // namespace newsboat

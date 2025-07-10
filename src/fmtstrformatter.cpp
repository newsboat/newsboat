#include "fmtstrformatter.h"

namespace Newsboat {

FmtStrFormatter::FmtStrFormatter()
	: rs_object(fmtstrformatter::bridged::create())
{
}

void FmtStrFormatter::register_fmt(char f, const std::string& value)
{
	fmtstrformatter::bridged::register_fmt(*rs_object, f, value);
}

std::string FmtStrFormatter::do_format(const std::string& fmt,
	unsigned int width)
{
	auto formatted = fmtstrformatter::bridged::do_format(*rs_object, fmt, width);
	return std::string(formatted);
}

} // namespace Newsboat

#include "fmtstrformatter.h"

#include <cstdlib>
#include <sstream>
#include <vector>

#include "logger.h"
#include "ruststring.h"

namespace newsboat {

FmtStrFormatter::FmtStrFormatter()
	: rs_object(fmtstrformatter::bridged::create())
{
}

void FmtStrFormatter::register_fmt(char f, const std::string& value)
{
	std::string key(1, f);
	fmtstrformatter::bridged::register_fmt(*rs_object, key, value);
}

std::string FmtStrFormatter::do_format(const std::string& fmt,
	unsigned int width)
{
	auto formatted = fmtstrformatter::bridged::do_format(*rs_object, fmt, width);
	return std::string(formatted);
}

} // namespace newsboat

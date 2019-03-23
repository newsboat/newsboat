#include "fmtstrformatter.h"

#include <cstdlib>
#include <sstream>
#include <vector>

#include "logger.h"
#include "rs_utils.h"
#include "utils.h"

extern "C" {
	void* rs_fmtstrformatter_new();

	void rs_fmtstrformatter_free(void* fmt);

	void rs_fmtstrformatter_register_fmt(
		void* fmt,
		char key,
		const char* value);

	char* rs_fmtstrformatter_do_format(
		void* fmt,
		const char* format,
		std::uint32_t width);
}

namespace newsboat {

FmtStrFormatter::FmtStrFormatter()
{
	rs_fmt = rs_fmtstrformatter_new();
}

FmtStrFormatter::~FmtStrFormatter()
{
	rs_fmtstrformatter_free(rs_fmt);
}

void FmtStrFormatter::register_fmt(char f, const std::string& value)
{
	rs_fmtstrformatter_register_fmt(rs_fmt, f, value.c_str());
}

std::string FmtStrFormatter::do_format(const std::string& fmt,
	unsigned int width)
{
	return RustString(rs_fmtstrformatter_do_format(rs_fmt, fmt.c_str(), width));
}

} // namespace newsboat

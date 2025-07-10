#include "strprintf.h"

using namespace Newsboat;

std::string strprintf::fmt(const std::string& format)
{
	char buffer[1024];
	std::string result;
	// Empty string is a dummy value that we pass in order to
	// silence Clang's warning about format not being a literal.
	//
	// The thing is, at this point we know *for sure* that the
	// format either contains no formats at all, or only escaped
	// percent signs (which don't require any additional arguments
	// to snprintf). It's just the way fmt recurses. The only reason
	// we're calling snprintf at all is to process these escaped
	// percent signs, if any. So we don't need additional
	// parameters.
	unsigned int len = 1 +
		snprintf(buffer, sizeof(buffer), format.c_str(), "");
	if (len <= sizeof(buffer)) {
		result = buffer;
	} else {
		std::vector<char> buf(len);
		snprintf(buf.data(), len, format.c_str(), "");
		result = buf.data();
	}
	return result;
}


/* Splits a printf-like format string into two parts, where first part contains
 * at most one format while the second part contains the rest of the input
 * string:
 *
 * "hello %i world %s haha"       =>  { "hello %i world ", "%s haha" }
 * "a 100%% rel%iable e%xamp%le"  =>  { "a 100%% rel%iable e", "%xamp%le" }
 */
std::pair<std::string, std::string> strprintf::split_format(
	const std::string& printf_format)
{
	std::string first_format, rest;

	std::string::size_type startpos = 0, next_sign_pos = 0;
	bool found_escaped_percent_sign = false;
	do {
		found_escaped_percent_sign = false;

		startpos = printf_format.find_first_of('%', startpos);
		next_sign_pos = printf_format.find_first_of('%', startpos + 1);
		if (next_sign_pos - startpos == 1) {
			found_escaped_percent_sign = true;
			startpos = next_sign_pos + 1;
		}
	} while (found_escaped_percent_sign);

	first_format = printf_format.substr(0, next_sign_pos);

	if (next_sign_pos != std::string::npos) {
		rest = printf_format.substr(next_sign_pos);
	}

	return {first_format, rest};
}

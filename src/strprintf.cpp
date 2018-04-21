#include "strprintf.h"

using namespace newsboat;

/* Splits a printf-like format string into two parts, where first part contains
 * at most one format while the second part contains the rest of the input
 * string:
 *
 * "hello %i world %s haha"       =>  { "hello %i world ", "%s haha" }
 * "a 100%% rel%iable e%xamp%le"  =>  { "a 100%% rel%iable e", "%xamp%le" }
 */
std::pair<std::string, std::string>
strprintf::split_format(const std::string& printf_format) {
	std::string first_format, rest;

	std::string::size_type startpos = 0, next_sign_pos = 0;
	bool found_escaped_percent_sign = false;
	do {
		found_escaped_percent_sign = false;

		startpos = printf_format.find_first_of('%', startpos);
		next_sign_pos = printf_format.find_first_of('%', startpos+1);
		if (next_sign_pos - startpos == 1) {
			found_escaped_percent_sign = true;
			startpos = next_sign_pos+1;
		}
	} while (found_escaped_percent_sign);

	first_format = printf_format.substr(0, next_sign_pos);

	if (next_sign_pos != std::string::npos) {
		rest = printf_format.substr(next_sign_pos);
	}

	return { first_format, rest };
}

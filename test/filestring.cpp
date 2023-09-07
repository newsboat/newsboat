#include "3rd-party/catch.hpp"
#include "filestring.h"

TEST_CASE("vector constructor", "[invalid utf8]")
{
	std::vector<std::uint8_t> invalid_utf8{0b11100000, 0b10000000, 0b11000000};
	std::string res(invalid_utf8.begin(), invalid_utf8.end());
	res[2] = '\x80'; // set invalid byte to \x80

	FileString filestring(invalid_utf8);
	REQUIRE(filestring.to_utf8() == res);
}

TEST_CASE("string constructor", "[invalid utf8]")
{
	std::vector<std::uint8_t> invalid_utf8{0b11100000, 0b10000000, 0b11000000};
	std::string invalid_utf8_string(invalid_utf8.begin(), invalid_utf8.end());
	std::string fixed_utf8_string = invalid_utf8_string;
	fixed_utf8_string[2] = '\x80';

	FileString filestring(invalid_utf8_string);
	REQUIRE(filestring.to_utf8() == fixed_utf8_string);
}


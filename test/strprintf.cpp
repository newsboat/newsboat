#include "strprintf.h"

#include <limits>

#include "3rd-party/catch.hpp"

using namespace newsboat;

TEST_CASE("strprintf::fmt()", "[strprintf]")
{
	REQUIRE(strprintf::fmt("") == "");
	REQUIRE(strprintf::fmt("%s", "") == "");
	REQUIRE(strprintf::fmt("%u", 0) == "0");
	REQUIRE(strprintf::fmt("%s", nullptr) == "(null)");
	REQUIRE(strprintf::fmt("%u-%s-%c", 23, "hello world", 'X') ==
		"23-hello world-X");
	REQUIRE(strprintf::fmt("%%") == "%");

	std::string long_input_without_formats(240000, 'x');
	REQUIRE(strprintf::fmt(long_input_without_formats) ==
		long_input_without_formats);
}

TEST_CASE("strprintf::split_format()", "[strprintf]")
{
	std::string first, rest;

	SECTION("empty format string")
	{
		std::tie(first, rest) = strprintf::split_format("");
		REQUIRE(first == "");
		REQUIRE(rest == "");
	}

	SECTION("string without formats")
	{
		const std::string input = "hello world!";
		std::tie(first, rest) = strprintf::split_format(input);
		REQUIRE(first == input);
		REQUIRE(rest == "");
	}

	SECTION("string with a couple formats")
	{
		const std::string input = "hello %i world %s haha";
		std::tie(first, rest) = strprintf::split_format(input);
		REQUIRE(first == "hello %i world ");
		REQUIRE(rest == "%s haha");

		std::tie(first, rest) = strprintf::split_format(rest);
		REQUIRE(first == "%s haha");
		REQUIRE(rest == "");
	}

	SECTION("string with %% (escaped percent sign)")
	{
		SECTION("before any formats")
		{
			const std::string input = "a 100%% rel%iable e%xamp%le";
			std::tie(first, rest) = strprintf::split_format(input);
			REQUIRE(first == "a 100%% rel%iable e");
			REQUIRE(rest == "%xamp%le");

			std::tie(first, rest) = strprintf::split_format(rest);
			REQUIRE(first == "%xamp");
			REQUIRE(rest == "%le");

			std::tie(first, rest) = strprintf::split_format(rest);
			REQUIRE(first == "%le");
			REQUIRE(rest == "");
		}

		SECTION("after all formats")
		{
			const std::string input = "%3u %% ";
			std::tie(first, rest) = strprintf::split_format(input);
			REQUIRE(first == "%3u ");
			REQUIRE(rest == "%% ");

			std::tie(first, rest) = strprintf::split_format(rest);
			REQUIRE(first == "%% ");
			REQUIRE(rest == "");
		}

		SECTION("consecutive escaped percent signs")
		{
			const std::string input = "%3u %% %% %i";
			std::tie(first, rest) = strprintf::split_format(input);
			REQUIRE(first == "%3u ");
			REQUIRE(rest == "%% %% %i");

			std::tie(first, rest) = strprintf::split_format(rest);
			REQUIRE(first == "%% %% %i");
			REQUIRE(rest == "");
		}
	}
}

TEST_CASE("strprintf::fmt() formats char* as a string", "[strprintf]")
{
	const char* input = "Hello, world!";
	REQUIRE(strprintf::fmt("%s", input) == std::string(input));
}

TEST_CASE("strprintf::fmt() formats int", "[strprintf]")
{
	REQUIRE(strprintf::fmt("%i", 42) == "42");

	const auto int_min = std::numeric_limits<int>::min();
	REQUIRE(int_min == -2147483648);
	REQUIRE(strprintf::fmt("%i", int_min) == "-2147483648");

	const auto int_max = std::numeric_limits<int>::max();
	REQUIRE(int_max == 2147483647);
	REQUIRE(strprintf::fmt("%i", int_max) == "2147483647");
}

TEST_CASE("strprintf::fmt() formats unsigned int", "[strprintf]")
{
	REQUIRE(strprintf::fmt("%u", 42u) == "42");

	REQUIRE(strprintf::fmt("%u", 0u) == "0");

	const auto uint_max = std::numeric_limits<unsigned int>::max();
	REQUIRE(uint_max == 4294967295u);
	REQUIRE(strprintf::fmt("%u", uint_max) == "4294967295");
}

TEST_CASE("strprintf::fmt() formats long int", "[strprintf]")
{
	REQUIRE(strprintf::fmt("%li", 42l) == "42");

	const auto int_min = std::numeric_limits<int>::min();
	const auto long_min = std::numeric_limits<long int>::min();
	REQUIRE(long_min < int_min);
	REQUIRE(int_min == -2147483648);
	REQUIRE(strprintf::fmt("%li", int_min - 1l) == "-2147483649");

	const auto int_max = std::numeric_limits<int>::max();
	const auto long_max = std::numeric_limits<long int>::max();
	REQUIRE(long_max > int_max);
	REQUIRE(int_max == 2147483647);
	REQUIRE(strprintf::fmt("%li", int_max + 1l) == "2147483648");
}

TEST_CASE("strprintf::fmt() formats long unsigned int", "[strprintf]")
{
	REQUIRE(strprintf::fmt("%lu", 42lu) == "42");

	REQUIRE(strprintf::fmt("%lu", 0lu) == "0");

	const auto ulong_max = std::numeric_limits<long unsigned int>::max();
	REQUIRE(ulong_max == 18446744073709551615lu);
	REQUIRE(strprintf::fmt("%lu", ulong_max) == "18446744073709551615");
}

TEST_CASE("strprintf::fmt() formats long long int", "[strprintf]")
{
	REQUIRE(strprintf::fmt("%lli", 42ll) == "42");

	// This is one bigger than actual std::numeric_limits<long long int>::min()
	// on x86_64. The actual value can't be written as a literal because both
	// GCC 8 and Clang complain about it being too small to be represented with
	// long long int.
	const auto input = -9223372036854775807ll;
	const auto llong_min = std::numeric_limits<long long int>::min();
	REQUIRE(llong_min <= input);
	REQUIRE(strprintf::fmt("%lli", input) == "-9223372036854775807");

	const auto llong_max = std::numeric_limits<long long int>::max();
	REQUIRE(llong_max == 9223372036854775807ll);
	REQUIRE(strprintf::fmt("%lli", llong_max) == "9223372036854775807");
}

TEST_CASE("strprintf::fmt() formats unsigned long long int", "[strprintf]")
{
	REQUIRE(strprintf::fmt("%llu", 42llu) == "42");

	REQUIRE(strprintf::fmt("%llu", 0llu) == "0");

	const auto ullong_max = std::numeric_limits<unsigned long long int>::max();
	REQUIRE(ullong_max == 18446744073709551615llu);
	REQUIRE(strprintf::fmt("%llu", ullong_max) == "18446744073709551615");
}

TEST_CASE("strprintf::fmt() formats void*", "[strprintf]")
{
	const auto x = 42;
	REQUIRE_FALSE(strprintf::fmt("%p", reinterpret_cast<const void*>(&x)).empty());
}

TEST_CASE("strprintf::fmt() formats nullptr", "[strprintf]")
{
	REQUIRE_FALSE(strprintf::fmt("%p", nullptr) == "(null)");
}

TEST_CASE("strprintf::fmt() formats double", "[strprintf]")
{
	const auto x = 42.0;
	REQUIRE(strprintf::fmt("%f", x) == "42.000000");
	REQUIRE(strprintf::fmt("%.3f", x) == "42.000");

	const auto y = 42e138;
	REQUIRE(strprintf::fmt("%e", y) == "4.200000e+139");
	REQUIRE(strprintf::fmt("%.3e", y) == "4.200e+139");
}

TEST_CASE("strprintf::fmt() formats float", "[strprintf]")
{
	const auto x = 42.0f;
	REQUIRE(strprintf::fmt("%f", x) == "42.000000");
	REQUIRE(strprintf::fmt("%.3f", x) == "42.000");

	const auto y = 42e3f;
	REQUIRE(strprintf::fmt("%e", y) == "4.200000e+04");
	REQUIRE(strprintf::fmt("%.3e", y) == "4.200e+04");
}

TEST_CASE("strprintf::fmt() formats std::string", "[strprintf]")
{
	const auto input = std::string("Hello, world!");
	REQUIRE(strprintf::fmt("%s", input) == input);
}

TEST_CASE("strprintf::fmt() formats std::string*", "[strprintf]")
{
	const auto input = std::string("Hello, world!");
	REQUIRE(strprintf::fmt("%s", &input) == input);
}

TEST_CASE("strprintf::fmt() works fine with 1MB format string", "[strprintf]")
{
	const auto format = std::string(1024 * 1024, ' ');
	REQUIRE(strprintf::fmt(format) == format);
}

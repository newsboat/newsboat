#include "strprintf.h"

#include <cinttypes>
#include <limits>

#include "3rd-party/catch.hpp"

using namespace Newsboat;

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

	SECTION("empty format string") {
		std::tie(first, rest) = strprintf::split_format("");
		REQUIRE(first == "");
		REQUIRE(rest == "");
	}

	SECTION("string without formats") {
		const std::string input = "hello world!";
		std::tie(first, rest) = strprintf::split_format(input);
		REQUIRE(first == input);
		REQUIRE(rest == "");
	}

	SECTION("string with a couple formats") {
		const std::string input = "hello %i world %s haha";
		std::tie(first, rest) = strprintf::split_format(input);
		REQUIRE(first == "hello %i world ");
		REQUIRE(rest == "%s haha");

		std::tie(first, rest) = strprintf::split_format(rest);
		REQUIRE(first == "%s haha");
		REQUIRE(rest == "");
	}

	SECTION("string with %% (escaped percent sign)") {
		SECTION("before any formats") {
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

		SECTION("after all formats") {
			const std::string input = "%3u %% ";
			std::tie(first, rest) = strprintf::split_format(input);
			REQUIRE(first == "%3u ");
			REQUIRE(rest == "%% ");

			std::tie(first, rest) = strprintf::split_format(rest);
			REQUIRE(first == "%% ");
			REQUIRE(rest == "");
		}

		SECTION("consecutive escaped percent signs") {
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
	REQUIRE(strprintf::fmt("%15s", input) == "  Hello, world!");
	REQUIRE(strprintf::fmt("%-15s", input) == "Hello, world!  ");
}

TEST_CASE("strprintf::fmt() formats int32_t", "[strprintf]")
{
	const int32_t input = 42;
	REQUIRE(strprintf::fmt("%" PRIi32, input) == "42");
	REQUIRE(strprintf::fmt("%" PRId32, input) == "42");

	const auto i32_min = std::numeric_limits<int32_t>::min();
	REQUIRE(strprintf::fmt("%" PRIi32, i32_min) == "-2147483648");
	REQUIRE(strprintf::fmt("%" PRId32, i32_min) == "-2147483648");

	const auto i32_max = std::numeric_limits<int32_t>::max();
	REQUIRE(strprintf::fmt("%" PRIi32, i32_max) == "2147483647");
	REQUIRE(strprintf::fmt("%" PRId32, i32_max) == "2147483647");
}

TEST_CASE("strprintf::fmt() formats uint32_t", "[strprintf]")
{
	const std::uint32_t input = 42;
	REQUIRE(strprintf::fmt("%" PRIu32, input) == "42");

	const std::uint32_t zero = 0;
	REQUIRE(strprintf::fmt("%" PRIu32, zero) == "0");

	const std::uint32_t u32_max = std::numeric_limits<std::uint32_t>::max();
	REQUIRE(strprintf::fmt("%" PRIu32, u32_max) == "4294967295");
}

TEST_CASE("strprintf::fmt() formats int64_t", "[strprintf]")
{
	const int64_t input = 42;
	REQUIRE(strprintf::fmt("%" PRIi64, input) == "42");
	REQUIRE(strprintf::fmt("%" PRId64, input) == "42");

	const auto i64_min = std::numeric_limits<int64_t>::min();
	REQUIRE(strprintf::fmt("%" PRIi64, i64_min) == "-9223372036854775808");
	REQUIRE(strprintf::fmt("%" PRId64, i64_min) == "-9223372036854775808");

	const auto i64_max = std::numeric_limits<int64_t>::max();
	REQUIRE(strprintf::fmt("%" PRIi64, i64_max) == "9223372036854775807");
	REQUIRE(strprintf::fmt("%" PRId64, i64_max) == "9223372036854775807");
}

TEST_CASE("strprintf::fmt() formats uint64_t", "[strprintf]")
{
	const uint64_t input = 42;
	REQUIRE(strprintf::fmt("%" PRIu64, input) == "42");

	const uint64_t zero = 0;
	REQUIRE(strprintf::fmt("%" PRIu64, zero) == "0");

	const uint64_t u64_max = std::numeric_limits<uint64_t>::max();
	REQUIRE(strprintf::fmt("%" PRIu64, u64_max) == "18446744073709551615");
}

TEST_CASE("strprintf::fmt() formats pointers", "[strprintf]")
{
	const auto x = 42;

	const auto x_ptr = &x;
	const auto x_ptr_formatted = strprintf::fmt("%p", x_ptr);

	const auto x_ptr_void = reinterpret_cast<const void*>(x_ptr);
	const auto x_ptr_void_formatted = strprintf::fmt("%p", x_ptr_void);

	REQUIRE_FALSE(x_ptr_formatted == "");
	REQUIRE(x_ptr_formatted == x_ptr_void_formatted);
}

TEST_CASE("strprintf::fmt() formats null pointers the same", "[strprintf]")
{
	const auto int_ptr_fmt =
		strprintf::fmt("%p", static_cast<const int*>(nullptr));
	const auto uint_ptr_fmt =
		strprintf::fmt("%p", static_cast<const unsigned int*>(nullptr));
	const auto long_ptr_fmt =
		strprintf::fmt("%p", static_cast<const long*>(nullptr));
	const auto ulong_ptr_fmt =
		strprintf::fmt("%p", static_cast<const unsigned long*>(nullptr));
	const auto llong_ptr_fmt =
		strprintf::fmt("%p", static_cast<const long long*>(nullptr));
	const auto ullong_ptr_fmt =
		strprintf::fmt("%p", static_cast<const unsigned long long*>(nullptr));
	REQUIRE_FALSE(int_ptr_fmt == "");
	REQUIRE(int_ptr_fmt == uint_ptr_fmt);
	REQUIRE(uint_ptr_fmt == long_ptr_fmt);
	REQUIRE(long_ptr_fmt == ulong_ptr_fmt);
	REQUIRE(ulong_ptr_fmt == llong_ptr_fmt);
	REQUIRE(llong_ptr_fmt == ullong_ptr_fmt);
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
	REQUIRE(strprintf::fmt("%15s", input) == "  Hello, world!");
	REQUIRE(strprintf::fmt("%-15s", input) == "Hello, world!  ");
}

TEST_CASE("strprintf::fmt() formats std::string*", "[strprintf]")
{
	const auto input = std::string("Hello, world!");
	REQUIRE(strprintf::fmt("%s", &input) == input);
	REQUIRE(strprintf::fmt("%15s", input) == "  Hello, world!");
	REQUIRE(strprintf::fmt("%-15s", input) == "Hello, world!  ");
}

TEST_CASE("strprintf::fmt() works fine with 2MB format string", "[strprintf]")
{
	const auto spacer = std::string(1024 * 1024, ' ');
	const auto format = spacer + "%i" + spacer + "%i";
	const auto expected = spacer + "42" + spacer + "100500";
	REQUIRE(strprintf::fmt(format, 42, 100500) == expected);
}

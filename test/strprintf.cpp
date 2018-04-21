#include "strprintf.h"

#include "3rd-party/catch.hpp"

using namespace newsboat;

TEST_CASE("strprintf::fmt()", "[strprintf]") {
	REQUIRE(strprintf::fmt("") == "");
	REQUIRE(strprintf::fmt("%s", "") == "");
	REQUIRE(strprintf::fmt("%u", 0) == "0");
	REQUIRE(strprintf::fmt("%s", nullptr) == "(null)");
	REQUIRE(strprintf::fmt("%u-%s-%c", 23, "hello world", 'X') == "23-hello world-X");
	REQUIRE(strprintf::fmt("%%") == "%");

	std::string long_input_without_formats(240000, 'x');
	REQUIRE(strprintf::fmt(long_input_without_formats)
			== long_input_without_formats);
}

TEST_CASE("strprintf::split_format()", "[strprintf]") {
	std::string first, rest;

	SECTION("empty format string") {
		std::tie(first, rest) = strprintf::split_format("");
		REQUIRE(first == "");
		REQUIRE(rest  == "");
	}

	SECTION("string without formats") {
		const std::string input = "hello world!";
		std::tie(first, rest) = strprintf::split_format(input);
		REQUIRE(first == input);
		REQUIRE(rest  == "");
	}

	SECTION("string with a couple formats") {
		const std::string input = "hello %i world %s haha";
		std::tie(first, rest) = strprintf::split_format(input);
		REQUIRE(first == "hello %i world ");
		REQUIRE(rest  == "%s haha");

		std::tie(first, rest) = strprintf::split_format(rest);
		REQUIRE(first == "%s haha");
		REQUIRE(rest  == "");
	}

	SECTION("string with %% (escaped percent sign)") {
		SECTION("before any formats") {
			const std::string input = "a 100%% rel%iable e%xamp%le";
			std::tie(first, rest) = strprintf::split_format(input);
			REQUIRE(first == "a 100%% rel%iable e");
			REQUIRE(rest  == "%xamp%le");

			std::tie(first, rest) = strprintf::split_format(rest);
			REQUIRE(first == "%xamp");
			REQUIRE(rest  == "%le");

			std::tie(first, rest) = strprintf::split_format(rest);
			REQUIRE(first == "%le");
			REQUIRE(rest  == "");
		}

		SECTION("after all formats") {
			const std::string input = "%3u %% ";
			std::tie(first, rest) = strprintf::split_format(input);
			REQUIRE(first == "%3u ");
			REQUIRE(rest  == "%% ");

			std::tie(first, rest) = strprintf::split_format(rest);
			REQUIRE(first == "%% ");
			REQUIRE(rest  == "");
		}

		SECTION("consecutive escaped percent signs") {
			const std::string input = "%3u %% %% %i";
			std::tie(first, rest) = strprintf::split_format(input);
			REQUIRE(first == "%3u ");
			REQUIRE(rest  == "%% %% %i");

			std::tie(first, rest) = strprintf::split_format(rest);
			REQUIRE(first == "%% %% %i");
			REQUIRE(rest  == "");
		}
	}
}

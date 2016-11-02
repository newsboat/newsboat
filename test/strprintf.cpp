#include "catch.hpp"

#include <strprintf.h>

using namespace newsbeuter;

TEST_CASE("strprintf::fmt()") {
	REQUIRE(strprintf::fmt("") == "");
	REQUIRE(strprintf::fmt("%s", "") == "");
	REQUIRE(strprintf::fmt("%u", 0) == "0");
	REQUIRE(strprintf::fmt("%s", nullptr) == "(null)");
	REQUIRE(strprintf::fmt("%u-%s-%c", 23, "hello world", 'X') == "23-hello world-X");
}

TEST_CASE("strprintf::split_format()") {
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
}

#include "regexowner.h"

#include <regex.h>

#include "3rd-party/catch.hpp"

using namespace Newsboat;

TEST_CASE("Regex matches basic posix regular expression", "[Regex]")
{
	std::string errorMessage;
	auto regex = Regex::compile("abc+", 0, errorMessage);
	REQUIRE(errorMessage == "");
	REQUIRE(regex);

	const int max_matches = 1;
	const auto matches = regex->matches("abc+others", max_matches, 0);
	REQUIRE(matches.size() == 1);
	REQUIRE(matches[0].first == 0);
	REQUIRE(matches[0].second == 4);
}

TEST_CASE("Regex matches extended posix regular expression", "[Regex]")
{
	std::string errorMessage;
	auto regex = Regex::compile("aBc+", REG_EXTENDED | REG_ICASE, errorMessage);
	REQUIRE(errorMessage == "");
	REQUIRE(regex);

	const int max_matches = 1;
	const auto matches = regex->matches("AbCcCcCC and others", max_matches, 0);
	REQUIRE(matches.size() == 1);
	REQUIRE(matches[0].first == 0);
	REQUIRE(matches[0].second == 8);
}

TEST_CASE("Regex returns empty vector when regex is valid but there is no match",
	"[Regex]")
{
	std::string errorMessage;
	auto regex = Regex::compile("abc", 0, errorMessage);
	REQUIRE(errorMessage == "");
	REQUIRE(regex);

	const int max_matches = 1;
	const auto matches = regex->matches("not a match", max_matches, 0);
	REQUIRE(matches.size() == 0);
}

TEST_CASE("Regex returns at most max_results matches", "[Regex]")
{
	std::string errorMessage;
	auto regex = Regex::compile("(a)(b)(c)", REG_EXTENDED, errorMessage);
	REQUIRE(errorMessage == "");
	REQUIRE(regex);

	const int max_matches = 2;
	const auto matches = regex->matches("abc", max_matches, 0);
	REQUIRE(matches.size() == 2);
	REQUIRE(matches[0].first == 0);
	REQUIRE(matches[0].second == 3);
	REQUIRE(matches[1].first == 0);
	REQUIRE(matches[1].second == 1);
}

TEST_CASE("Regex returns no more results than available", "[Regex]")
{
	std::string errorMessage;
	auto regex = Regex::compile("abc", REG_EXTENDED, errorMessage);
	REQUIRE(errorMessage == "");
	REQUIRE(regex);

	const int max_matches = 10;
	const auto matches = regex->matches("abc", max_matches, 0);
	REQUIRE(matches.size() == 1);
	REQUIRE(matches[0].first == 0);
	REQUIRE(matches[0].second == 3);
}

TEST_CASE("Regex returns error on invalid regex", "[Regex]")
{
	std::string errorMessage;
	auto regex = Regex::compile("(abc", REG_EXTENDED, errorMessage);

	REQUIRE_FALSE(errorMessage.empty());

	// The message shouldn't contain a C string terminator (NUL) at the end
	REQUIRE(errorMessage.back() != 0);

	REQUIRE_FALSE(regex);
}

#include "rss/rssparser.h"

#include "3rd-party/catch.hpp"
#include "test-helpers.h"

TEST_CASE("W3CDTF parser extracts date and time from any valid string",
	"[rsspp::RssParser]")
{
	SECTION("year only")
	{
		REQUIRE(rsspp::RssParser::__w3cdtf_to_rfc822("2008") ==
			"Tue, 01 Jan 2008 00:00:00 +0000");
	}

	SECTION("year-month only")
	{
		REQUIRE(rsspp::RssParser::__w3cdtf_to_rfc822("2008-12") ==
			"Mon, 01 Dec 2008 00:00:00 +0000");
	}

	SECTION("year-month-day only")
	{
		REQUIRE(rsspp::RssParser::__w3cdtf_to_rfc822("2008-12-30") ==
			"Tue, 30 Dec 2008 00:00:00 +0000");
	}

	SECTION("date and time with Z timezone")
	{
		REQUIRE(rsspp::RssParser::__w3cdtf_to_rfc822(
				"2008-12-30T13:03:15Z") ==
			"Tue, 30 Dec 2008 13:03:15 +0000");
	}

	SECTION("date and time with -08:00 timezone")
	{
		REQUIRE(rsspp::RssParser::__w3cdtf_to_rfc822(
				"2008-12-30T10:03:15-08:00") ==
			"Tue, 30 Dec 2008 18:03:15 +0000");
	}
}

TEST_CASE("W3CDTF parser returns empty string on invalid input",
	"[rsspp::RssParser]")
{
	REQUIRE(rsspp::RssParser::__w3cdtf_to_rfc822("foobar") == "");
	REQUIRE(rsspp::RssParser::__w3cdtf_to_rfc822("-3") == "");
	REQUIRE(rsspp::RssParser::__w3cdtf_to_rfc822("") == "");
}

TEST_CASE(
	"W3C DTF to RFC 822 conversion does not take into account the local "
	"timezone (#369)",
	"[rsspp::RssParser]")
{
	auto input = "2008-12-30T10:03:15-08:00";
	auto expected = "Tue, 30 Dec 2008 18:03:15 +0000";

	TestHelpers::EnvVar tzEnv("TZ");
	tzEnv.on_change([]() { ::tzset(); });

	// US/Pacific and Australia/Sydney have pretty much opposite DST
	// schedules, so for any given moment in time one of the following two
	// sections will be observing DST while other won't
	SECTION("Timezone Pacific")
	{
		tzEnv.set("US/Pacific");
		REQUIRE(rsspp::RssParser::__w3cdtf_to_rfc822(input) ==
			expected);
	}

	SECTION("Timezone Australia")
	{
		tzEnv.set("Australia/Sydney");
		REQUIRE(rsspp::RssParser::__w3cdtf_to_rfc822(input) ==
			expected);
	}

	// During October, both US/Pacific and Australia/Sydney are observing
	// DST. Arizona and UTC *never* observe it, though, so the following two
	// tests will cover October
	SECTION("Timezone Arizona")
	{
		tzEnv.set("US/Arizona");
		REQUIRE(rsspp::RssParser::__w3cdtf_to_rfc822(input) ==
			expected);
	}

	SECTION("Timezone UTC")
	{
		tzEnv.set("UTC");
		REQUIRE(rsspp::RssParser::__w3cdtf_to_rfc822(input) ==
			expected);
	}
}

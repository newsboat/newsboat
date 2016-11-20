#include "catch.hpp"

#include <rsspp.h>
#include <rsspp_internal.h>
#include <rss.h>
#include <cache.h>
#include <configcontainer.h>

TEST_CASE("Throws exception if file doesn't exist", "[rsspp::parser]") {
	rsspp::parser p;

	try {
		rsspp::feed f = p.parse_file("data/non-existent.xml");
	} catch (rsspp::exception e) {
		REQUIRE(e.what() == std::string("could not parse file"));
	}
}

TEST_CASE("Throws exception if file can't be parsed", "[rsspp::parser]") {
	rsspp::parser p;

	try {
		rsspp::feed f = p.parse_file("data/empty.xml");
	} catch (rsspp::exception e) {
		REQUIRE(e.what() == std::string("could not parse file"));
	}
}

TEST_CASE("Parsers behave correctly", "[rsspp::parser]") {
	rsspp::parser p;
	rsspp::feed f;

	SECTION("RSS 0.91 is parsed correctly") {
		REQUIRE_NOTHROW(f = p.parse_file("data/rss091_1.xml"));

		REQUIRE(f.rss_version == rsspp::RSS_0_91);
		REQUIRE(f.title == "Example Channel");
		REQUIRE(f.description == "an example feed");
		REQUIRE(f.link == "http://example.com/");
		REQUIRE(f.language == "en");

		REQUIRE(f.items.size() == 1u);
		REQUIRE(f.items[0].title == "1 < 2");
		REQUIRE(f.items[0].link == "http://example.com/1_less_than_2.html");
		REQUIRE(f.items[0].description == "1 < 2, 3 < 4.\nIn HTML, <b> starts a bold phrase\nand you start a link with <a href=\n");
		REQUIRE(f.items[0].author == "");
		REQUIRE(f.items[0].guid == "");
	}

	SECTION("RSS 0.92 is parsed correctly") {
		REQUIRE_NOTHROW(f = p.parse_file("data/rss092_1.xml"));

		REQUIRE(f.rss_version == rsspp::RSS_0_92);
		REQUIRE(f.title == "Example Channel");
		REQUIRE(f.description == "an example feed");
		REQUIRE(f.link == "http://example.com/");
		REQUIRE(f.language == "en");

		REQUIRE(f.items.size() == 3u);

		REQUIRE(f.items[0].title == "1 < 2");
		REQUIRE(f.items[0].link == "http://example.com/1_less_than_2.html");
		REQUIRE(f.items[0].base == "http://example.com/feed/rss_testing.html");

		REQUIRE(f.items[1].title == "A second item");
		REQUIRE(f.items[1].link == "http://example.com/a_second_item.html");
		REQUIRE(f.items[1].description == "no description");
		REQUIRE(f.items[1].author == "");
		REQUIRE(f.items[1].guid == "");
		REQUIRE(f.items[1].base == "http://example.com/item/rss_testing.html");

		REQUIRE(f.items[2].title == "A third item");
		REQUIRE(f.items[2].link == "http://example.com/a_third_item.html");
		REQUIRE(f.items[2].description == "no description");
		REQUIRE(f.items[2].base == "http://example.com/desc/rss_testing.html");
	}


	SECTION("RSS 2.0 is parsed correctly") {
		REQUIRE_NOTHROW(f = p.parse_file("data/rss20_1.xml"));

		REQUIRE(f.title == "my weblog");
		REQUIRE(f.link == "http://example.com/blog/");
		REQUIRE(f.description == "my description");

		REQUIRE(f.items.size() == 1u);

		REQUIRE(f.items[0].title == "this is an item");
		REQUIRE(f.items[0].link == "http://example.com/blog/this_is_an_item.html");
		REQUIRE(f.items[0].author == "Andreas Krennmair");
		REQUIRE(f.items[0].author_email == "blog@synflood.at");
		REQUIRE(f.items[0].content_encoded == "oh well, this is the content.");
		REQUIRE(f.items[0].pubDate == "Fri, 12 Dec 2008 02:36:10 +0100");
		REQUIRE(f.items[0].guid == "http://example.com/blog/this_is_an_item.html");
		REQUIRE_FALSE(f.items[0].guid_isPermaLink);
	}

	SECTION("RSS 1.0 is parsed correctly") {
		REQUIRE_NOTHROW(f = p.parse_file("data/rss10_1.xml"));

		REQUIRE(f.rss_version == rsspp::RSS_1_0);

		REQUIRE(f.title == "Example Dot Org");
		REQUIRE(f.link == "http://www.example.org");
		REQUIRE(f.description == "the Example Organization web site");

		REQUIRE(f.items.size() == 1u);

		REQUIRE(f.items[0].title == "New Status Updates");
		REQUIRE(f.items[0].link == "http://www.example.org/status/foo");
		REQUIRE(f.items[0].guid == "http://www.example.org/status/");
		REQUIRE(f.items[0].description == "News about the Example project");
		REQUIRE(f.items[0].pubDate == "Tue, 30 Dec 2008 07:20:00 +0000");
	}


	SECTION("Atom 1.0 is parsed correctly") {
		REQUIRE_NOTHROW(f = p.parse_file("data/atom10_1.xml"));

		REQUIRE(f.rss_version == rsspp::ATOM_1_0);

		REQUIRE(f.title == "test atom");
		REQUIRE(f.title_type == "text");
		REQUIRE(f.description == "atom description!");
		REQUIRE(f.pubDate == "Tue, 30 Dec 2008 18:26:15 +0000");
		REQUIRE(f.link == "http://example.com/");

		REQUIRE(f.items.size() == 3u);
		REQUIRE(f.items[0].title == "A gentle introduction to Atom testing");
		REQUIRE(f.items[0].title_type == "html");
		REQUIRE(f.items[0].link == "http://example.com/atom_testing.html");
		REQUIRE(f.items[0].guid == "tag:example.com,2008-12-30:/atom_testing");
		REQUIRE(f.items[0].description == "some content");
		REQUIRE(f.items[0].base == "http://example.com/feed/atom_testing.html");

		REQUIRE(f.items[1].title == "A missing rel attribute");
		REQUIRE(f.items[1].title_type == "html");
		REQUIRE(f.items[1].link == "http://example.com/atom_testing.html");
		REQUIRE(f.items[1].guid == "tag:example.com,2008-12-30:/atom_testing1");
		REQUIRE(f.items[1].description == "some content");
		REQUIRE(f.items[1].base == "http://example.com/entry/atom_testing.html");

		REQUIRE(f.items[2].title == "alternate link isn't first");
		REQUIRE(f.items[2].title_type == "html");
		REQUIRE(f.items[2].link == "http://example.com/atom_testing.html");
		REQUIRE(f.items[2].guid == "tag:example.com,2008-12-30:/atom_testing2");
		REQUIRE(f.items[2].description == "some content");
		REQUIRE(f.items[2].base == "http://example.com/content/atom_testing.html");
	}
}

TEST_CASE("W3CDTF parser behaves correctly", "[rsspp::rss_parser]") {
	SECTION("W3CDTF year only") {
		REQUIRE(rsspp::rss_parser::__w3cdtf_to_rfc822("2008") == "Tue, 01 Jan 2008 00:00:00 +0000");
	}

	SECTION("W3CDTF year-month only") {
		REQUIRE(rsspp::rss_parser::__w3cdtf_to_rfc822("2008-12") == "Mon, 01 Dec 2008 00:00:00 +0000");
	}

	SECTION("W3CDTF year-month-day only") {
		REQUIRE(rsspp::rss_parser::__w3cdtf_to_rfc822("2008-12-30") == "Tue, 30 Dec 2008 00:00:00 +0000");
	}

	SECTION("W3CDTF with Z timezone") {
		REQUIRE(rsspp::rss_parser::__w3cdtf_to_rfc822("2008-12-30T13:03:15Z") == "Tue, 30 Dec 2008 13:03:15 +0000");
	}

	SECTION("W3CDTF with -08:00 timezone") {
		REQUIRE(rsspp::rss_parser::__w3cdtf_to_rfc822("2008-12-30T10:03:15-08:00") == "Tue, 30 Dec 2008 18:03:15 +0000");
	}

	SECTION("Invalid W3CDTF (foobar)") {
		REQUIRE(rsspp::rss_parser::__w3cdtf_to_rfc822("foobar") == "");
	}

	SECTION("Invalid W3CDTF (negative number)") {
		REQUIRE(rsspp::rss_parser::__w3cdtf_to_rfc822("-3") == "");
	}

	SECTION("Invalid W3CDTF (empty string)") {
		REQUIRE(rsspp::rss_parser::__w3cdtf_to_rfc822("") == "");
	}
}

TEST_CASE("W3C DTF to RFC 822 conversion behaves correctly with different "
          "local timezones", "[rsspp::rss_parser]") {
	// There has been a problem in the C date conversion functions when the TZ is
	// set to different locations, and localtime is in daylight savings. One of
	// these two next tests sections should be in active daylight savings.
	// https://github.com/akrennmair/newsbeuter/issues/369
	
	char *tz = getenv("TZ");
	std::string tz_saved_value;
	if (tz) {
		// tz can be null, and buffer may be reused.  Save it if it exists.
		tz_saved_value = tz;
	}

	SECTION("Timezone Pacific") {
		setenv("TZ", "US/Pacific", 1);
		tzset();
		REQUIRE(rsspp::rss_parser::__w3cdtf_to_rfc822("2008-12-30T10:03:15-08:00") == "Tue, 30 Dec 2008 18:03:15 +0000");
	}

	SECTION("Timezone Australia") {
		setenv("TZ", "Australia/Sydney", 1);
		tzset();
		REQUIRE(rsspp::rss_parser::__w3cdtf_to_rfc822("2008-12-30T10:03:15-08:00") == "Tue, 30 Dec 2008 18:03:15 +0000");
	}

	SECTION("Timezone Arizona") {
		setenv("TZ", "US/Arizona", 1);
		tzset();
		REQUIRE(rsspp::rss_parser::__w3cdtf_to_rfc822("2008-12-30T10:03:15-08:00") == "Tue, 30 Dec 2008 18:03:15 +0000");
	}

	SECTION("Timezone UTC") {
		setenv("TZ", "UTC", 1);
		tzset();
		REQUIRE(rsspp::rss_parser::__w3cdtf_to_rfc822("2008-12-30T10:03:15-08:00") == "Tue, 30 Dec 2008 18:03:15 +0000");
	}

	// Reset back to original value
	if (tz) {
		setenv("TZ", tz_saved_value.c_str(), 1);
	}
	else {
		unsetenv("TZ");
	}
	tzset();
}

namespace newsbeuter {

TEST_CASE("set_rssurl checks if query feed has a valid query", "[rss]") {
	configcontainer cfg;
	cache rsscache(":memory:", &cfg);
	rss_feed f(&rsscache);

	SECTION("invalid query results in exception") {
		REQUIRE_THROWS(f.set_rssurl("query:a title:unread ="));
		REQUIRE_THROWS(f.set_rssurl("query:a title:between 1:3"));
	}

	SECTION("valid query doesn't throw an exception") {
		REQUIRE_NOTHROW(f.set_rssurl("query:a title:unread = \"yes\""));
		REQUIRE_NOTHROW(f.set_rssurl("query:Title:unread = \"yes\" and age between 0:7"));
	}
}

}

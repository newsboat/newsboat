#include "rss.h"
#include "rsspp.h"

#include "3rd-party/catch.hpp"
#include "rsspp_internal.h"
#include "rss_parser.h"
#include "cache.h"
#include "configcontainer.h"

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

TEST_CASE("Extracts data from RSS 0.91", "[rsspp::parser]") {
	rsspp::parser p;
	rsspp::feed f;

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

TEST_CASE("Extracts data from RSS 0.92", "[rsspp::parser]") {
	rsspp::parser p;
	rsspp::feed f;

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

TEST_CASE("Extracts data fro RSS 2.0", "[rsspp::parser]") {
	rsspp::parser p;
	rsspp::feed f;

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

TEST_CASE("Extracts data from RSS 1.0", "[rsspp::parser]") {
	rsspp::parser p;
	rsspp::feed f;

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

TEST_CASE("Extracts data from Atom 1.0", "[rsspp::parser]") {
	rsspp::parser p;
	rsspp::feed f;

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

TEST_CASE("W3CDTF parser extracts date and time from any valid string",
          "[rsspp::rss_parser]")
{
	SECTION("year only") {
		REQUIRE(rsspp::rss_parser::__w3cdtf_to_rfc822("2008") == "Tue, 01 Jan 2008 00:00:00 +0000");
	}

	SECTION("year-month only") {
		REQUIRE(rsspp::rss_parser::__w3cdtf_to_rfc822("2008-12") == "Mon, 01 Dec 2008 00:00:00 +0000");
	}

	SECTION("year-month-day only") {
		REQUIRE(rsspp::rss_parser::__w3cdtf_to_rfc822("2008-12-30") == "Tue, 30 Dec 2008 00:00:00 +0000");
	}

	SECTION("date and time with Z timezone") {
		REQUIRE(rsspp::rss_parser::__w3cdtf_to_rfc822("2008-12-30T13:03:15Z") == "Tue, 30 Dec 2008 13:03:15 +0000");
	}

	SECTION("date and time with -08:00 timezone") {
		REQUIRE(rsspp::rss_parser::__w3cdtf_to_rfc822("2008-12-30T10:03:15-08:00") == "Tue, 30 Dec 2008 18:03:15 +0000");
	}
}

TEST_CASE("W3CDTF parser returns empty string on invalid input",
          "[rsspp::rss_parser]")
{
	REQUIRE(rsspp::rss_parser::__w3cdtf_to_rfc822("foobar") == "");
	REQUIRE(rsspp::rss_parser::__w3cdtf_to_rfc822("-3") == "");
	REQUIRE(rsspp::rss_parser::__w3cdtf_to_rfc822("") == "");
}

TEST_CASE("W3C DTF to RFC 822 conversion does not take into account the local "
          "timezone (#369)", "[rsspp::rss_parser]")
{
	auto input = "2008-12-30T10:03:15-08:00";
	auto expected = "Tue, 30 Dec 2008 18:03:15 +0000";

	char *tz = getenv("TZ");
	std::string tz_saved_value;
	if (tz) {
		// tz can be null, and buffer may be reused.  Save it if it exists.
		tz_saved_value = tz;
	}

	// US/Pacific and Australia/Sydney have pretty much opposite DST schedules,
	// so for any given moment in time one of the following two sections will
	// be observing DST while other won't
	SECTION("Timezone Pacific") {
		setenv("TZ", "US/Pacific", 1);
		tzset();
		REQUIRE(rsspp::rss_parser::__w3cdtf_to_rfc822(input) == expected);
	}

	SECTION("Timezone Australia") {
		setenv("TZ", "Australia/Sydney", 1);
		tzset();
		REQUIRE(rsspp::rss_parser::__w3cdtf_to_rfc822(input) == expected);
	}

	// During October, both US/Pacific and Australia/Sydney are observing DST.
	// Arizona and UTC *never* observe it, though, so the following two tests
	// will cover October
	SECTION("Timezone Arizona") {
		setenv("TZ", "US/Arizona", 1);
		tzset();
		REQUIRE(rsspp::rss_parser::__w3cdtf_to_rfc822(input) == expected);
	}

	SECTION("Timezone UTC") {
		setenv("TZ", "UTC", 1);
		tzset();
		REQUIRE(rsspp::rss_parser::__w3cdtf_to_rfc822(input) == expected);
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

namespace newsboat {

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

TEST_CASE("rss_item::sort_flags() cleans up flags", "[rss]") {
	configcontainer cfg;
	cache rsscache(":memory:", &cfg);
	rss_item item(&rsscache);

	SECTION("Repeated letters do not erase other letters"){
		std::string inputflags = "Abcdecf";
		std::string result = "Abcdef";
		item.set_flags(inputflags);
		REQUIRE(result == item.flags());
	}

	SECTION("Non alpha characters in input flags are ignored"){
		std::string inputflags = "Abcd";
		item.set_flags(inputflags + "1234568790^\"#'é(£");
		REQUIRE(inputflags == item.flags());
	}
}

TEST_CASE("If item's <title> is empty, try to deduce it from the URL",
          "[rss::rss_parser]")
{
	configcontainer cfg;
	cache rsscache(":memory:", &cfg);
	rss_parser p(
			"file://data/items_without_titles.xml",
			&rsscache,
			&cfg,
			nullptr,
			nullptr);
	auto feed = p.parse();

	REQUIRE(feed->items()[0]->title() == "A gentle introduction to testing");
	REQUIRE(feed->items()[1]->title() == "A missing rel attribute");
	REQUIRE(feed->items()[2]->title() == "Alternate link isnt first");
	REQUIRE(feed->items()[3]->title() == "A test for htm extension");
	REQUIRE(feed->items()[4]->title() == "Alternate link isn't first");
}

}

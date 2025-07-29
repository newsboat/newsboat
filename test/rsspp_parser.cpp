#include "rss/parser.h"

#include <cstdint>

#include "3rd-party/catch.hpp"
#include "curlhandle.h"
#include "rss/exception.h"
#include "strprintf.h"
#include "test_helpers/exceptionwithmsg.h"
#include "test_helpers/httptestserver.h"
#include "test_helpers/misc.h"
#include "utils.h"

using newsboat::operator""_path;

TEST_CASE("Throws exception if file doesn't exist", "[rsspp::Parser]")
{
	using test_helpers::ExceptionWithMsg;

	rsspp::Parser p;

	REQUIRE_THROWS_MATCHES(p.parse_file("data/non-existent.xml"_path),
		rsspp::Exception,
		ExceptionWithMsg<rsspp::Exception>("could not parse file"));
}

TEST_CASE("Throws exception if file can't be parsed", "[rsspp::Parser]")
{
	using test_helpers::ExceptionWithMsg;

	rsspp::Parser p;

	REQUIRE_THROWS_MATCHES(p.parse_file("data/empty.xml"_path),
		rsspp::Exception,
		ExceptionWithMsg<rsspp::Exception>("could not parse file"));
}

TEST_CASE("Extracts data from RSS 0.91", "[rsspp::Parser]")
{
	rsspp::Parser p;
	rsspp::Feed f;

	REQUIRE_NOTHROW(f = p.parse_file("data/rss091_1.xml"_path));

	REQUIRE(f.rss_version == rsspp::Feed::RSS_0_91);
	REQUIRE(f.title == "Example Channel");
	REQUIRE(f.description == "an example feed");
	REQUIRE(f.link == "http://example.com/");
	REQUIRE(f.language == "en");

	REQUIRE(f.items.size() == 1u);
	REQUIRE(f.items[0].title == "1 < 2");
	REQUIRE(f.items[0].link == "http://example.com/1_less_than_2.html");
	REQUIRE(f.items[0].description ==
		"1 < 2, 3 < 4.\nIn HTML, <b> starts a bold phrase\nand you "
		"start a link with <a href=\n");
	REQUIRE(f.items[0].author == "");
	REQUIRE(f.items[0].guid == "");
}

TEST_CASE("Doesn't crash or garble data if an item in RSS 0.9x contains "
	"an empty author tag",
	"[rsspp::Parser][issue542]")
{
	rsspp::Parser p;
	rsspp::Feed f;

	const auto check = [&]() {
		REQUIRE(f.title == "A Channel with Unnamed Authors");
		REQUIRE(f.description == "an example feed");
		REQUIRE(f.link == "http://example.com/");
		REQUIRE(f.language == "en");

		REQUIRE(f.items.size() == 2u);

		REQUIRE(f.items[0].title == "This one has en empty author tag");
		REQUIRE(f.items[0].link == "http://example.com/test_1.html");
		REQUIRE(f.items[0].description == "It doesn't matter.");
		REQUIRE(f.items[0].author == "");
		REQUIRE(f.items[0].guid == "");

		REQUIRE(f.items[1].title == "This one has en empty author tag as well");
		REQUIRE(f.items[1].link == "http://example.com/test_2.html");
		REQUIRE(f.items[1].description == "Non-empty description though.");
		REQUIRE(f.items[1].author == "");
		REQUIRE(f.items[1].guid == "");
	};

	SECTION("RSS 0.91") {
		REQUIRE_NOTHROW(f = p.parse_file("data/rss_091_with_empty_author.xml"_path));
		REQUIRE(f.rss_version == rsspp::Feed::RSS_0_91);
		check();
	}

	SECTION("RSS 0.92") {
		REQUIRE_NOTHROW(f = p.parse_file("data/rss_092_with_empty_author.xml"_path));
		REQUIRE(f.rss_version == rsspp::Feed::RSS_0_92);
		check();
	}

	SECTION("RSS 0.94") {
		REQUIRE_NOTHROW(f = p.parse_file("data/rss_094_with_empty_author.xml"_path));
		REQUIRE(f.rss_version == rsspp::Feed::RSS_0_94);
		check();
	}
}

TEST_CASE("Doesn't crash or garble data if an item in RSS 0.9x contains "
	"an author tag which ends with a bracket",
	"[rsspp::Parser][issue2834]")
{
	rsspp::Parser p;
	rsspp::Feed f;

	const auto check = [&]() {
		REQUIRE(f.title == "A Channel with Authors With Names Containing Brackets");
		REQUIRE(f.description == "an example feed");
		REQUIRE(f.link == "http://example.com/");
		REQUIRE(f.language == "en");

		REQUIRE(f.items.size() == 3u);

		REQUIRE(f.items[0].title == "This one has an author name ending with a closing bracket");
		REQUIRE(f.items[0].link == "http://example.com/test_1.html");
		REQUIRE(f.items[0].description == "Non-empty description.");
		REQUIRE(f.items[0].author == "Author name)");
		REQUIRE(f.items[0].guid == "");

		REQUIRE(f.items[1].title == "This one has an author name with an email in brackets");
		REQUIRE(f.items[1].link == "http://example.com/test_2.html");
		REQUIRE(f.items[1].description == "This is empty description (no).");
		REQUIRE(f.items[1].author == "Author");
		REQUIRE(f.items[1].guid == "");

		REQUIRE(f.items[2].title ==
			"This one has an author name with a non-email next in brackets");
		REQUIRE(f.items[2].link == "http://example.com/test_3.html");
		REQUIRE(f.items[2].description == "This is empty description (yes (no)).");
		REQUIRE(f.items[2].author == "Author (name)");
		REQUIRE(f.items[2].guid == "");
	};

	SECTION("RSS 0.91") {
		REQUIRE_NOTHROW(f = p.parse_file("data/rss_091_with_bracket_author.xml"_path));
		REQUIRE(f.rss_version == rsspp::Feed::RSS_0_91);
		check();
	}

	SECTION("RSS 0.92") {
		REQUIRE_NOTHROW(f = p.parse_file("data/rss_092_with_bracket_author.xml"_path));
		REQUIRE(f.rss_version == rsspp::Feed::RSS_0_92);
		check();
	}

	SECTION("RSS 0.94") {
		REQUIRE_NOTHROW(f = p.parse_file("data/rss_094_with_bracket_author.xml"_path));
		REQUIRE(f.rss_version == rsspp::Feed::RSS_0_94);
		check();
	}
}

TEST_CASE("Extracts data from RSS 0.92", "[rsspp::Parser]")
{
	rsspp::Parser p;
	rsspp::Feed f;

	REQUIRE_NOTHROW(f = p.parse_file("data/rss092_1.xml"_path));

	REQUIRE(f.rss_version == rsspp::Feed::RSS_0_92);
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

TEST_CASE("Extracts data fro RSS 2.0", "[rsspp::Parser]")
{
	rsspp::Parser p;
	rsspp::Feed f;

	REQUIRE_NOTHROW(f = p.parse_file("data/rss20_1.xml"_path));

	REQUIRE(f.title == "my weblog");
	REQUIRE(f.link == "http://example.com/blog/");
	REQUIRE(f.description == "my description");

	REQUIRE(f.items.size() == 1u);

	REQUIRE(f.items[0].title == "this is an item");
	REQUIRE(f.items[0].link ==
		"http://example.com/blog/this_is_an_item.html");
	REQUIRE(f.items[0].author == "Andreas Krennmair");
	REQUIRE(f.items[0].author_email == "blog@synflood.at");
	REQUIRE(f.items[0].content_encoded == "oh well, this is the content.");
	REQUIRE(f.items[0].pubDate == "Fri, 12 Dec 2008 02:36:10 +0100");
	REQUIRE(f.items[0].guid ==
		"http://example.com/blog/this_is_an_item.html");
	REQUIRE_FALSE(f.items[0].guid_isPermaLink);
}

TEST_CASE("Extracts data from RSS 1.0", "[rsspp::Parser]")
{
	rsspp::Parser p;
	rsspp::Feed f;

	REQUIRE_NOTHROW(f = p.parse_file("data/rss10_1.xml"_path));

	REQUIRE(f.rss_version == rsspp::Feed::RSS_1_0);

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

TEST_CASE("Extracts data from Atom 1.0", "[rsspp::Parser]")
{
	rsspp::Parser p;
	rsspp::Feed f;

	REQUIRE_NOTHROW(f = p.parse_file("data/atom10_1.xml"_path));

	REQUIRE(f.rss_version == rsspp::Feed::ATOM_1_0);

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
	REQUIRE(f.items[0].author == "A Person");

	REQUIRE(f.items[1].title == "A missing rel attribute");
	REQUIRE(f.items[1].title_type == "html");
	REQUIRE(f.items[1].link == "http://example.com/atom_testing.html");
	REQUIRE(f.items[1].guid == "tag:example.com,2008-12-30:/atom_testing1");
	REQUIRE(f.items[1].description == "some content");
	REQUIRE(f.items[1].base ==
		"http://example.com/entry/atom_testing.html");
	REQUIRE(f.items[1].author == "A different Person");

	REQUIRE(f.items[2].title == "alternate link isn't first");
	REQUIRE(f.items[2].title_type == "html");
	REQUIRE(f.items[2].link == "http://example.com/atom_testing.html");
	REQUIRE(f.items[2].guid == "tag:example.com,2008-12-30:/atom_testing2");
	REQUIRE(f.items[2].description == "some content");
	REQUIRE(f.items[2].base ==
		"http://example.com/content/atom_testing.html");
	REQUIRE(f.items[2].author == "Person A, Person B");
}

TEST_CASE("Extracts data from media:... tags in atom feed", "[rsspp::Parser]")
{
	rsspp::Parser p;
	rsspp::Feed f;

	REQUIRE_NOTHROW(f = p.parse_file("data/atom10_2.xml"_path));

	REQUIRE(f.rss_version == rsspp::Feed::ATOM_1_0);

	REQUIRE(f.title == "Media test feed");
	REQUIRE(f.title_type == "text");
	REQUIRE(f.pubDate == "Tue, 30 Dec 2008 18:26:15 +0000");
	REQUIRE(f.link == "http://example.com/");

	REQUIRE(f.items.size() == 6u);
	REQUIRE(f.items[0].title == "using regular content");
	REQUIRE(f.items[0].description == "regular html content");
	REQUIRE(f.items[0].description_mime_type == "text/html");
	REQUIRE(f.items[0].author == "A Person");

	REQUIRE(f.items[1].title == "using media:description");
	REQUIRE(f.items[1].description == "media plaintext content");
	REQUIRE(f.items[1].description_mime_type == "text/plain");
	REQUIRE(f.items[1].author == "John Doe");

	REQUIRE(f.items[2].title == "using multiple media tags");
	REQUIRE(f.items[2].description == "media html content");
	REQUIRE(f.items[2].description_mime_type == "text/html");
	REQUIRE(f.items[2].link == "http://example.com/player.html");
	REQUIRE(f.items[2].author == "John Doe");

	REQUIRE(f.items[3].title ==
		"using multiple media tags nested in group/content");
	REQUIRE(f.items[3].description == "nested media html content");
	REQUIRE(f.items[3].description_mime_type == "text/html");
	REQUIRE(f.items[3].link == "http://example.com/player.html");
	REQUIRE(f.items[3].author == "John Doe");
	REQUIRE(f.items[3].enclosures.size() == 1);
	REQUIRE(f.items[3].enclosures[0].description == "nested media html content");
	REQUIRE(f.items[3].enclosures[0].description_mime_type == "text/html");

	SECTION("media:{title,description,player} does not overwrite regular title, description, and link if they exist") {
		REQUIRE(f.items[4].title == "regular title");
		REQUIRE(f.items[4].description == "regular content");
		REQUIRE(f.items[4].description_mime_type == "text/html");
		REQUIRE(f.items[4].link == "http://example.com/regular-link");
		REQUIRE(f.items[4].author == "John Doe");
	}

	REQUIRE(f.items[5].title == "using media:content");
	REQUIRE(f.items[5].description == "regular content");
	REQUIRE(f.items[5].description_mime_type == "text/html");
	REQUIRE(f.items[5].link == "http://example.com/4.html");
	REQUIRE(f.items[5].enclosures.size() == 2);
	REQUIRE(f.items[5].enclosures[0].url == "http://example.com/media-test.png");
	REQUIRE(f.items[5].enclosures[0].type == "image/png");
	REQUIRE(f.items[5].enclosures[1].url == "http://example.com/movie.mov");
	REQUIRE(f.items[5].enclosures[1].type == "video/quicktime");
}

TEST_CASE("Extracts data from media:... tags in  RSS 2.0 feeds",
	"[rsspp::Parser]")
{
	rsspp::Parser p;
	rsspp::Feed f;

	REQUIRE_NOTHROW(f = p.parse_file("data/rss20_2.xml"_path));

	REQUIRE(f.title == "my weblog");
	REQUIRE(f.link == "http://example.com/blog/");
	REQUIRE(f.description == "my description");

	REQUIRE(f.items.size() == 3u);

	REQUIRE(f.items[0].title == "using multiple media tags");
	REQUIRE(f.items[0].description == "media html content");
	REQUIRE(f.items[0].description_mime_type == "text/html");
	REQUIRE(f.items[0].link == "http://example.com/player.html");

	REQUIRE(f.items[1].title ==
		"using multiple media tags nested in group/content");
	REQUIRE(f.items[1].description == "nested media html content");
	REQUIRE(f.items[1].description_mime_type == "text/html");
	REQUIRE(f.items[1].link == "http://example.com/player.html");
	REQUIRE(f.items[1].enclosures.size() == 1);
	REQUIRE(f.items[1].enclosures[0].description == "nested media html content");
	REQUIRE(f.items[1].enclosures[0].description_mime_type == "text/html");

	REQUIRE(f.items[2].enclosures.size() == 2);
	REQUIRE(f.items[2].enclosures[0].url == "http://example.com/media-test.png");
	REQUIRE(f.items[2].enclosures[0].type == "image/png");
	REQUIRE(f.items[2].enclosures[1].url == "http://example.com/movie.mov");
	REQUIRE(f.items[2].enclosures[1].type == "video/quicktime");
}

TEST_CASE("Multiple links in item", "[rsspp::Parser]")
{
	rsspp::Parser p;
	rsspp::Feed f;

	REQUIRE_NOTHROW(f = p.parse_file("data/multiple_item_links.atom"_path));

	REQUIRE(f.items.size() == 1u);

	REQUIRE(f.items[0].title == "Multiple links");
	REQUIRE(f.items[0].link == "http://www.test.org/tests");
}

TEST_CASE("Feed authors and source authors in atom feed", "[rsspp::Parser]")
{
	rsspp::Parser p;
	rsspp::Feed f;

	REQUIRE_NOTHROW(f = p.parse_file("data/atom10_feed_authors.xml"_path));

	REQUIRE(f.rss_version == rsspp::Feed::ATOM_1_0);

	REQUIRE(f.title == "Author Test Feed");
	REQUIRE(f.title_type == "text");
	REQUIRE(f.pubDate == "Mon, 28 Nov 2022 13:01:25 +0000");
	REQUIRE(f.link == "http://example.com/");

	REQUIRE(f.items.size() == 9u);
	REQUIRE(f.items[0].title == "Entry Without Author");
	REQUIRE(f.items[0].description == "Feed authors should be used.");
	REQUIRE(f.items[0].author == "First Feed Author, Second Feed Author");

	REQUIRE(f.items[1].title == "Entry With Single Author");
	REQUIRE(f.items[1].description == "Entry author should be used.");
	REQUIRE(f.items[1].author == "Entry Author");

	REQUIRE(f.items[2].title == "Entry With Multiple Authors");
	REQUIRE(f.items[2].description == "Both entry authors should be used.");
	REQUIRE(f.items[2].author == "Entry Author 1, Entry Author 2");

	REQUIRE(f.items[3].title == "Entry With Empty Author Names");
	REQUIRE(f.items[3].description == "Both feed authors should be used.");
	REQUIRE(f.items[3].author == "First Feed Author, Second Feed Author");

	REQUIRE(f.items[4].title == "Entry With Single Source Author");
	REQUIRE(f.items[4].description == "Entry source author should be used.");
	REQUIRE(f.items[4].author == "Entry Source Author");

	REQUIRE(f.items[5].title == "Entry With Multiple Source Authors");
	REQUIRE(f.items[5].description == "All entry source authors should be used.");
	REQUIRE(f.items[5].author ==
		"Entry Source Author 1, Entry Source Author 2, Entry Source Author 3");

	REQUIRE(f.items[6].title == "Entry With Empty Source Author Names");
	REQUIRE(f.items[6].description == "Both feed authors should be used.");
	REQUIRE(f.items[6].author == "First Feed Author, Second Feed Author");

	REQUIRE(f.items[7].title == "Entry With Single Author And Source Author");
	REQUIRE(f.items[7].description == "Entry author should be used.");
	REQUIRE(f.items[7].author == "Entry Author");

	REQUIRE(f.items[8].title == "Entry With Multiple Author And Source Authors");
	REQUIRE(f.items[8].description == "Both entry authors should be used.");
	REQUIRE(f.items[8].author == "Entry Author 1, Entry Author 2");
}

TEST_CASE("parse_url() extracts etag and lastmodified data", "[rsspp::Parser]")
{
	using namespace newsboat;

	auto feed_xml = test_helpers::read_binary_file("data/atom10_1.xml"_path);

	auto& test_server = test_helpers::HttpTestServer::get_instance();
	auto mock_registration = test_server.add_endpoint("/feed", {}, 200, {
		{"content-type", "text/xml"},
		{"ETag", "returned-etag"},
		{"Last-Modified", "Wed, 21 Oct 2015 07:28:00 GMT"},
	}, feed_xml);
	const auto address = test_server.get_address();
	const auto url = strprintf::fmt("http://%s/feed", address);

	rsspp::Parser parser;
	CurlHandle easyhandle;
	parser.parse_url(url, easyhandle);

	REQUIRE(parser.get_etag() == "returned-etag");
	REQUIRE(parser.get_last_modified() == 1445412480);
}

// Placeholders:
// %s: encoding
// %s: feed title
constexpr auto atom_feed_with_encoding =
	R"(<?xml version="1.0" encoding="%s"?>)"
	R"(<feed xmlns="http://www.w3.org/2005/Atom" xml:lang="en">)"
	R"(<id>tag:example.com</id>)"
	R"(<title type="text">%s</title>)"
	R"(<updated>2008-12-30T18:26:15Z</updated>)"
	R"(<entry>)"
	R"(<id>tag:example.com,2008-12-30:/atom_testing</id>)"
	R"(<title>regular title</title>)"
	R"(<updated>2008-12-30T20:04:15Z</updated>)"
	R"(</entry>)"
	R"(</feed>)";

TEST_CASE("parse_url() converts data if specified in xml encoding attribute",
	"[rsspp::Parser]")
{
	using namespace newsboat;

	constexpr auto title_utf8 = u8"タイトル"; // Japanese for "title"

	auto feed_xml_utf8 = strprintf::fmt(atom_feed_with_encoding, "utf-16", title_utf8);

	auto feed_xml_utf16 = utils::convert_text(feed_xml_utf8, "utf-16", "utf-8");
	auto feed_xml = std::vector<std::uint8_t>(feed_xml_utf16.begin(), feed_xml_utf16.end());

	auto& test_server = test_helpers::HttpTestServer::get_instance();
	auto mock_registration = test_server.add_endpoint("/feed", {}, 200, {
		{"content-type", "text/xml"},
	}, feed_xml);
	const auto address = test_server.get_address();
	const auto url = strprintf::fmt("http://%s/feed", address);

	rsspp::Parser parser;
	CurlHandle easyhandle;
	auto parsed_feed = parser.parse_url(url, easyhandle);

	REQUIRE(parsed_feed.items.size() == 1);
	REQUIRE(parsed_feed.title == title_utf8);
}

TEST_CASE("parse_url() only converts once even when encoding specified twice (xml encoding and http header)",
	"[rsspp::Parser]")
{
	using namespace newsboat;

	constexpr auto title_utf8 = u8"Prøve"; // Danish for "test"

	auto feed_xml_utf8 = strprintf::fmt(atom_feed_with_encoding, "iso-8859-1", title_utf8);

	auto feed_xml_iso8859_1 = utils::convert_text(feed_xml_utf8, "iso-8859-1", "utf-8");
	auto feed_xml = std::vector<std::uint8_t>(feed_xml_iso8859_1.begin(),
			feed_xml_iso8859_1.end());

	auto& test_server = test_helpers::HttpTestServer::get_instance();
	auto mock_registration = test_server.add_endpoint("/feed", {}, 200, {
		{"content-type", "text/xml; charset=iso-8859-1"},
	}, feed_xml);
	const auto address = test_server.get_address();
	const auto url = strprintf::fmt("http://%s/feed", address);

	rsspp::Parser parser;
	CurlHandle easyhandle;
	auto parsed_feed = parser.parse_url(url, easyhandle);

	REQUIRE(parsed_feed.items.size() == 1);
	REQUIRE(parsed_feed.title == title_utf8);
}

TEST_CASE("parse_url() uses xml encoding if specified encodings conflict (xml encoding vs http header)",
	"[rsspp::Parser]")
{
	using namespace newsboat;

	constexpr auto title_utf8 = u8"Prøve"; // Danish for "test"

	auto feed_xml_utf8 = strprintf::fmt(atom_feed_with_encoding, "iso-8859-1", title_utf8);

	auto feed_xml_iso8859_1 = utils::convert_text(feed_xml_utf8, "iso-8859-1", "utf-8");
	auto feed_xml = std::vector<std::uint8_t>(feed_xml_iso8859_1.begin(),
			feed_xml_iso8859_1.end());

	auto& test_server = test_helpers::HttpTestServer::get_instance();
	auto mock_registration = test_server.add_endpoint("/feed", {}, 200, {
		{"content-type", "text/xml; charset=utf-16"}, // Expected to be ignored
	}, feed_xml);
	const auto address = test_server.get_address();
	const auto url = strprintf::fmt("http://%s/feed", address);

	rsspp::Parser parser;
	CurlHandle easyhandle;
	auto parsed_feed = parser.parse_url(url, easyhandle);

	REQUIRE(parsed_feed.items.size() == 1);
	REQUIRE(parsed_feed.title == title_utf8);
}

// Placeholders:
// %s: feed title
constexpr auto atom_feed_without_encoding =
	R"(<?xml version="1.0"?>)"
	R"(<feed xmlns="http://www.w3.org/2005/Atom" xml:lang="en">)"
	R"(<id>tag:example.com</id>)"
	R"(<title type="text">%s</title>)"
	R"(<updated>2008-12-30T18:26:15Z</updated>)"
	R"(<entry>)"
	R"(<id>tag:example.com,2008-12-30:/atom_testing</id>)"
	R"(<title>regular title</title>)"
	R"(<updated>2008-12-30T20:04:15Z</updated>)"
	R"(</entry>)"
	R"(</feed>)";

TEST_CASE("parse_url() applies encoding specified in http header if no xml encoding specified",
	"[rsspp::Parser]")
{
	using namespace newsboat;

	constexpr auto title_utf8 = u8"Prøve"; // Danish for "test"

	auto feed_xml_utf8 = strprintf::fmt(atom_feed_without_encoding, title_utf8);

	auto feed_xml_iso8859_1 = utils::convert_text(feed_xml_utf8, "iso-8859-1", "utf-8");
	auto feed_xml = std::vector<std::uint8_t>(feed_xml_iso8859_1.begin(),
			feed_xml_iso8859_1.end());

	auto& test_server = test_helpers::HttpTestServer::get_instance();
	auto mock_registration = test_server.add_endpoint("/feed", {}, 200, {
		{"content-type", "text/xml; charset=iso-8859-1"},
	}, feed_xml);
	const auto address = test_server.get_address();
	const auto url = strprintf::fmt("http://%s/feed", address);

	rsspp::Parser parser;
	CurlHandle easyhandle;
	auto parsed_feed = parser.parse_url(url, easyhandle);

	REQUIRE(parsed_feed.items.size() == 1);
	REQUIRE(parsed_feed.title == title_utf8);
}

TEST_CASE("parse_url() assumes utf-8 if no encoding specified and replaces invalid code units",
	"[rsspp::Parser]")
{
	using namespace newsboat;

	constexpr auto title_utf8 = u8"Prøve"; // Danish for "test"
	constexpr auto expected_title = u8"Pr�ve";

	auto feed_xml_utf8 = strprintf::fmt(atom_feed_without_encoding, title_utf8);

	auto feed_xml_iso8859_1 = utils::convert_text(feed_xml_utf8, "iso-8859-1", "utf-8");
	auto feed_xml = std::vector<std::uint8_t>(feed_xml_iso8859_1.begin(),
			feed_xml_iso8859_1.end());

	auto& test_server = test_helpers::HttpTestServer::get_instance();
	auto mock_registration = test_server.add_endpoint("/feed", {}, 200, {}, feed_xml);
	const auto address = test_server.get_address();
	const auto url = strprintf::fmt("http://%s/feed", address);

	rsspp::Parser parser;
	CurlHandle easyhandle;
	auto parsed_feed = parser.parse_url(url, easyhandle);

	REQUIRE(parsed_feed.items.size() == 1);
	REQUIRE(parsed_feed.title == expected_title);
}

TEST_CASE("parse_url() with unsupported encoding falls back to libxml auto-detection",
	"[rsspp::Parser]")
{
	using namespace newsboat;

	constexpr auto title_utf8 = u8"Prøve"; // Danish for "test"

	auto feed_xml_utf8 = strprintf::fmt(atom_feed_without_encoding, title_utf8);

	auto feed_xml = std::vector<std::uint8_t>(feed_xml_utf8.begin(), feed_xml_utf8.end());

	auto& test_server = test_helpers::HttpTestServer::get_instance();
	auto mock_registration = test_server.add_endpoint("/feed", {}, 200, {
		{"content-type", "text/xml; charset=non-existent-encoding"},
	}, feed_xml);
	const auto address = test_server.get_address();
	const auto url = strprintf::fmt("http://%s/feed", address);

	rsspp::Parser parser;
	CurlHandle easyhandle;
	auto parsed_feed = parser.parse_url(url, easyhandle);

	REQUIRE(parsed_feed.items.size() == 1);
	REQUIRE(parsed_feed.title == title_utf8);
}

TEST_CASE("Throws if no \"channel\" element is found",
	"[rsspp::Parser][issue3108]")
{
	using test_helpers::ExceptionWithMsg;

	rsspp::Parser p;
	REQUIRE_THROWS_MATCHES(p.parse_buffer("<rss version=\"0.92\"><![CDATA[]]>"),
		rsspp::Exception,
		ExceptionWithMsg<rsspp::Exception>("no RSS channel found"));
}

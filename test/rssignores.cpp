#include "rssignores.h"

#include "3rd-party/catch.hpp"

using namespace newsboat;

TEST_CASE(
	"RssIgnores::matches_lastmodified() returns true if given url "
	"has to always be downloaded",
	"[rss]")
{
	RssIgnores ignores;
	ignores.handle_action("always-download", {
		"http://newsboat.org",
		"www.cool-website.com",
		"www.example.com"
	});

	REQUIRE(ignores.matches_lastmodified("www.example.com"));
	REQUIRE(ignores.matches_lastmodified("http://newsboat.org"));
	REQUIRE(ignores.matches_lastmodified("www.cool-website.com"));
	REQUIRE_FALSE(ignores.matches_lastmodified("www.smth.com"));
}

TEST_CASE(
	"RssIgnores::matches_resetunread() returns true if given url "
	"will always have its unread flag reset on update",
	"[rss]")
{
	RssIgnores ignores;
	ignores.handle_action("reset-unread-on-update", {
		"http://newsboat.org",
		"www.cool-website.com",
		"www.example.com"
	});

	REQUIRE(ignores.matches_resetunread("www.example.com"));
	REQUIRE(ignores.matches_resetunread("http://newsboat.org"));
	REQUIRE(ignores.matches_resetunread("www.cool-website.com"));
	REQUIRE_FALSE(ignores.matches_resetunread("www.smth.com"));
}

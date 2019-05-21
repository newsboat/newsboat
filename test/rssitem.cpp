#include "rssitem.h"

#include "3rd-party/catch.hpp"
#include "cache.h"
#include "configcontainer.h"

using namespace newsboat;

TEST_CASE("RssItem::sort_flags() cleans up flags", "[rss]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	RssItem item(&rsscache);

	SECTION("Repeated letters do not erase other letters")
	{
		std::string inputflags = "Abcdecf";
		std::string result = "Abcdef";
		item.set_flags(inputflags);
		REQUIRE(result == item.flags());
	}

	SECTION("Non alpha characters in input flags are ignored")
	{
		std::string inputflags = "Abcd";
		item.set_flags(inputflags + "1234568790^\"#'é(£");
		REQUIRE(inputflags == item.flags());
	}
}

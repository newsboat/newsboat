#include "links.h"

#include "3rd-party/catch.hpp"

using namespace newsboat;

TEST_CASE("Each URL is unique", "[Links]")
{
	Links links;

	links.add_link("https://newsboat.org/news.atom", LinkType::HREF);
	links.add_link("https://newsboat.org/news.atom", LinkType::HREF);
	REQUIRE(links.size() == 1);
}

TEST_CASE("Password and username are censored", "[Links]")
{
	Links links;

	links.add_link("http://user:pass@somesite.com/feed", LinkType::HREF);
	REQUIRE(links.begin()->first == "http://*:*@somesite.com/feed");
}


#include "links.h"

#include "3rd-party/catch.hpp"

using namespace Newsboat;

TEST_CASE("Each URL is unique", "[Links]")
{
	Links links;

	links.add_link("https://Newsboat.org/news.atom", LinkType::HREF);
	links.add_link("https://Newsboat.org/news.atom", LinkType::HREF);
	REQUIRE(links.size() == 1);
}

TEST_CASE("Password and username are censored", "[Links]")
{
	Links links;

	links.add_link("http://user:pass@somesite.com/feed", LinkType::HREF);
	REQUIRE(links.begin()->url == "http://*:*@somesite.com/feed");
	links.add_link("http://user:pass@somesite.com/feed", LinkType::HREF);
	REQUIRE(links.size() == 1);
}


#include "catch.hpp"

#include <urlreader.h>

using namespace newsbeuter;

TEST_CASE("URL reader extracts all URLs from the file", "[file_urlreader]") {
	file_urlreader u;
	u.load_config("data/test-urls.txt");

	REQUIRE(u.get_urls().size() == 3);
	REQUIRE(u.get_urls()[0] == "http://test1.url.cc/feed.xml");
	REQUIRE(u.get_urls()[1] == "http://anotherfeed.com/");
	REQUIRE(u.get_urls()[2] == "http://onemorefeed.at/feed/");
}

TEST_CASE("URL reader extracts feeds' tags", "[file_urlreader]") {
	file_urlreader u;
	u.load_config("data/test-urls.txt");

	REQUIRE(u.get_tags("http://test1.url.cc/feed.xml").size() == 2);
	REQUIRE(u.get_tags("http://test1.url.cc/feed.xml")[0] == "tag1");
	REQUIRE(u.get_tags("http://test1.url.cc/feed.xml")[1] == "tag2");

	REQUIRE(u.get_tags("http://anotherfeed.com/").size() == 0);
	REQUIRE(u.get_tags("http://onemorefeed.at/feed/").size() == 2);
}

TEST_CASE("URL reader keeps track of unique tags", "[file_urlreader]") {
	file_urlreader u;
	u.load_config("data/test-urls.txt");

	REQUIRE(u.get_alltags().size() == 3);
}

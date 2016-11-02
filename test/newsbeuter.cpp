#include "catch.hpp"

#include <configcontainer.h>
#include <cache.h>
#include <rss_parser.h>

#include <unistd.h>

using namespace newsbeuter;

TEST_CASE("Newsbeuter reload behaves correctly") {
	configcontainer cfg;
	cache rsscache(":memory:", &cfg);

	rss_parser parser("file://data/rss.xml", &rsscache, &cfg, nullptr);
	std::shared_ptr<rss_feed> feed = parser.parse();
	REQUIRE(feed->total_item_count() == 8);

	SECTION("externalization and internalization preserve number of items") {
		rsscache.externalize_rssfeed(feed, false);
		REQUIRE_NOTHROW(
				feed = rsscache.internalize_rssfeed(
					"file://data/rss.xml", nullptr));
		REQUIRE(feed->total_item_count() == 8);
	}

	SECTION("item titles are correct") {
		REQUIRE(feed->items()[0]->title() == "Teh Saxxi");
		REQUIRE(feed->items()[7]->title() == "Handy als IR-Detektor");

		SECTION("set_title() changes item title") {
			feed->items()[0]->set_title("Another Title");
			REQUIRE(feed->items()[0]->title() == "Another Title");
		}
	}

	std::vector<std::shared_ptr<rss_feed>> feedv;
	feedv.push_back(feed);

	cfg.set_configvalue("cleanup-on-quit", "true");
	rsscache.cleanup_cache(feedv);
}

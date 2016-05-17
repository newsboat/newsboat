#include "catch.hpp"

#include <configcontainer.h>
#include <cache.h>
#include <rss_parser.h>

#include <unistd.h>

using namespace newsbeuter;

TEST_CASE("Newsbeuter reload behaves correctly") {
	configcontainer * cfg = new configcontainer();
	cache * rsscache = new cache("test-cache.db", cfg);

	rss_parser parser("file://data/rss.xml", rsscache, cfg, NULL);
	std::shared_ptr<rss_feed> feed = parser.parse();
	REQUIRE(feed->items().size() == 8);

	SECTION("externalization and internalization preserve number of items") {
		rsscache->externalize_rssfeed(feed, false);
		feed = rsscache->internalize_rssfeed("file://data/rss.xml", NULL);
		REQUIRE(feed->items().size() == 8);

			SECTION("cache contains correct feed URLs") {
				std::vector<std::string> feedurls = rsscache->get_feed_urls();
				REQUIRE(feedurls.size() == 1);
				REQUIRE(feedurls[0] == "file://data/rss.xml");
			}
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

	cfg->set_configvalue("cleanup-on-quit", "true");
	rsscache->cleanup_cache(feedv);

	delete rsscache;
	delete cfg;

	::unlink("test-cache.db");
}

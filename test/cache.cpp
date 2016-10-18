#include "catch.hpp"

#include <cache.h>
#include <configcontainer.h>
#include <rss_parser.h>

#include <unistd.h>

using namespace newsbeuter;

TEST_CASE("cache behaves correctly") {
	configcontainer * cfg = new configcontainer();
	cache * rsscache = new cache("test-cache.db", cfg);
	rss_parser parser("file://data/rss.xml", rsscache, cfg, nullptr);
	std::shared_ptr<rss_feed> feed = parser.parse();
	REQUIRE(feed->total_item_count() == 8);
	rsscache->externalize_rssfeed(feed, false);

	SECTION("items in search result are marked as read") {
		auto search_items = rsscache->search_for_items("Botox", "");
		REQUIRE(search_items.size() == 1);
		auto item = search_items.front();
		REQUIRE(true == item->unread());

		item->set_unread(false);
		search_items.clear();

		search_items = rsscache->search_for_items("Botox", "");
		REQUIRE(search_items.size() == 1);
		auto updatedItem = search_items.front();
		REQUIRE(false == updatedItem->unread());
	}

	std::vector<std::shared_ptr<rss_feed>> feedv;
	feedv.push_back(feed);

	cfg->set_configvalue("cleanup-on-quit", "true");
	rsscache->cleanup_cache(feedv);

	delete rsscache;
	delete cfg;

	::unlink("test-cache.db");
}

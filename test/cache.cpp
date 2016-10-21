#include "catch.hpp"
#include "test-helpers.h"

#include <cache.h>
#include <configcontainer.h>
#include <rss_parser.h>

using namespace newsbeuter;

TEST_CASE("cache behaves correctly") {
	configcontainer cfg;
	cache rsscache(":memory:", &cfg);
	rss_parser parser("file://data/rss.xml", &rsscache, &cfg, nullptr);
	std::shared_ptr<rss_feed> feed = parser.parse();
	REQUIRE(feed->total_item_count() == 8);
	rsscache.externalize_rssfeed(feed, false);

	SECTION("items in search result are marked as read") {
		auto search_items = rsscache.search_for_items("Botox", "");
		REQUIRE(search_items.size() == 1);
		auto item = search_items.front();
		REQUIRE(true == item->unread());

		item->set_unread(false);
		search_items.clear();

		search_items = rsscache.search_for_items("Botox", "");
		REQUIRE(search_items.size() == 1);
		auto updatedItem = search_items.front();
		REQUIRE(false == updatedItem->unread());
	}

	std::vector<std::shared_ptr<rss_feed>> feedv;
	feedv.push_back(feed);

	cfg.set_configvalue("cleanup-on-quit", "true");
	rsscache.cleanup_cache(feedv);
}

TEST_CASE("Cleaning old articles works") {
	TestHelpers::TempFile dbfile;
	std::unique_ptr<configcontainer> cfg( new configcontainer() );
	std::unique_ptr<cache> rsscache( new cache(dbfile.getPath(), cfg.get()) );
	rss_parser parser("file://data/rss.xml", rsscache.get(), cfg.get(), nullptr);
	std::shared_ptr<rss_feed> feed = parser.parse();

	/* Adding a fresh item that won't be deleted. If it survives the test, we
	 * will know that "keep-articles-days" really deletes the old articles
	 * *only* and not the whole database. */
	auto item = std::make_shared<rss_item>(rsscache.get());
	item->set_title("Test item");
	item->set_link("http://example.com/item");
	item->set_guid("http://example.com/item");
	item->set_author("Newsbeuter Testsuite");
	item->set_description("");
	item->set_pubDate(time(nullptr)); // current time
	item->set_unread(true);
	feed->add_item(item);

	rsscache->externalize_rssfeed(feed, false);

	/* Simulating a restart of Newsbeuter. */

	/* Setting "keep-articles-days" to non-zero value to trigger
	 * cache::clean_old_articles().
	 *
	 * The value of 42 days is sufficient because the items in the test feed
	 * are dating back to 2006. */
	cfg.reset( new configcontainer() );
	cfg->set_configvalue("keep-articles-days", "42");
	rsscache.reset( new cache(dbfile.getPath(), cfg.get()) );
	rss_ignores ign;
	feed = rsscache->internalize_rssfeed("file://data/rss.xml", &ign);

	/* The important part: old articles should be gone, new one remains. */
	REQUIRE(feed->items().size() == 1);
}

TEST_CASE("Last-Modified and ETag values are preserved correctly") {
	configcontainer cfg;
	cache rsscache(":memory:", &cfg);
	const auto feedurl = "file://data/rss.xml";
	rss_parser parser(feedurl, &rsscache, &cfg, nullptr);
	std::shared_ptr<rss_feed> feed = parser.parse();
	rsscache.externalize_rssfeed(feed, false);

	/* We will run this lambda on different inputs to check different
	 * situations. */
	auto test = [&](const time_t& lm_value, const std::string& etag_value) {
		time_t last_modified = lm_value;
		std::string etag = etag_value;

		rsscache.update_lastmodified(feedurl, last_modified, etag);
		/* Scrambling the value to make sure the following call changes it. */
		last_modified = 42;
		etag = "42";
		rsscache.fetch_lastmodified(feedurl, last_modified, etag);

		REQUIRE(last_modified == lm_value);
		REQUIRE(etag == etag_value);
	};

	SECTION("Only Last-Modified header was returned") {
		test(1476382350, "");
	}

	SECTION("Only ETag header was returned") {
		test(0, "1234567890");
	}

	SECTION("Both Last-Modified and ETag headers were returned") {
		test(1476382350, "1234567890");
	}
}

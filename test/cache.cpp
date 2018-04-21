#include "cache.h"

#include <sstream>

#include "3rd-party/catch.hpp"
#include "test-helpers.h"
#include "configcontainer.h"
#include "rss_parser.h"

using namespace newsboat;

TEST_CASE("items in search result can be marked read", "[cache]") {
	configcontainer cfg;
	cache rsscache(":memory:", &cfg);
	rss_parser parser("file://data/rss.xml", &rsscache, &cfg, nullptr);
	std::shared_ptr<rss_feed> feed = parser.parse();
	REQUIRE(feed->total_item_count() == 8);
	rsscache.externalize_rssfeed(feed, false);

	auto search_items = rsscache.search_for_items("Botox", "");
	REQUIRE(search_items.size() == 1);
	auto item = search_items.front();
	REQUIRE(item->unread());

	item->set_unread(false);
	search_items.clear();

	search_items = rsscache.search_for_items("Botox", "");
	REQUIRE(search_items.size() == 1);
	auto updatedItem = search_items.front();
	REQUIRE_FALSE(updatedItem->unread());
}

TEST_CASE("Cleaning old articles works", "[cache]") {
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
	item->set_author("Newsboat Testsuite");
	item->set_description("");
	item->set_pubDate(time(nullptr)); // current time
	item->set_unread(true);
	feed->add_item(item);

	rsscache->externalize_rssfeed(feed, false);

	/* Simulating a restart of Newsboat. */

	/* Setting "keep-articles-days" to non-zero value to trigger
	 * cache::clean_old_articles().
	 *
	 * The value of 42 days is sufficient because the items in the test feed
	 * are dating back to 2006. */
	cfg.reset( new configcontainer() );
	cfg->set_configvalue("keep-articles-days", "42");
	rsscache.reset( new cache(dbfile.getPath(), cfg.get()) );
	feed = rsscache->internalize_rssfeed("file://data/rss.xml", nullptr);

	/* The important part: old articles should be gone, new one remains. */
	REQUIRE(feed->items().size() == 1);
}

TEST_CASE("Last-Modified and ETag values are persisted to DB", "[cache]") {
	std::unique_ptr<configcontainer> cfg( new configcontainer() );
	TestHelpers::TempFile dbfile;
	std::unique_ptr<cache> rsscache( new cache(dbfile.getPath(), cfg.get()) );
	const auto feedurl = "file://data/rss.xml";
	rss_parser parser(feedurl, rsscache.get(), cfg.get(), nullptr);
	std::shared_ptr<rss_feed> feed = parser.parse();
	rsscache->externalize_rssfeed(feed, false);

	/* We will run this lambda on different inputs to check different
	 * situations. */
	auto test = [&](const time_t& lm_value, const std::string& etag_value) {
		time_t last_modified = lm_value;
		std::string etag = etag_value;

		REQUIRE_NOTHROW(
				rsscache->update_lastmodified(feedurl, last_modified, etag));

		cfg.reset( new configcontainer() );
		rsscache.reset( new cache(dbfile.getPath(), cfg.get()) );

		/* Scrambling the value to make sure the following call changes it. */
		last_modified = 42;
		etag = "42";
		rsscache->fetch_lastmodified(feedurl, last_modified, etag);

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

	SECTION("Neither Last-Modified nor ETag headers were returned") {
		test(0, "");
	}
}

TEST_CASE("mark_all_read marks all items in the feed read", "[cache]") {
	std::shared_ptr<rss_feed> feed, test_feed;

	configcontainer cfg;
	cache rsscache(":memory:", &cfg);

	test_feed = std::make_shared<rss_feed>(&rsscache);
	test_feed->set_title("Test feed");
	auto test_feed_url = "http://example.com/atom.xml";
	test_feed->set_link(test_feed_url);

	std::vector<std::pair<std::string, unsigned int>> feeds = {
		// { feed's URL, number of items in the feed }
		{ "file://data/rss.xml", 8 },
		{ "file://data/atom10_1.xml", 3 }
	};

	/* Ensure that the feeds contain expected number of items, then externalize
	 * them (put into cache). */
	for (const auto& feed_data : feeds) {
		rss_parser parser(feed_data.first, &rsscache, &cfg, nullptr);
		feed = parser.parse();
		REQUIRE(feed->total_item_count() == feed_data.second);

		test_feed->add_item(feed->items()[0]);

		rsscache.externalize_rssfeed(feed, false);
	}

	SECTION("empty feedurl") {
		INFO("All items should be marked as read.");
		rsscache.mark_all_read();

		for (const auto& feed_data : feeds) {
			feed = rsscache.internalize_rssfeed(feed_data.first, nullptr);
			for (const auto& item : feed->items()) {
				REQUIRE_FALSE(item->unread());
			}
		}
	}

	SECTION("non-empty feedurl") {
		INFO("All items with particular feedurl should be marked as read");
		rsscache.mark_all_read(feeds[0].first);

		INFO("First feed should all be marked read");
		feed = rsscache.internalize_rssfeed(feeds[0].first, nullptr);
		for (const auto& item : feed->items()) {
			REQUIRE_FALSE(item->unread());
		}

		INFO("Second feed should all be marked unread");
		feed = rsscache.internalize_rssfeed(feeds[1].first, nullptr);
		for (const auto& item : feed->items()) {
			REQUIRE(item->unread());
		}
	}

	SECTION("actual feed") {
		INFO("All items that are in the specific feed should be marked as read");
		rsscache.externalize_rssfeed(test_feed, false);
		rsscache.mark_all_read(test_feed);

		/* Since test_feed contains the first item of each feed, only these two
		 * items should be marked read. */
		auto unread_items_count = [](std::shared_ptr<rss_feed>& feed) {
			unsigned int count = 0;
			for (const auto& item : feed->items()) {
				if (item->unread()) {
					count++;
				}
			}
			return count;
		};

		{
			feed = rsscache.internalize_rssfeed(test_feed_url, nullptr);
			INFO("Test feed should all be marked read");
			REQUIRE(unread_items_count(feed) == 0);
		}

		{
			feed = rsscache.internalize_rssfeed(feeds[0].first, nullptr);
			INFO("First feed should have just one item read");
			REQUIRE(unread_items_count(feed) == (feeds[0].second - 1));
		}

		{
			feed = rsscache.internalize_rssfeed(feeds[1].first, nullptr);
			INFO("Second feed should have just one item read");
			REQUIRE(unread_items_count(feed) == (feeds[1].second - 1));
		}
	}
}

TEST_CASE("cleanup_cache is controlled by `cleanup-on-quit` "
		"and `delete-read-articles-on-quit` settings", "[cache]") {
	TestHelpers::TempFile dbfile;

	std::vector<std::string> feedurls = {
		"file://data/rss.xml",
		"file://data/atom10_1.xml"
	};

	std::vector<std::shared_ptr<rss_feed>> feeds;
	std::unique_ptr<configcontainer> cfg( new configcontainer() );
	std::unique_ptr<cache> rsscache( new cache(dbfile.getPath(), cfg.get()) );
	for (const auto& url : feedurls) {
		rss_parser parser(url, rsscache.get(), cfg.get(), nullptr);
		std::shared_ptr<rss_feed> feed = parser.parse();
		feeds.push_back(feed);
		rsscache->externalize_rssfeed(feed, false);
	}

	SECTION("cleanup-on-quit set to \"no\"") {
		cfg->set_configvalue("cleanup-on-quit", "no");
		rsscache->cleanup_cache(feeds);

		cfg.reset( new configcontainer() );
		rsscache.reset( new cache(dbfile.getPath(), cfg.get()) );

		for (const auto& url : feedurls) {
			std::shared_ptr<rss_feed> feed =
				rsscache->internalize_rssfeed(url, nullptr);
			REQUIRE(feed->total_item_count() != 0);
		}
	}

	SECTION("cleanup-on-quit set to \"yes\"") {
		cfg->set_configvalue("cleanup-on-quit", "yes");

		SECTION("delete-read-articles-on-quit set to \"no\"") {
			/* Drop first feed; it should now be removed from the cache, too. */
			feeds.erase(feeds.cbegin(), feeds.cbegin()+1);
			rsscache->cleanup_cache(feeds);

			cfg.reset( new configcontainer() );
			rsscache.reset( new cache(dbfile.getPath(), cfg.get()) );

			std::shared_ptr<rss_feed> feed =
				rsscache->internalize_rssfeed(feedurls[0], nullptr);
			REQUIRE(feed->total_item_count() == 0);
			feed = rsscache->internalize_rssfeed(feedurls[1], nullptr);
			REQUIRE(feed->total_item_count() != 0);
		}

		SECTION("delete-read-articles-on-quit set to \"yes\"") {
			cfg->set_configvalue("delete-read-articles-on-quit", "yes");
			REQUIRE(feeds[0]->total_item_count() == 8);
			feeds[0]->items()[0]->set_unread(false);
			feeds[0]->items()[1]->set_unread(false);

			rsscache->cleanup_cache(feeds);

			cfg.reset( new configcontainer() );
			rsscache.reset( new cache(dbfile.getPath(), cfg.get()) );

			std::shared_ptr<rss_feed> feed =
				rsscache->internalize_rssfeed(feedurls[0], nullptr);
			REQUIRE(feed->total_item_count() == 6);
		}
	}
}

TEST_CASE("fetch_descriptions fills out feed item's descriptions", "[cache]") {
	configcontainer cfg;
	cache rsscache(":memory:", &cfg);
	const auto feedurl = "file://data/rss.xml";
	rss_parser parser(feedurl, &rsscache, &cfg, nullptr);
	std::shared_ptr<rss_feed> feed = parser.parse();

	rsscache.externalize_rssfeed(feed, false);

	for (auto& item : feed->items()) {
		item->set_description("your test failed!");
	}

	REQUIRE_NOTHROW(rsscache.fetch_descriptions(feed.get()));

	for (auto& item : feed->items()) {
		REQUIRE(item->description() != "your test failed!");
	}
}

TEST_CASE("get_unread_count returns number of yet unread articles", "[cache]") {
	TestHelpers::TempFile dbfile;
	configcontainer cfg;
	std::unique_ptr<cache> rsscache( new cache(dbfile.getPath(), &cfg) );
	std::unique_ptr<rss_parser>
		parser( new rss_parser(
					"file://data/rss.xml",
					rsscache.get(),
					&cfg,
					nullptr) );
	std::shared_ptr<rss_feed> feed = parser->parse();
	// Marking one article as read to make sure get_unread_count() really
	// counts only unread articles
	feed->items()[0]->set_unread(false);
	rsscache->externalize_rssfeed(feed, false);

	REQUIRE(rsscache->get_unread_count() == 7);

	// Let's add another article to make sure get_unread_count looks at all
	// feeds present in the cache
	parser.reset( new rss_parser(
				"file://data/atom10_1.xml",
				rsscache.get(),
				&cfg,
				nullptr) );
	feed = parser->parse();
	feed->items()[0]->set_unread(false);
	feed->items()[2]->set_unread(false);
	rsscache->externalize_rssfeed(feed, false);
	REQUIRE(rsscache->get_unread_count() == 8);

	// Lastly, let's make sure the info is indeed retrieved from the database
	// and isn't just stored in the cache object
	rsscache.reset( new cache(dbfile.getPath(), &cfg) );
	REQUIRE(rsscache->get_unread_count() == 8);
}

TEST_CASE("get_read_item_guids returns GUIDs of items that are marked read",
          "[cache]") {
	TestHelpers::TempFile dbfile;
	configcontainer cfg;
	std::unique_ptr<cache> rsscache( new cache(dbfile.getPath(), &cfg) );

	// We'll keep our own count of which GUIDs are unread
	std::unordered_set<std::string> read_guids;
	std::unique_ptr<rss_parser> parser(
		new rss_parser(
			"file://data/rss.xml",
			rsscache.get(),
			&cfg,
			nullptr) );
	std::shared_ptr<rss_feed> feed = parser->parse();

	auto mark_read = [&read_guids](std::shared_ptr<rss_item> item) {
		item->set_unread(false);
		INFO("add  " + item->guid());
		read_guids.insert(item->guid());
	};

	// This function will be used to check if the result is consistent with our
	// count
	auto check = [&read_guids](const std::vector<std::string>& result) {
		auto local = read_guids;
		REQUIRE(local.size() != 0);

		for (const auto& guid : result) {
			INFO("check " + guid);
			auto it = local.find(guid);
			REQUIRE(it != local.end());
			local.erase(it);
		}

		REQUIRE(local.size() == 0);
	};

	mark_read(feed->items()[0]);
	rsscache->externalize_rssfeed(feed, false);

	INFO("Testing on single feed");
	check(rsscache->get_read_item_guids());

	// Let's add another article to make sure get_unread_count looks at all
	// feeds present in the cache
	parser.reset( new rss_parser(
			"file://data/atom10_1.xml",
			rsscache.get(),
			&cfg,
			nullptr) );
	feed = parser->parse();
	mark_read(feed->items()[0]);
	mark_read(feed->items()[2]);

	rsscache->externalize_rssfeed(feed, false);
	INFO("Testing on two feeds");
	check(rsscache->get_read_item_guids());

	// Lastly, let's make sure the info is indeed retrieved from the database
	// and isn't just stored in the cache object
	rsscache.reset( new cache(dbfile.getPath(), &cfg) );
	INFO("Testing on two feeds with new `cache` object");
	check(rsscache->get_read_item_guids());
}

TEST_CASE("mark_item_deleted changes \"deleted\" flag of item with given GUID ",
          "[cache]") {
	TestHelpers::TempFile dbfile;
	configcontainer cfg;
	std::unique_ptr<cache> rsscache( new cache(dbfile.getPath(), &cfg) );
	auto feedurl = "file://data/rss.xml";
	rss_parser parser(feedurl, rsscache.get(), &cfg, nullptr);
	std::shared_ptr<rss_feed> feed = parser.parse();

	auto item = feed->items()[1];
	auto guid = item->guid();
	REQUIRE(feed->total_item_count() == 8);
	rsscache->externalize_rssfeed(feed, false);
	rsscache->mark_item_deleted(guid, true);

	rsscache.reset( new cache(dbfile.getPath(), &cfg) );
	feed = rsscache->internalize_rssfeed(feedurl, nullptr);
	// One item was deleted, so shouldn't have been loaded
	REQUIRE(feed->total_item_count() == 7);
}

TEST_CASE("mark_items_read_by_guid marks items with given GUIDs as unread ",
          "[cache]") {
	TestHelpers::TempFile dbfile;
	configcontainer cfg;
	std::unique_ptr<cache> rsscache( new cache(dbfile.getPath(), &cfg) );
	auto feedurl = "file://data/rss.xml";
	rss_parser parser(feedurl, rsscache.get(), &cfg, nullptr);
	std::shared_ptr<rss_feed> feed = parser.parse();

	REQUIRE(feed->unread_item_count() == 8);

	SECTION("Not marking any items read") {
		rsscache->externalize_rssfeed(feed, false);

		REQUIRE_NOTHROW(rsscache->mark_items_read_by_guid({}));

		rsscache.reset( new cache(dbfile.getPath(), &cfg) );
		feed = rsscache->internalize_rssfeed(feedurl, nullptr);
		REQUIRE(feed->unread_item_count() == 8);
	}

	SECTION("Marking two items read") {
		auto guids = { feed->items()[0]->guid(), feed->items()[2]->guid() };
		rsscache->externalize_rssfeed(feed, false);

		REQUIRE_NOTHROW(rsscache->mark_items_read_by_guid(guids));

		rsscache.reset( new cache(dbfile.getPath(), &cfg) );
		feed = rsscache->internalize_rssfeed(feedurl, nullptr);
		REQUIRE(feed->unread_item_count() == 6);
	}
}

TEST_CASE("remove_old_deleted_items removes deleted items with particular "
          "feedurl if its GUID is not in the vector", "[cache]") {
	configcontainer cfg;
	cache rsscache(":memory:", &cfg);
	auto feedurl = "file://data/rss.xml";
	rss_parser parser(feedurl, &rsscache, &cfg, nullptr);
	std::shared_ptr<rss_feed> feed = parser.parse();

	REQUIRE(feed->total_item_count() == 8);

	std::vector<std::string> should_be_absent = {
		feed->items()[0]->guid(),
		feed->items()[3]->guid()
	};
	std::vector<std::string> should_be_present = {
		feed->items()[2]->guid(),
		feed->items()[5]->guid()
	};

	rsscache.externalize_rssfeed(feed, false);

	for (const auto& guid : should_be_absent) {
		rsscache.mark_item_deleted(guid, true);
	}
	for (const auto& guid : should_be_present) {
		rsscache.mark_item_deleted(guid, true);
	}

	REQUIRE_NOTHROW(
			rsscache.remove_old_deleted_items(
				feed->rssurl(),
				should_be_present));

	// Trying to "undelete" items
	for (const auto& guid : should_be_absent) {
		rsscache.mark_item_deleted(guid, false);
	}
	for (const auto& guid : should_be_present) {
		rsscache.mark_item_deleted(guid, false);
	}

	feed = rsscache.internalize_rssfeed(feedurl, nullptr);
	// Two items should've been removed by remove_old_deleted_items
	REQUIRE(feed->total_item_count() == 6);
}

TEST_CASE("search_for_items finds all items with matching title or content",
          "[cache]") {
	configcontainer cfg;
	cache rsscache(":memory:", &cfg);
	std::vector<std::string> feedurls = {
		"file://data/atom10_1.xml",
		"file://data/rss20_1.xml"
	};
	for (const auto& url : feedurls) {
		rss_parser parser(url, &rsscache, &cfg, nullptr);
		std::shared_ptr<rss_feed> feed = parser.parse();

		rsscache.externalize_rssfeed(feed, false);
	}

	auto query = "content";
	std::vector<std::shared_ptr<rss_item>> items;

	SECTION("Search the whole DB") {
		REQUIRE_NOTHROW(items = rsscache.search_for_items(query, ""));
		REQUIRE(items.size() == 4);
	}

	SECTION("Search specific feed") {
		REQUIRE_NOTHROW(items = rsscache.search_for_items(query, feedurls[0]));
		REQUIRE(items.size() == 3);

		REQUIRE_NOTHROW(items = rsscache.search_for_items(query, feedurls[1]));
		REQUIRE(items.size() == 1);
	}
}

TEST_CASE("update_rssitem_flags dumps `rss_item` object's flags to DB",
          "[cache]") {
	TestHelpers::TempFile dbfile;
	configcontainer cfg;
	std::unique_ptr<cache> rsscache( new cache(dbfile.getPath(), &cfg) );
	const auto feedurl = "file://data/rss.xml";
	rss_parser parser(feedurl, rsscache.get(), &cfg, nullptr);
	std::shared_ptr<rss_feed> feed = parser.parse();
	rsscache->externalize_rssfeed(feed, false);

	auto item = feed->items()[0];
	item->set_flags("abc");
	REQUIRE_NOTHROW(rsscache->update_rssitem_flags(item.get()));

	rsscache.reset( new cache(dbfile.getPath(), &cfg) );
	feed = rsscache->internalize_rssfeed("file://data/rss.xml", nullptr);

	REQUIRE(feed->items()[0]->flags() == "abc");
}

TEST_CASE("update_rssitem_unread_and_enqueued updates item's \"unread\" and "
          "\"enqueued\" fields", "[cache]") {
	TestHelpers::TempFile dbfile;
	configcontainer cfg;
	std::unique_ptr<cache> rsscache( new cache(dbfile.getPath(), &cfg) );
	const auto feedurl = "file://data/rss.xml";
	rss_parser parser(feedurl, rsscache.get(), &cfg, nullptr);
	std::shared_ptr<rss_feed> feed = parser.parse();

	auto item = feed->items()[0];

	rsscache->externalize_rssfeed(feed, false);

	SECTION("\"unread\" field is updated") {
		REQUIRE(item->unread());
		item->set_unread_nowrite(false);

		REQUIRE_NOTHROW(
				rsscache->update_rssitem_unread_and_enqueued(item, feedurl));
		rsscache.reset( new cache(dbfile.getPath(), &cfg) );
		feed = rsscache->internalize_rssfeed(feedurl, nullptr);

		REQUIRE_FALSE(feed->items()[0]->unread());
	}

	SECTION("\"enqueued\" field is updated") {
		REQUIRE_FALSE(item->enqueued());
		item->set_enqueued(true);

		REQUIRE_NOTHROW(
				rsscache->update_rssitem_unread_and_enqueued(item, feedurl));
		rsscache.reset( new cache(dbfile.getPath(), &cfg) );
		feed = rsscache->internalize_rssfeed(feedurl, nullptr);

		REQUIRE(feed->items()[0]->enqueued());
	}

	SECTION("both \"unread\" and \"enqueued\" fields are updated") {
		REQUIRE(item->unread());
		item->set_unread_nowrite(false);
		REQUIRE_FALSE(item->enqueued());
		item->set_enqueued(true);

		REQUIRE_NOTHROW(
				rsscache->update_rssitem_unread_and_enqueued(item, feedurl));
		rsscache.reset( new cache(dbfile.getPath(), &cfg) );
		feed = rsscache->internalize_rssfeed(feedurl, nullptr);

		REQUIRE_FALSE(feed->items()[0]->unread());
		REQUIRE(feed->items()[0]->enqueued());
	}
}

TEST_CASE("{externalize,internalize}_rssfeed puts a feed into DB and gets it back",
          "[cache]") {
	auto feeds_are_the_same =
		[](const std::shared_ptr<rss_feed>& feed1,
		   const std::shared_ptr<rss_feed>& feed2)
		{
			REQUIRE(feed1->title_raw()         == feed2->title_raw());
			REQUIRE(feed1->title()             == feed2->title());
			REQUIRE(feed1->description_raw()   == feed2->description_raw());
			REQUIRE(feed1->description()       == feed2->description());
			REQUIRE(feed1->link()              == feed2->link());
			REQUIRE(feed1->rssurl()            == feed2->rssurl());
			REQUIRE(feed1->unread_item_count() == feed2->unread_item_count());
			REQUIRE(feed1->total_item_count()  == feed2->total_item_count());
			REQUIRE(feed1->is_rtl()            == feed2->is_rtl());

			auto fst_it  = feed1->items().cbegin();
			auto fst_end = feed1->items().cend();
			auto snd_it  = feed2->items().cbegin();
			auto snd_end = feed2->items().cend();

			for (
				;
				fst_it != fst_end && snd_it != snd_end;
				++fst_it, ++snd_it)
			{
				REQUIRE((*fst_it)->guid()              == (*snd_it)->guid());
				REQUIRE((*fst_it)->title()             == (*snd_it)->title());
				REQUIRE((*fst_it)->title_raw()         == (*snd_it)->title_raw());
				REQUIRE((*fst_it)->link()              == (*snd_it)->link());
				REQUIRE((*fst_it)->author()            == (*snd_it)->author());
				REQUIRE((*fst_it)->author_raw()        == (*snd_it)->author_raw());
				REQUIRE((*fst_it)->description()       == (*snd_it)->description());
				REQUIRE((*fst_it)->description_raw()   == (*snd_it)->description_raw());
				REQUIRE((*fst_it)->size()              == (*snd_it)->size());
				REQUIRE((*fst_it)->length()            == (*snd_it)->length());
				REQUIRE((*fst_it)->pubDate()           == (*snd_it)->pubDate());
				REQUIRE((*fst_it)->pubDate_timestamp() == (*snd_it)->pubDate_timestamp());
				REQUIRE((*fst_it)->unread()            == (*snd_it)->unread());
				REQUIRE((*fst_it)->feedurl()           == (*snd_it)->feedurl());
				REQUIRE((*fst_it)->enclosure_url()     == (*snd_it)->enclosure_url());
				REQUIRE((*fst_it)->enclosure_type()    == (*snd_it)->enclosure_type());
				REQUIRE((*fst_it)->enqueued()          == (*snd_it)->enqueued());
				REQUIRE((*fst_it)->flags()             == (*snd_it)->flags());
				REQUIRE((*fst_it)->deleted()           == (*snd_it)->deleted());
			}
		};

	TestHelpers::TempFile dbfile;
	configcontainer cfg;
	std::unique_ptr<cache> rsscache( new cache(dbfile.getPath(), &cfg) );
	const auto feedurl = "file://data/rss.xml";
	rss_parser parser(feedurl, rsscache.get(), &cfg, nullptr);
	auto initial_feed = parser.parse();
	initial_feed->load();
	initial_feed->unload();
	initial_feed->load();

	SECTION("Simple case") {
		rsscache->externalize_rssfeed(initial_feed, false);

		rsscache.reset( new cache(dbfile.getPath(), &cfg) );
		auto new_feed = rsscache->internalize_rssfeed(feedurl, nullptr);
		new_feed->load();

		feeds_are_the_same(initial_feed, new_feed);
	}

	SECTION("Calling externalize_rssfeed twice in a row shouldn't break anything") {
		rsscache->externalize_rssfeed(initial_feed, false);
		rsscache->externalize_rssfeed(initial_feed, false);

		rsscache.reset( new cache(dbfile.getPath(), &cfg) );
		auto new_feed = rsscache->internalize_rssfeed(feedurl, nullptr);
		new_feed->load();

		feeds_are_the_same(initial_feed, new_feed);
	}
}

TEST_CASE("externalize_rssfeed doesn't store more than `max-items` items",
          "[cache]") {
	TestHelpers::TempFile dbfile;
	std::unique_ptr<configcontainer> cfg( new configcontainer() );
	std::unique_ptr<cache> rsscache( new cache(dbfile.getPath(), cfg.get()) );

	cfg->set_configvalue("max-items", "3");
	const auto feedurl = "file://data/rss.xml";
	rss_parser parser(feedurl, rsscache.get(), cfg.get(), nullptr);
	auto feed = parser.parse();
	REQUIRE(feed->total_item_count() == 8);
	rsscache->externalize_rssfeed(feed, false);

	cfg.reset( new configcontainer() );
	rsscache.reset( new cache(dbfile.getPath(), cfg.get()) );
	feed = rsscache->internalize_rssfeed(feedurl, nullptr);
	REQUIRE(feed->total_item_count() == 3);
}

TEST_CASE("externalize_rssfeed does nothing if it's passed a query feed",
		"[cache]")
{

}

TEST_CASE("internalize_rssfeed doesn't return more than `max-items` items, "
          "not counting the flagged ones", "[cache]")
{
	TestHelpers::TempFile dbfile;
	std::unique_ptr<configcontainer> cfg( new configcontainer() );
	std::unique_ptr<cache> rsscache( new cache(dbfile.getPath(), cfg.get()) );

	const auto feedurl = "file://data/rss.xml";
	rss_parser parser(feedurl, rsscache.get(), cfg.get(), nullptr);
	auto feed = parser.parse();
	REQUIRE(feed->total_item_count() == 8);

	SECTION("no flagged items") {
		rsscache->externalize_rssfeed(feed, false);

		cfg.reset( new configcontainer() );
		cfg->set_configvalue("max-items", "3");
		rsscache.reset( new cache(dbfile.getPath(), cfg.get()) );
		feed = rsscache->internalize_rssfeed(feedurl, nullptr);
		REQUIRE(feed->total_item_count() == 3);
	}

	SECTION("a couple of flagged items") {
		feed->items()[0]->set_flags("a");
		feed->items()[1]->set_flags("b");
		rsscache->externalize_rssfeed(feed, false);
		rsscache->update_rssitem_flags(feed->items()[0].get());
		rsscache->update_rssitem_flags(feed->items()[1].get());

		cfg.reset( new configcontainer() );
		cfg->set_configvalue("max-items", "1");
		rsscache.reset( new cache(dbfile.getPath(), cfg.get()) );
		feed = rsscache->internalize_rssfeed(feedurl, nullptr);
		// All flagged items should be present no matter what
		unsigned int flagged_count = 0;
		for (const auto& item : feed->items()) {
			if (item->flags() != "") {
				flagged_count++;
			}
		}
		REQUIRE(flagged_count == 2);
	}
}

TEST_CASE("internalize_rssfeed returns feed without items but specified RSS URL",
          "[cache]") {
	configcontainer cfg;
	cache rsscache(":memory:", &cfg);
	auto feedurl = "query:misc:age between 0:10";
	auto feed = rsscache.internalize_rssfeed(feedurl, nullptr);

	REQUIRE(feed->total_item_count() == 0);
	REQUIRE(feed->rssurl() == feedurl);
}

TEST_CASE("internalize_rssfeed doesn't return items that are ignored",
		"[cache]")
{
	configcontainer cfg;
	cache rsscache(":memory:", &cfg);

	const std::string feedurl("file://data/rss092_1.xml");
	rss_parser parser(feedurl, &rsscache, &cfg, nullptr);
	auto feed = parser.parse();
	REQUIRE(feed->total_item_count() == 3);
	rsscache.externalize_rssfeed(feed, false);

	rss_ignores ign;
	ign.handle_action("ignore-article", {"*", "title =~ \"third\""});

	feed = rsscache.internalize_rssfeed(feedurl, nullptr);
	REQUIRE(feed->total_item_count() == 3);
	feed = rsscache.internalize_rssfeed(feedurl, &ign);
	REQUIRE(feed->total_item_count() == 2);
}

TEST_CASE("externalize_rssfeed resets \"unread\" field if item's content "
          "changed and reset_unread = \"yes\"", "[cache]") {
	TestHelpers::TempFile dbfile;
	configcontainer cfg;
	std::unique_ptr<cache> rsscache( new cache(dbfile.getPath(), &cfg) );
	auto feedurl = "file://data/rss.xml";
	rss_parser parser(feedurl, rsscache.get(), &cfg, nullptr);
	std::shared_ptr<rss_feed> feed = parser.parse();
	feed->items()[0]->set_unread_nowrite(false);
	rsscache->externalize_rssfeed(feed, false);

	rsscache.reset( new cache(dbfile.getPath(), &cfg) );
	feed = rsscache->internalize_rssfeed(feedurl, nullptr);
	feed->load();
	REQUIRE_FALSE(feed->items()[0]->unread());
	feed->items()[0]->set_unread_nowrite(true);
	feed->items()[0]->set_description("changed!");

	SECTION("reset_unread = false; item remains read") {
		rsscache->externalize_rssfeed(feed, false);
		rsscache.reset( new cache(dbfile.getPath(), &cfg) );
		feed = rsscache->internalize_rssfeed(feedurl, nullptr);
		REQUIRE_FALSE(feed->items()[0]->unread());
	}

	SECTION("reset_unread = true; item becomes unread") {
		rsscache->externalize_rssfeed(feed, true);
		rsscache.reset( new cache(dbfile.getPath(), &cfg) );
		feed = rsscache->internalize_rssfeed(feedurl, nullptr);
		REQUIRE(feed->items()[0]->unread());
	}
}

TEST_CASE("externalize_rssfeed only updates \"unread\" field if override_unread is set",
          "[cache]") {
	TestHelpers::TempFile dbfile;
	configcontainer cfg;
	std::unique_ptr<cache> rsscache( new cache(dbfile.getPath(), &cfg) );
	auto feedurl = "file://data/rss.xml";
	rss_parser parser(feedurl, rsscache.get(), &cfg, nullptr);
	std::shared_ptr<rss_feed> feed = parser.parse();
	feed->items()[0]->set_unread_nowrite(false);
	rsscache->externalize_rssfeed(feed, false);

	rsscache.reset( new cache(dbfile.getPath(), &cfg) );
	feed = rsscache->internalize_rssfeed(feedurl, nullptr);
	auto item = feed->items()[0];
	item->set_unread_nowrite(true);

	SECTION("override_unread not set; item remains read") {
		rsscache->externalize_rssfeed(feed, false);
		rsscache.reset( new cache(dbfile.getPath(), &cfg) );
		feed = rsscache->internalize_rssfeed(feedurl, nullptr);
		REQUIRE_FALSE(feed->items()[0]->unread());
	}

	SECTION("override_unread is set; item becomes unread") {
		item->set_override_unread(true);
		rsscache->externalize_rssfeed(feed, false);
		rsscache.reset( new cache(dbfile.getPath(), &cfg) );
		feed = rsscache->internalize_rssfeed(feedurl, nullptr);
		REQUIRE(feed->items()[0]->unread());
	}
}

TEST_CASE("externalize_rssfeed does not create an entry in rss_feed table "
		"when passed a query feed", "[cache]")
{
	TestHelpers::TempFile dbfile;
	configcontainer cfg;
	std::unique_ptr<cache> rsscache( new cache(dbfile.getPath(), &cfg) );

	auto feed = std::make_shared<rss_feed>(rsscache.get());
	feed->set_query("unread = \"yes\"");
	feed->set_rssurl("query:All unread:unread = \"yes\"");

	REQUIRE_NOTHROW(rsscache->externalize_rssfeed(feed, false));

	// Getting rid of `cache` instance to make sure it doesn't hold the
	// database anymore and did everything it wanted to
	rsscache.reset();

	// This struct is a bit of machinery that'll ensure that we let go of
	// SQLite3 handle even if the test fails
	struct sqlite_deleter {
		void operator()(sqlite3* db) {
			sqlite3_close(db);
		}
	};
	std::unique_ptr<sqlite3, sqlite_deleter> db;

	// Open the database and manuever the pointer into our unique_ptr (which
	// can't be done by direct assignment because sqlite3_open doesn't return
	// that pointer)
	sqlite3* dbptr = nullptr;
	int error = sqlite3_open(dbfile.getPath().c_str(), &dbptr);
	REQUIRE(error == SQLITE_OK);
	db.reset(dbptr);
	dbptr = nullptr;

	auto count_callback =
		[]
		(void * data, int argc, char ** argv, char ** /* azColName */)
		-> int
		{
			int* count = static_cast<int*>(data);
			if (argc>0) {
				std::istringstream is(argv[0]);
				is >> *count;
			}
			return 0;
		};

	const std::string query("SELECT count(*) FROM rss_feed");
	int count = -42;
	char* errors = nullptr;
	int rc = sqlite3_exec(
			db.get(), query.c_str(), count_callback, &count, &errors);
	REQUIRE(rc == SQLITE_OK);

	INFO("There should be no feeds in the cache");
	REQUIRE(count == 0);
}

TEST_CASE("do_vacuum doesn't throw an exception", "[cache]") {
	TestHelpers::TempFile dbfile;
	configcontainer cfg;
	std::unique_ptr<cache> rsscache( new cache(dbfile.getPath(), &cfg) );
	rss_parser parser("file://data/rss.xml", rsscache.get(), &cfg, nullptr);
	std::shared_ptr<rss_feed> feed = parser.parse();
	rsscache->externalize_rssfeed(feed, false);

	REQUIRE_NOTHROW(rsscache->do_vacuum());

	// Checking that cache can still be opened
	REQUIRE_NOTHROW(rsscache.reset( new cache(dbfile.getPath(), &cfg) ));
}

TEST_CASE("search_in_items returns items that contain given substring",
		"[cache]")
{
	configcontainer cfg;
	cache rsscache(":memory:", &cfg);
	rss_parser parser("file://data/rss.xml", &rsscache, &cfg, nullptr);
	std::shared_ptr<rss_feed> feed = parser.parse();
	REQUIRE(feed->total_item_count() == 8);
	rsscache.externalize_rssfeed(feed, false);

	using guids = std::unordered_set<std::string>;

	const guids matching_guids {
		"http://www.blogger.com/feeds/33750310/posts/full/115822000722667899",
	};
	const guids non_matching_guids {
		"http://www.blogger.com/feeds/33750310/posts/full/115720132609158191",
		"http://www.blogger.com/feeds/33750310/posts/full/115868412707787974",
		"http://www.blogger.com/feeds/33750310/posts/full/115902176438316101",
		"http://www.blogger.com/feeds/33750310/posts/full/115934092675971946",
		"http://www.blogger.com/feeds/33750310/posts/full/115954641079726548",
		"http://www.blogger.com/feeds/33750310/posts/full/116026794030557608",
		"http://www.blogger.com/feeds/33750310/posts/full/116665092831467603",
	};

	guids guids_from_feed;
	for (const auto& item : feed->items()) {
		guids_from_feed.insert(item->guid());
	}

	const guids with_botox
		= rsscache.search_in_items("Botox", guids_from_feed);
	for (const auto& guid : with_botox) {
		INFO("Checking GUID " << guid);
		REQUIRE_FALSE(matching_guids.find(guid) == matching_guids.end());
		REQUIRE(non_matching_guids.find(guid) == non_matching_guids.end());
	}
}

TEST_CASE("search_in_items returns empty set if input set is empty", "[cache]") {
	configcontainer cfg;
	cache rsscache(":memory:", &cfg);
	rss_parser parser("file://data/rss.xml", &rsscache, &cfg, nullptr);
	std::shared_ptr<rss_feed> feed = parser.parse();
	rsscache.externalize_rssfeed(feed, false);

	using guids = std::unordered_set<std::string>;
	std::unordered_set<std::string> empty;
	const guids result = rsscache.search_in_items("Botox", empty);
	REQUIRE(result.empty());
}

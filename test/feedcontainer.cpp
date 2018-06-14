#include "3rd-party/catch.hpp"

#include <memory>

#include "cache.h"
#include "configcontainer.h"
#include "feedcontainer.h"
#include "rss.h"
#include "test-helpers.h"

using namespace newsboat;

namespace {

std::vector<std::shared_ptr<rss_feed>> get_five_empty_feeds(cache* rsscache)
{
	std::vector<std::shared_ptr<rss_feed>> feeds;
	for (int i = 0; i < 5; ++i) {
		const auto feed = std::make_shared<rss_feed>(rsscache);
		feeds.push_back(feed);
	}
	return feeds;
}

} // anonymous namespace

TEST_CASE("get_feed() returns feed by its position number", "[feedcontainer]")
{
	FeedContainer feedcontainer;
	std::unique_ptr<configcontainer> cfg(new configcontainer());
	std::unique_ptr<cache> rsscache(new cache(":memory:", cfg.get()));
	const auto feeds = get_five_empty_feeds(rsscache.get());
	int i = 0;
	for (const auto& feed : feeds) {
		feed->set_title(std::to_string(i));
		feed->set_rssurl("url/" + std::to_string(i));
		i++;
	}
	feedcontainer.set_feeds(feeds);

	auto feed = feedcontainer.get_feed(0);
	REQUIRE(feed->title_raw() == "0");

	feed = feedcontainer.get_feed(4);
	REQUIRE(feed->title_raw() == "4");
}

TEST_CASE("get_all_feeds() returns copy of FeedContainer's feed vector",
	"[feedcontainer]")
{
	FeedContainer feedcontainer;
	std::unique_ptr<configcontainer> cfg(new configcontainer());
	std::unique_ptr<cache> rsscache(new cache(":memory:", cfg.get()));
	const auto feeds = get_five_empty_feeds(rsscache.get());
	feedcontainer.set_feeds(feeds);

	REQUIRE(feedcontainer.get_all_feeds() == feeds);
}

TEST_CASE("add_feed() adds specific feed to its \"feeds\" vector", "[feedcontainer]")
{
	FeedContainer feedcontainer;
	std::unique_ptr<configcontainer> cfg(new configcontainer());
	std::unique_ptr<cache> rsscache(new cache(":memory:", cfg.get()));
	feedcontainer.set_feeds({});
	const auto feed = std::make_shared<rss_feed>(rsscache.get());
	feed->set_title("Example feed");

	feedcontainer.add_feed(feed);

	REQUIRE(feedcontainer.get_feed(0)->title_raw() == "Example feed");
}

TEST_CASE("set_feeds() sets FeedContainer's feed vector to the given one",
	"[feedcontainer]")
{
	FeedContainer feedcontainer;
	std::unique_ptr<configcontainer> cfg(new configcontainer());
	std::unique_ptr<cache> rsscache(new cache(":memory:", cfg.get()));
	const auto feeds = get_five_empty_feeds(rsscache.get());

	feedcontainer.set_feeds(feeds);

	REQUIRE(feedcontainer.feeds == feeds);
}

TEST_CASE("get_feed_by_url() returns feed by its URL", "[feedcontainer]")
{
	FeedContainer feedcontainer;
	std::unique_ptr<configcontainer> cfg(new configcontainer());
	std::unique_ptr<cache> rsscache(new cache(":memory:", cfg.get()));
	const auto feeds = get_five_empty_feeds(rsscache.get());
	int i = 0;
	for (const auto& feed : feeds) {
		feed->set_title(std::to_string(i));
		feed->set_rssurl("url/" + std::to_string(i));
		i++;
	}
	feedcontainer.set_feeds(feeds);

	auto feed = feedcontainer.get_feed_by_url("url/1");
	REQUIRE(feed->title_raw() == "1");

	feed = feedcontainer.get_feed_by_url("url/4");
	REQUIRE(feed->title_raw() == "4");
}

TEST_CASE(
	"get_feed_by_url() returns nullptr when it cannot find feed with "
	"given URL",
	"[feedcontainer]")
{
	FeedContainer feedcontainer;
	std::unique_ptr<configcontainer> cfg(new configcontainer());
	std::unique_ptr<cache> rsscache(new cache(":memory:", cfg.get()));
	const auto feeds = get_five_empty_feeds(rsscache.get());
	int i = 0;
	for (const auto& feed : feeds) {
		feed->set_rssurl("url/" + std::to_string(i));
		i++;
	}
	feedcontainer.set_feeds(feeds);

	auto feed = feedcontainer.get_feed_by_url("Wrong URL");
	REQUIRE(feed == nullptr);
}

TEST_CASE("Throws on get_feed() with pos out of range", "[feedcontainer]")
{
	FeedContainer feedcontainer;
	std::unique_ptr<configcontainer> cfg(new configcontainer());
	std::unique_ptr<cache> rsscache(new cache(":memory:", cfg.get()));
	feedcontainer.set_feeds(get_five_empty_feeds(rsscache.get()));

	REQUIRE_NOTHROW(feedcontainer.get_feed(4));
	CHECK_THROWS_AS(feedcontainer.get_feed(5), std::out_of_range);
	CHECK_THROWS_AS(feedcontainer.get_feed(-1), std::out_of_range);
}

TEST_CASE("Returns correct number using get_feed_count_by_tag()",
	"[feedcontainer]")
{
	FeedContainer feedcontainer;
	std::unique_ptr<configcontainer> cfg(new configcontainer());
	std::unique_ptr<cache> rsscache(new cache(":memory:", cfg.get()));
	feedcontainer.set_feeds(get_five_empty_feeds(rsscache.get()));
	feedcontainer.get_feed(0)->set_tags({"Chicken", "Horse"});
	feedcontainer.get_feed(1)->set_tags({"Horse", "Duck"});
	feedcontainer.get_feed(2)->set_tags({"Duck", "Frog"});
	feedcontainer.get_feed(3)->set_tags({"Duck", "Hawk"});

	REQUIRE(feedcontainer.get_feed_count_per_tag("Ice Cream") == 0);
	REQUIRE(feedcontainer.get_feed_count_per_tag("Chicken") == 1);
	REQUIRE(feedcontainer.get_feed_count_per_tag("Horse") == 2);
	REQUIRE(feedcontainer.get_feed_count_per_tag("Duck") == 3);
}

TEST_CASE("Correctly returns pos of next unread item", "[feedcontainer]")
{
	FeedContainer feedcontainer;
	std::unique_ptr<configcontainer> cfg(new configcontainer());
	std::unique_ptr<cache> rsscache(new cache(":memory:", cfg.get()));
	const auto feeds = get_five_empty_feeds(rsscache.get());
	int i = 0;
	for (const auto& feed : feeds) {
		const auto item = std::make_shared<rss_item>(rsscache.get());
		if ((i % 2) == 0)
			item->set_unread_nowrite(true);
		else
			item->set_unread_nowrite(false);
		feed->add_item(item);
		i++;
	}
	feedcontainer.set_feeds(feeds);

	REQUIRE(feedcontainer.get_pos_of_next_unread(0) == 2);
	REQUIRE(feedcontainer.get_pos_of_next_unread(2) == 4);
}

TEST_CASE("feeds_size() returns FeedContainer's current feed vector size",
	"[feedcontainer]")
{
	FeedContainer feedcontainer;
	std::unique_ptr<configcontainer> cfg(new configcontainer());
	std::unique_ptr<cache> rsscache(new cache(":memory:", cfg.get()));
	const auto feeds = get_five_empty_feeds(rsscache.get());
	feedcontainer.set_feeds(feeds);

	REQUIRE(feedcontainer.feeds_size() == feeds.size());
}

TEST_CASE("Correctly sorts feeds", "[feedcontainer]")
{
	FeedContainer feedcontainer;
	std::unique_ptr<configcontainer> cfg(new configcontainer());
	std::unique_ptr<cache> rsscache(new cache(":memory:", cfg.get()));
	const auto feeds = get_five_empty_feeds(rsscache.get());
	feedcontainer.set_feeds(feeds);

	feeds[0]->set_order(4);
	feeds[1]->set_order(5);
	feeds[2]->set_order(1);
	feeds[3]->set_order(33);
	feeds[4]->set_order(10);

	SECTION("by none asc")
	{
		cfg->set_configvalue("feed-sort-order", "none-asc");
		feedcontainer.sort_feeds(cfg.get());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0]->get_order() == 33);
		REQUIRE(sorted_feeds[1]->get_order() == 10);
		REQUIRE(sorted_feeds[2]->get_order() == 5);
		REQUIRE(sorted_feeds[3]->get_order() == 4);
		REQUIRE(sorted_feeds[4]->get_order() == 1);
	}

	SECTION("by none desc")
	{
		cfg->set_configvalue("feed-sort-order", "none-desc");
		feedcontainer.sort_feeds(cfg.get());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0]->get_order() == 1);
		REQUIRE(sorted_feeds[1]->get_order() == 4);
		REQUIRE(sorted_feeds[2]->get_order() == 5);
		REQUIRE(sorted_feeds[3]->get_order() == 10);
		REQUIRE(sorted_feeds[4]->get_order() == 33);
	}

	feeds[0]->set_tags({"aa"});
	feeds[1]->set_tags({"Taggy"});
	feeds[2]->set_tags({"Zaza"});
	feeds[3]->set_tags({"taggy"});
	feeds[4]->set_tags({"aaa"});

	SECTION("by firsttag asc")
	{
		cfg->set_configvalue("feed-sort-order", "firsttag-asc");
		feedcontainer.sort_feeds(cfg.get());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0]->get_firsttag() == "Zaza");
		REQUIRE(sorted_feeds[1]->get_firsttag() == "taggy");
		REQUIRE(sorted_feeds[2]->get_firsttag() == "Taggy");
		REQUIRE(sorted_feeds[3]->get_firsttag() == "aaa");
		REQUIRE(sorted_feeds[4]->get_firsttag() == "aa");
	}

	SECTION("by firsttag desc")
	{
		cfg->set_configvalue("feed-sort-order", "firsttag-desc");
		feedcontainer.sort_feeds(cfg.get());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0]->get_firsttag() == "aa");
		REQUIRE(sorted_feeds[1]->get_firsttag() == "aaa");
		REQUIRE(sorted_feeds[2]->get_firsttag() == "Taggy");
		REQUIRE(sorted_feeds[3]->get_firsttag() == "taggy");
		REQUIRE(sorted_feeds[4]->get_firsttag() == "Zaza");
	}

	feeds[0]->set_title({"aa"});
	feeds[1]->set_title({"Taggy"});
	feeds[2]->set_title({"Zaza"});
	feeds[3]->set_title({"taggy"});
	feeds[4]->set_title({"aaa"});

	SECTION("by title asc")
	{
		cfg->set_configvalue("feed-sort-order", "title-asc");
		feedcontainer.sort_feeds(cfg.get());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0]->title() == "Zaza");
		REQUIRE(sorted_feeds[1]->title() == "taggy");
		REQUIRE(sorted_feeds[2]->title() == "Taggy");
		REQUIRE(sorted_feeds[3]->title() == "aaa");
		REQUIRE(sorted_feeds[4]->title() == "aa");
	}

	SECTION("by title desc")
	{
		cfg->set_configvalue("feed-sort-order", "title-desc");
		feedcontainer.sort_feeds(cfg.get());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0]->title() == "aa");
		REQUIRE(sorted_feeds[1]->title() == "aaa");
		REQUIRE(sorted_feeds[2]->title() == "Taggy");
		REQUIRE(sorted_feeds[3]->title() == "taggy");
		REQUIRE(sorted_feeds[4]->title() == "Zaza");
	}

	feeds[0]->add_item(std::make_shared<rss_item>(rsscache.get()));
	feeds[0]->add_item(std::make_shared<rss_item>(rsscache.get()));
	feeds[1]->add_item(std::make_shared<rss_item>(rsscache.get()));
	feeds[1]->add_item(std::make_shared<rss_item>(rsscache.get()));
	feeds[1]->add_item(std::make_shared<rss_item>(rsscache.get()));
	feeds[2]->add_item(std::make_shared<rss_item>(rsscache.get()));
	feeds[3]->add_item(std::make_shared<rss_item>(rsscache.get()));

	SECTION("by articlecount asc")
	{
		cfg->set_configvalue("feed-sort-order", "articlecount-asc");
		feedcontainer.sort_feeds(cfg.get());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0]->total_item_count() == 3);
		REQUIRE(sorted_feeds[1]->total_item_count() == 2);
		REQUIRE(sorted_feeds[2]->total_item_count() == 1);
		REQUIRE(sorted_feeds[3]->total_item_count() == 1);
		REQUIRE(sorted_feeds[4]->total_item_count() == 0);
	}

	SECTION("by articlecount desc")
	{
		cfg->set_configvalue("feed-sort-order", "articlecount-desc");
		feedcontainer.sort_feeds(cfg.get());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0]->total_item_count() == 0);
		REQUIRE(sorted_feeds[1]->total_item_count() == 1);
		REQUIRE(sorted_feeds[2]->total_item_count() == 1);
		REQUIRE(sorted_feeds[3]->total_item_count() == 2);
		REQUIRE(sorted_feeds[4]->total_item_count() == 3);
	}

	auto item = std::make_shared<rss_item>(rsscache.get());
	item->set_unread_nowrite(false);
	feeds[1]->add_item(item);
	item = std::make_shared<rss_item>(rsscache.get());
	feeds[2]->add_item(item);
	item = std::make_shared<rss_item>(rsscache.get());
	item->set_unread_nowrite(false);
	feeds[4]->add_item(item);

	SECTION("by unreadarticlecount asc")
	{
		cfg->set_configvalue("feed-sort-order", "unreadarticlecount-asc");
		feedcontainer.sort_feeds(cfg.get());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0]->unread_item_count() == 3);
		REQUIRE(sorted_feeds[1]->unread_item_count() == 2);
		REQUIRE(sorted_feeds[2]->unread_item_count() == 2);
		REQUIRE(sorted_feeds[3]->unread_item_count() == 1);
		REQUIRE(sorted_feeds[4]->unread_item_count() == 0);
	}

	SECTION("by unreadarticlecount desc")
	{
		cfg->set_configvalue("feed-sort-order", "unreadarticlecount-desc");
		feedcontainer.sort_feeds(cfg.get());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0]->unread_item_count() == 0);
		REQUIRE(sorted_feeds[1]->unread_item_count() == 1);
		REQUIRE(sorted_feeds[2]->unread_item_count() == 2);
		REQUIRE(sorted_feeds[3]->unread_item_count() == 2);
		REQUIRE(sorted_feeds[4]->unread_item_count() == 3);
	}

	SECTION("by lastupdated asc")
	{
	}

	SECTION("by lastupdated desc")
	{
	}
}

TEST_CASE("mark_all_feed_items_read() marks all of feed's items as read",
	"[feedcontainer]")
{
	FeedContainer feedcontainer;
	std::unique_ptr<configcontainer> cfg(new configcontainer());
	std::unique_ptr<cache> rsscache(new cache(":memory:", cfg.get()));
	const auto feeds = get_five_empty_feeds(rsscache.get());
	const auto feed = feeds.at(0);
	for (int j = 0; j < 5; ++j) {
		const auto item = std::make_shared<rss_item>(rsscache.get());
		item->set_unread_nowrite(true);
		feed->add_item(item);
	}
	feedcontainer.set_feeds(feeds);

	feedcontainer.mark_all_feed_items_read(0);

	for (const auto& item : feed->items()) {
		REQUIRE(item->unread() == false);
	}
}

TEST_CASE(
	"reset_feeds_status() changes status of all feeds to \"to be "
	"downloaded\"",
	"[feedcontainer]")
{
	FeedContainer feedcontainer;
	std::unique_ptr<configcontainer> cfg(new configcontainer());
	std::unique_ptr<cache> rsscache(new cache(":memory:", cfg.get()));
	const auto feeds = get_five_empty_feeds(rsscache.get());
	feeds[0]->set_status(dl_status::SUCCESS);
	feeds[1]->set_status(dl_status::TO_BE_DOWNLOADED);
	feeds[2]->set_status(dl_status::DURING_DOWNLOAD);
	feeds[3]->set_status(dl_status::DL_ERROR);
	feedcontainer.set_feeds(feeds);

	feedcontainer.reset_feeds_status();

	for (const auto& feed : feeds) {
		REQUIRE(feed->get_status() == "_");
	}
}

TEST_CASE("clear_feeds_items() clears all of feed's items", "[feedcontainer]")
{
	FeedContainer feedcontainer;
	std::unique_ptr<configcontainer> cfg(new configcontainer());
	std::unique_ptr<cache> rsscache(new cache(":memory:", cfg.get()));
	feedcontainer.set_feeds({});
	const auto feed = std::make_shared<rss_feed>(rsscache.get());
	for (int j = 0; j < 5; ++j) {
		feed->add_item(std::make_shared<rss_item>(rsscache.get()));
	}
	feedcontainer.add_feed(feed);

	REQUIRE(feed->items().size() == 5);
	feedcontainer.clear_feeds_items();
	REQUIRE(feed->items().size() == 0);
}

TEST_CASE(
	"unread_feed_count() returns number of feeds that have unread items in "
	"them",
	"[feedcontainer]")
{
	FeedContainer feedcontainer;
	std::unique_ptr<configcontainer> cfg(new configcontainer());
	std::unique_ptr<cache> rsscache(new cache(":memory:", cfg.get()));
	const auto feeds = get_five_empty_feeds(rsscache.get());
	for (int j = 0; j < 5; ++j) {
		// Make sure that number of unread items in feed doesn't matter
		const auto item = std::make_shared<rss_item>(rsscache.get());
		const auto item2 = std::make_shared<rss_item>(rsscache.get());
		if ((j % 2) == 0) {
			item->set_unread_nowrite(false);
			item2->set_unread_nowrite(false);
		}
		feeds[j]->add_item(item);
		feeds[j]->add_item(item2);
	}
	feedcontainer.set_feeds(feeds);

	REQUIRE(feedcontainer.unread_feed_count() == 2);
}

TEST_CASE("unread_item_count() returns number of unread items in all feeds",
	"[feedcontainer]")
{
	FeedContainer feedcontainer;
	std::unique_ptr<configcontainer> cfg(new configcontainer());
	std::unique_ptr<cache> rsscache(new cache(":memory:", cfg.get()));
	const auto feeds = get_five_empty_feeds(rsscache.get());
	for (int j = 0; j < 5; ++j) {
		const auto item = std::make_shared<rss_item>(rsscache.get());
		const auto item2 = std::make_shared<rss_item>(rsscache.get());
		if ((j % 2) == 0) {
			item->set_unread_nowrite(false);
			item2->set_unread_nowrite(false);
		}
		feeds[j]->add_item(item);
		feeds[j]->add_item(item2);
	}
	feedcontainer.set_feeds(feeds);

	REQUIRE(feedcontainer.unread_item_count() == 4);
}

#include "3rd-party/catch.hpp"

#include <memory>

#include "cache.h"
#include "configcontainer.h"
#include "feedhandler.h"
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

TEST_CASE("get_feed() returns feed by its position number", "[feedhandler]")
{
	FeedHandler feedhandler;
	std::unique_ptr<configcontainer> cfg(new configcontainer());
	std::unique_ptr<cache> rsscache(new cache(":memory:", cfg.get()));
	const auto feeds = get_five_empty_feeds(rsscache.get());
	int i = 0;
	for (const auto& feed : feeds) {
		feed->set_title(std::to_string(i));
		feed->set_rssurl("url/" + std::to_string(i));
		i++;
	}
	feedhandler.set_feeds(feeds);

	auto feed = feedhandler.get_feed(0);
	REQUIRE(feed->title_raw() == "0");

	feed = feedhandler.get_feed(4);
	REQUIRE(feed->title_raw() == "4");
}

TEST_CASE("get_feed_by_url() returns feed by its URL", "[feedhandler]")
{
	FeedHandler feedhandler;
	std::unique_ptr<configcontainer> cfg(new configcontainer());
	std::unique_ptr<cache> rsscache(new cache(":memory:", cfg.get()));
	const auto feeds = get_five_empty_feeds(rsscache.get());
	int i = 0;
	for (const auto& feed : feeds) {
		feed->set_title(std::to_string(i));
		feed->set_rssurl("url/" + std::to_string(i));
		i++;
	}
	feedhandler.set_feeds(feeds);

	auto feed = feedhandler.get_feed_by_url("url/1");
	REQUIRE(feed->title_raw() == "1");

	feed = feedhandler.get_feed_by_url("url/4");
	REQUIRE(feed->title_raw() == "4");
}

TEST_CASE("Throws on get_feed() with pos out of range", "[feedhandler]")
{
	FeedHandler feedhandler;
	std::unique_ptr<configcontainer> cfg(new configcontainer());
	std::unique_ptr<cache> rsscache(new cache(":memory:", cfg.get()));
	feedhandler.set_feeds(get_five_empty_feeds(rsscache.get()));

	REQUIRE_NOTHROW(feedhandler.get_feed(4));
	CHECK_THROWS_AS(feedhandler.get_feed(5), std::out_of_range);
	CHECK_THROWS_AS(feedhandler.get_feed(-1), std::out_of_range);
}

TEST_CASE("Returns correct number using get_feed_count_by_tag()",
	"[feedhandler]")
{
	FeedHandler feedhandler;
	std::unique_ptr<configcontainer> cfg(new configcontainer());
	std::unique_ptr<cache> rsscache(new cache(":memory:", cfg.get()));
	feedhandler.set_feeds(get_five_empty_feeds(rsscache.get()));
	feedhandler.get_feed(0)->set_tags({"Chicken", "Horse"});
	feedhandler.get_feed(1)->set_tags({"Horse", "Duck"});
	feedhandler.get_feed(2)->set_tags({"Duck", "Frog"});
	feedhandler.get_feed(3)->set_tags({"Duck", "Hawk"});

	REQUIRE(feedhandler.get_feed_count_per_tag("Ice Cream") == 0);
	REQUIRE(feedhandler.get_feed_count_per_tag("Chicken") == 1);
	REQUIRE(feedhandler.get_feed_count_per_tag("Horse") == 2);
	REQUIRE(feedhandler.get_feed_count_per_tag("Duck") == 3);
}

TEST_CASE("Correctly returns pos of next unread item", "[feedhandler]")
{
	FeedHandler feedhandler;
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
	feedhandler.set_feeds(feeds);

	REQUIRE(feedhandler.get_pos_of_next_unread(0) == 2);
	REQUIRE(feedhandler.get_pos_of_next_unread(2) == 4);
}

TEST_CASE("Correctly sorts feeds", "[feedhandler]")
{
	SECTION("by none asc")
	{
	}

	SECTION("by none desc")
	{
	}

	SECTION("by firsttag asc")
	{
	}

	SECTION("by firsttag desc")
	{
	}

	SECTION("by title asc")
	{
	}

	SECTION("by title desc")
	{
	}

	SECTION("by articlecount asc")
	{
	}

	SECTION("by articlecount desc")
	{
	}

	SECTION("by unreadarticlecount asc")
	{
	}

	SECTION("by unreadarticlecount desc")
	{
	}

	SECTION("by lastupdated asc")
	{
	}

	SECTION("by lastupdated desc")
	{
	}
}

TEST_CASE("mark_all_feed_items_read() marks all of feed's items as read",
	"[feedhandler]")
{
	FeedHandler feedhandler;
	std::unique_ptr<configcontainer> cfg(new configcontainer());
	std::unique_ptr<cache> rsscache(new cache(":memory:", cfg.get()));
	const auto feeds = get_five_empty_feeds(rsscache.get());
	const auto feed = feeds.at(0);
	for (int j = 0; j < 5; ++j) {
		const auto item = std::make_shared<rss_item>(rsscache.get());
		item->set_unread_nowrite(true);
		feed->add_item(item);
	}
	feedhandler.set_feeds(feeds);

	feedhandler.mark_all_feed_items_read(0);

	for (const auto& item : feed->items()) {
		REQUIRE(item->unread() == false);
	}
}

TEST_CASE(
	"reset_feeds_status() changes status of all feeds to \"to be "
	"downloaded\"",
	"[feedhandler]")
{
	FeedHandler feedhandler;
	std::unique_ptr<configcontainer> cfg(new configcontainer());
	std::unique_ptr<cache> rsscache(new cache(":memory:", cfg.get()));
	const auto feeds = get_five_empty_feeds(rsscache.get());
	feeds[0]->set_status(dl_status::SUCCESS);
	feeds[1]->set_status(dl_status::TO_BE_DOWNLOADED);
	feeds[2]->set_status(dl_status::DURING_DOWNLOAD);
	feeds[3]->set_status(dl_status::DL_ERROR);
	feedhandler.set_feeds(feeds);

	feedhandler.reset_feeds_status();

	for (const auto& feed : feeds) {
		REQUIRE(feed->get_status() == "_");
	}
}

TEST_CASE("clear_feeds_items() clears all of feed's items", "[feedhandler]")
{
	FeedHandler feedhandler;
	std::unique_ptr<configcontainer> cfg(new configcontainer());
	std::unique_ptr<cache> rsscache(new cache(":memory:", cfg.get()));
	feedhandler.set_feeds({});
	const auto feed = std::make_shared<rss_feed>(rsscache.get());
	for (int j = 0; j < 5; ++j) {
		feed->add_item(std::make_shared<rss_item>(rsscache.get()));
	}
	feedhandler.add_feed(feed);

	REQUIRE(feed->items().size() == 5);
	feedhandler.clear_feeds_items();
	REQUIRE(feed->items().size() == 0);
}

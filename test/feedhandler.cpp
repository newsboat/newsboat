#include "3rd-party/catch.hpp"

#include <memory>

#include "cache.h"
#include "configcontainer.h"
#include "feedhandler.h"
#include "rss.h"
#include "test-helpers.h"

using namespace newsboat;

namespace {

std::vector<std::shared_ptr<rss_feed>> get_five_empty_feeds()
{
	std::vector<std::shared_ptr<rss_feed>> feeds;
	TestHelpers::TempFile dbfile;
	std::unique_ptr<configcontainer> cfg(new configcontainer());
	std::unique_ptr<cache> rsscache(new cache(dbfile.getPath(), cfg.get()));
	for (int i = 0; i < 5; ++i) {
		const auto feed = std::make_shared<rss_feed>(rsscache.get());
		feeds.push_back(feed);
	}
	return feeds;
}

} // anonymous namespace

TEST_CASE("Correctly returns desired feed")
{
	FeedHandler feedhandler;
	const auto feeds = get_five_empty_feeds();
	int i = 0;
	for (const auto& feed : feeds) {
		feed->set_title(std::to_string(i));
		feed->set_rssurl("url/" + std::to_string(i));
		i++;
	}
	feedhandler.set_feeds(feeds);

	SECTION("By pos")
	{
		auto feed = feedhandler.get_feed(0);
		REQUIRE(feed->title_raw() == "0");

		feed = feedhandler.get_feed(4);
		REQUIRE(feed->title_raw() == "4");
	}

	SECTION("By rssurl")
	{
		auto feed = feedhandler.get_feed_by_url("url/1");
		REQUIRE(feed->title_raw() == "1");

		feed = feedhandler.get_feed_by_url("url/4");
		REQUIRE(feed->title_raw() == "4");
	}
}

TEST_CASE("Throws on get_feed() with pos out of range")
{
	FeedHandler feedhandler;
	feedhandler.set_feeds(get_five_empty_feeds());

	REQUIRE_NOTHROW(feedhandler.get_feed(4));
	CHECK_THROWS_AS(feedhandler.get_feed(5), std::out_of_range);
	CHECK_THROWS_AS(feedhandler.get_feed(-1), std::out_of_range);
}

TEST_CASE("Returns correct number using get_feed_count_by_tag()")
{
	FeedHandler feedhandler;
	feedhandler.set_feeds(get_five_empty_feeds());
	feedhandler.get_feed(0)->set_tags({"Chicken", "Horse"});
	feedhandler.get_feed(1)->set_tags({"Horse", "Duck"});
	feedhandler.get_feed(2)->set_tags({"Duck", "Frog"});
	feedhandler.get_feed(3)->set_tags({"Duck", "Hawk"});

	REQUIRE(feedhandler.get_feed_count_per_tag("Ice Cream") == 0);
	REQUIRE(feedhandler.get_feed_count_per_tag("Chicken") == 1);
	REQUIRE(feedhandler.get_feed_count_per_tag("Horse") == 2);
	REQUIRE(feedhandler.get_feed_count_per_tag("Duck") == 3);
}

TEST_CASE("Correctly returns pos of next unread item")
{
	FeedHandler feedhandler;
	const auto feeds = get_five_empty_feeds();
	int i = 0;
	for (const auto& feed : feeds) {
		const auto item = std::make_shared<rss_item>(nullptr);
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

TEST_CASE("Correctly sorts feeds")
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

#define ENABLE_IMPLICIT_FILEPATH_CONVERSIONS

#include "3rd-party/catch.hpp"

#include <memory>
#include <string>

#include "cache.h"
#include "configcontainer.h"
#include "feedcontainer.h"
#include "rssfeed.h"

#include "3rd-party/catch.hpp"

using namespace newsboat;

namespace {

std::vector<std::shared_ptr<RssFeed>> get_five_empty_feeds(Cache* rsscache)
{
	std::vector<std::shared_ptr<RssFeed>> feeds;
	for (int i = 0; i < 5; ++i) {
		const auto feed = std::make_shared<RssFeed>(rsscache, "");
		feeds.push_back(feed);
	}
	return feeds;
}

} // anonymous namespace

TEST_CASE("get_feed() returns feed by its position number", "[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	const std::vector<std::shared_ptr<RssFeed>> feeds = {
		std::make_shared<RssFeed>(&rsscache, "url/0"),
		std::make_shared<RssFeed>(&rsscache, "url/1"),
		std::make_shared<RssFeed>(&rsscache, "url/2"),
		std::make_shared<RssFeed>(&rsscache, "url/3"),
		std::make_shared<RssFeed>(&rsscache, "url/4")
	};
	int i = 0;
	for (const auto& feed : feeds) {
		feed->set_title(std::to_string(i));
		i++;
	}
	feedcontainer.set_feeds(feeds);

	auto feed = feedcontainer.get_feed(0);
	REQUIRE(feed->title_raw() == "0");

	feed = feedcontainer.get_feed(4);
	REQUIRE(feed->title_raw() == "4");
}

TEST_CASE("get_all_feeds() returns copy of FeedContainer's feed vector",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	const auto feeds = get_five_empty_feeds(&rsscache);
	feedcontainer.set_feeds(feeds);

	REQUIRE(feedcontainer.get_all_feeds() == feeds);
}

TEST_CASE("add_feed() adds specific feed to its \"feeds\" vector",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	feedcontainer.set_feeds({});
	const auto feed = std::make_shared<RssFeed>(&rsscache, "");
	feed->set_title("Example feed");

	feedcontainer.add_feed(feed);

	REQUIRE(feedcontainer.get_feed(0)->title_raw() == "Example feed");
}

TEST_CASE("populate_query_feeds() populates query feeds", "[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	auto feeds = get_five_empty_feeds(&rsscache);
	for (int j = 0; j < 5; ++j) {
		const auto item = std::make_shared<RssItem>(&rsscache);
		item->set_unread_nowrite(true);
		feeds[j]->add_item(item);
	}

	const auto feed = std::make_shared<RssFeed>(&rsscache,
			"query:a title:unread = \"yes\"");
	feeds.push_back(feed);

	feedcontainer.set_feeds(feeds);
	feedcontainer.populate_query_feeds();

	REQUIRE(feeds[5]->total_item_count() == 5);
}

TEST_CASE("set_feeds() sets FeedContainer's feed vector to the given one",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	const auto feeds = get_five_empty_feeds(&rsscache);

	feedcontainer.set_feeds(feeds);

	REQUIRE(feedcontainer.get_all_feeds() == feeds);
}

TEST_CASE("get_feed_by_url() returns feed by its URL", "[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	const std::vector<std::shared_ptr<RssFeed>> feeds = {
		std::make_shared<RssFeed>(&rsscache, "url/0"),
		std::make_shared<RssFeed>(&rsscache, "url/1"),
		std::make_shared<RssFeed>(&rsscache, "url/2"),
		std::make_shared<RssFeed>(&rsscache, "url/3"),
		std::make_shared<RssFeed>(&rsscache, "url/4")
	};
	int i = 0;
	for (const auto& feed : feeds) {
		feed->set_title(std::to_string(i));
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
	"[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	const std::vector<std::shared_ptr<RssFeed>> feeds = {
		std::make_shared<RssFeed>(&rsscache, "url/0"),
		std::make_shared<RssFeed>(&rsscache, "url/1"),
		std::make_shared<RssFeed>(&rsscache, "url/2"),
	};
	feedcontainer.set_feeds(feeds);

	auto feed = feedcontainer.get_feed_by_url("Wrong URL");
	REQUIRE(feed == nullptr);
}

TEST_CASE("get_feed() returns nullptr if pos is out of range",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	feedcontainer.set_feeds(get_five_empty_feeds(&rsscache));

	REQUIRE_NOTHROW(feedcontainer.get_feed(4));
	REQUIRE(feedcontainer.get_feed(5) == nullptr);
	REQUIRE(feedcontainer.get_feed(-1) == nullptr);
}

TEST_CASE("Returns correct number using get_feed_count_by_tag()",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	feedcontainer.set_feeds(get_five_empty_feeds(&rsscache));
	feedcontainer.get_feed(0)->set_tags({"Chicken", "Horse"});
	feedcontainer.get_feed(1)->set_tags({"Horse", "Duck"});
	feedcontainer.get_feed(2)->set_tags({"Duck", "Frog"});
	feedcontainer.get_feed(3)->set_tags({"Duck", "Hawk"});

	REQUIRE(feedcontainer.get_feed_count_per_tag("Ice Cream") == 0);
	REQUIRE(feedcontainer.get_feed_count_per_tag("Chicken") == 1);
	REQUIRE(feedcontainer.get_feed_count_per_tag("Horse") == 2);
	REQUIRE(feedcontainer.get_feed_count_per_tag("Duck") == 3);
}

TEST_CASE("Correctly returns pos of next unread item", "[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	const auto feeds = get_five_empty_feeds(&rsscache);
	int i = 0;
	for (const auto& feed : feeds) {
		const auto item = std::make_shared<RssItem>(&rsscache);
		if ((i % 2) == 0) {
			item->set_unread_nowrite(true);
		} else {
			item->set_unread_nowrite(false);
		}
		feed->add_item(item);
		i++;
	}
	feedcontainer.set_feeds(feeds);

	REQUIRE(feedcontainer.get_pos_of_next_unread(0) == 2);
	REQUIRE(feedcontainer.get_pos_of_next_unread(2) == 4);
}

TEST_CASE("feeds_size() returns FeedContainer's current feed vector size",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	const auto feeds = get_five_empty_feeds(&rsscache);
	feedcontainer.set_feeds(feeds);

	REQUIRE(feedcontainer.feeds_size() == feeds.size());
}

TEST_CASE("sort_feeds() sorts by position in urls file if `feed-sort-order` "
	"is \"none\"",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	const auto feeds = get_five_empty_feeds(&rsscache);
	feedcontainer.set_feeds(feeds);

	feeds[0]->set_order(4);
	feeds[1]->set_order(5);
	feeds[2]->set_order(1);
	feeds[3]->set_order(33);
	feeds[4]->set_order(10);

	SECTION("`none-asc` -- first in urls file is last in the list") {
		cfg.set_configvalue("feed-sort-order", "none-asc");
		feedcontainer.sort_feeds(cfg.get_feed_sort_strategy());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0]->get_order() == 33);
		REQUIRE(sorted_feeds[1]->get_order() == 10);
		REQUIRE(sorted_feeds[2]->get_order() == 5);
		REQUIRE(sorted_feeds[3]->get_order() == 4);
		REQUIRE(sorted_feeds[4]->get_order() == 1);
	}

	SECTION("`none-desc` -- first in the urls file is first in the list") {
		cfg.set_configvalue("feed-sort-order", "none-desc");
		feedcontainer.sort_feeds(cfg.get_feed_sort_strategy());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0]->get_order() == 1);
		REQUIRE(sorted_feeds[1]->get_order() == 4);
		REQUIRE(sorted_feeds[2]->get_order() == 5);
		REQUIRE(sorted_feeds[3]->get_order() == 10);
		REQUIRE(sorted_feeds[4]->get_order() == 33);
	}
}

TEST_CASE("sort_feeds() sorts by feed's first tag if `feed-sort-order` "
	"is \"firsttag\"",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	const auto feeds = get_five_empty_feeds(&rsscache);
	feedcontainer.set_feeds(feeds);

	feeds[0]->set_tags({"tag1"});
	feeds[1]->set_tags({"Taggy"});
	feeds[2]->set_tags({"tag10"});
	feeds[3]->set_tags({"taggy"});
	feeds[4]->set_tags({"tag2"});

	SECTION("`firsttag-asc` -- tags in reverse natural order") {
		cfg.set_configvalue("feed-sort-order", "firsttag-asc");
		feedcontainer.sort_feeds(cfg.get_feed_sort_strategy());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0]->get_firsttag() == "taggy");
		REQUIRE(sorted_feeds[1]->get_firsttag() == "tag10");
		REQUIRE(sorted_feeds[2]->get_firsttag() == "tag2");
		REQUIRE(sorted_feeds[3]->get_firsttag() == "tag1");
		REQUIRE(sorted_feeds[4]->get_firsttag() == "Taggy");
	}

	SECTION("`firsttag-desc` -- tags in natural order") {
		cfg.set_configvalue("feed-sort-order", "firsttag-desc");
		feedcontainer.sort_feeds(cfg.get_feed_sort_strategy());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0]->get_firsttag() == "Taggy");
		REQUIRE(sorted_feeds[1]->get_firsttag() == "tag1");
		REQUIRE(sorted_feeds[2]->get_firsttag() == "tag2");
		REQUIRE(sorted_feeds[3]->get_firsttag() == "tag10");
		REQUIRE(sorted_feeds[4]->get_firsttag() == "taggy");
	}
}

TEST_CASE("sort_feeds() sorts by feed's title if `feed-sort-order` "
	"is \"title\"",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	const auto feeds = get_five_empty_feeds(&rsscache);
	feedcontainer.set_feeds(feeds);

	feeds[0]->set_title("tag1");
	feeds[1]->set_title("Taggy");
	feeds[2]->set_title("tag10");
	feeds[3]->set_title("taggy");
	feeds[4]->set_title("tag2");

	SECTION("`title-asc` -- titles in reverse natural order") {
		cfg.set_configvalue("feed-sort-order", "title-asc");
		feedcontainer.sort_feeds(cfg.get_feed_sort_strategy());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0]->title() == "taggy");
		REQUIRE(sorted_feeds[1]->title() == "tag10");
		REQUIRE(sorted_feeds[2]->title() == "tag2");
		REQUIRE(sorted_feeds[3]->title() == "tag1");
		REQUIRE(sorted_feeds[4]->title() == "Taggy");
	}

	SECTION("`title-desc` -- titles in natural order") {
		cfg.set_configvalue("feed-sort-order", "title-desc");
		feedcontainer.sort_feeds(cfg.get_feed_sort_strategy());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0]->title() == "Taggy");
		REQUIRE(sorted_feeds[1]->title() == "tag1");
		REQUIRE(sorted_feeds[2]->title() == "tag2");
		REQUIRE(sorted_feeds[3]->title() == "tag10");
		REQUIRE(sorted_feeds[4]->title() == "taggy");
	}
}

TEST_CASE("sort_feeds() sorts by number of articles in a feed "
	"if `feed-sort-order` is \"articlecount\"",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	const auto feeds = get_five_empty_feeds(&rsscache);
	feedcontainer.set_feeds(feeds);

	feeds[0]->add_item(std::make_shared<RssItem>(&rsscache));
	feeds[0]->add_item(std::make_shared<RssItem>(&rsscache));
	feeds[1]->add_item(std::make_shared<RssItem>(&rsscache));
	feeds[1]->add_item(std::make_shared<RssItem>(&rsscache));
	feeds[1]->add_item(std::make_shared<RssItem>(&rsscache));
	feeds[2]->add_item(std::make_shared<RssItem>(&rsscache));
	feeds[3]->add_item(std::make_shared<RssItem>(&rsscache));

	SECTION("`articlecount-asc` -- feed with most items is at the top") {
		cfg.set_configvalue("feed-sort-order", "articlecount-asc");
		feedcontainer.sort_feeds(cfg.get_feed_sort_strategy());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0]->total_item_count() == 3);
		REQUIRE(sorted_feeds[1]->total_item_count() == 2);
		REQUIRE(sorted_feeds[2]->total_item_count() == 1);
		REQUIRE(sorted_feeds[3]->total_item_count() == 1);
		REQUIRE(sorted_feeds[4]->total_item_count() == 0);
	}

	SECTION("`articlecount-desc` -- feed with most items is at the bottom") {
		cfg.set_configvalue("feed-sort-order", "articlecount-desc");
		feedcontainer.sort_feeds(cfg.get_feed_sort_strategy());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0]->total_item_count() == 0);
		REQUIRE(sorted_feeds[1]->total_item_count() == 1);
		REQUIRE(sorted_feeds[2]->total_item_count() == 1);
		REQUIRE(sorted_feeds[3]->total_item_count() == 2);
		REQUIRE(sorted_feeds[4]->total_item_count() == 3);
	}
}

TEST_CASE("sort_feeds() and keep in-group order when sorting by unread articles",
	"[FeedContainer]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);

	const std::map<std::string, int> name_to_unreads = {
		{"a", 3}, {"b", 2}, {"c", 1}, {"d", 1}, {"e", 1}
	};

	std::vector<std::shared_ptr<RssFeed>> feeds;
	for (const auto& entry : name_to_unreads) {
		const auto feed = std::make_shared<RssFeed>(&rsscache, "");
		feed->set_title(entry.first);
		for (int i = 0; i < entry.second; ++i) {
			feed->add_item(std::make_shared<RssItem>(&rsscache));
		}
		feeds.push_back(feed);
	}
	FeedContainer feedcontainer;
	feedcontainer.set_feeds(feeds);

	FeedSortStrategy strategy;
	strategy.sm = FeedSortMethod::UNREAD_ARTICLE_COUNT;
	SECTION("acsending order") {
		strategy.sd = SortDirection::ASC;
		feedcontainer.sort_feeds(strategy);
		const auto sorted_feeds = feedcontainer.get_all_feeds();

		std::vector<std::string> actual;
		for (const auto& feed : sorted_feeds) {
			auto title = feed->title();
			actual.push_back(title);
		}

		const std::vector<std::string> expected = {"a", "b", "c", "d", "e"};
		REQUIRE(expected == actual);
	}

	SECTION("descending order") {
		strategy.sd = SortDirection::DESC;
		feedcontainer.sort_feeds(strategy);
		const auto sorted_feeds = feedcontainer.get_all_feeds();

		std::vector<std::string> actual;
		for (const auto& feed : sorted_feeds) {
			auto title = feed->title();
			actual.push_back(title);
		}

		const std::vector<std::string> expected = {"c", "d", "e", "b", "a"};
		REQUIRE(expected == actual);
	}
}

TEST_CASE("sort_feeds() and keep in-group order when sorting by order", "[FeedContainer]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);

	const std::map<std::string, int> name_to_order = {
		{"a", 3}, {"b", 2}, {"c", 1}, {"d", 1}, {"e", 1}
	};

	std::vector<std::shared_ptr<RssFeed>> feeds;
	for (const auto& entry : name_to_order) {
		const auto feed = std::make_shared<RssFeed>(&rsscache, "");
		feed->set_title(entry.first);
		feed->set_order(entry.second);
		feeds.push_back(feed);
	}
	FeedContainer feedcontainer;
	feedcontainer.set_feeds(feeds);

	FeedSortStrategy strategy;
	strategy.sm = FeedSortMethod::NONE;
	SECTION("descending order") {
		strategy.sd = SortDirection::DESC;
		feedcontainer.sort_feeds(strategy);
		const auto sorted_feeds = feedcontainer.get_all_feeds();

		std::vector<std::string> actual;
		for (const auto& feed : sorted_feeds) {
			auto title = feed->title();
			actual.push_back(title);
		}

		const std::vector<std::string> expected = {"c", "d", "e", "b", "a"};
		REQUIRE(expected == actual);
	}

	SECTION("acsending order") {
		strategy.sd = SortDirection::ASC;
		feedcontainer.sort_feeds(strategy);
		const auto sorted_feeds = feedcontainer.get_all_feeds();

		std::vector<std::string> actual;
		for (const auto& feed : sorted_feeds) {
			auto title = feed->title();
			actual.push_back(title);
		}

		const std::vector<std::string> expected = {"a", "b", "c", "d", "e"};
		REQUIRE(expected == actual);
	}
}

TEST_CASE("sort_feeds() and keep in-group order when sorting by articles",
	"[FeedContainer]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);

	const std::map<std::string, int> name_to_articles = {
		{"a", 3}, {"b", 2}, {"c", 1}, {"d", 1}, {"e", 1}
	};

	std::vector<std::shared_ptr<RssFeed>> feeds;
	for (const auto& entry : name_to_articles) {
		const auto feed = std::make_shared<RssFeed>(&rsscache, "");
		feed->set_title(entry.first);
		for (int i=0; i<entry.second; ++i) {
			feed->add_item(std::make_shared<RssItem>(&rsscache));
		}
		feeds.push_back(feed);
	}
	FeedContainer feedcontainer;
	feedcontainer.set_feeds(feeds);

	FeedSortStrategy strategy;
	strategy.sm = FeedSortMethod::ARTICLE_COUNT;
	SECTION("descending order") {
		strategy.sd = SortDirection::DESC;
		feedcontainer.sort_feeds(strategy);
		const auto sorted_feeds = feedcontainer.get_all_feeds();

		std::vector<std::string> actual;
		for (const auto& feed : sorted_feeds) {
			auto title = feed->title();
			actual.push_back(title);
		}

		const std::vector<std::string> expected = {"c", "d", "e", "b", "a"};
		REQUIRE(expected == actual);
	}

	SECTION("acsending order") {
		strategy.sd = SortDirection::ASC;
		feedcontainer.sort_feeds(strategy);
		const auto sorted_feeds = feedcontainer.get_all_feeds();

		std::vector<std::string> actual;
		for (const auto& feed : sorted_feeds) {
			auto title = feed->title();
			actual.push_back(title);
		}

		const std::vector<std::string> expected = {"a", "b", "c", "d", "e"};
		REQUIRE(expected == actual);
	}
}

TEST_CASE("sort_feeds() and keep in-group order when sorting by last updated item",
	"[FeedContainer]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);

	const std::map<std::string, int> name_to_date = {
		{"a", 3}, {"b", 2}, {"c", 1}, {"d", 1}, {"e", 1}
	};

	std::vector<std::shared_ptr<RssFeed>> feeds;
	for (const auto& entry : name_to_date) {
		const auto feed = std::make_shared<RssFeed>(&rsscache, "");
		feed->set_title(entry.first);
		auto item = std::make_shared<RssItem>(&rsscache);
		item->set_pubDate(entry.second);
		feed->add_item(item);
		feeds.push_back(feed);
	}
	FeedContainer feedcontainer;
	feedcontainer.set_feeds(feeds);

	FeedSortStrategy strategy;
	strategy.sm = FeedSortMethod::LAST_UPDATED;
	SECTION("descending order") {
		strategy.sd = SortDirection::DESC;
		feedcontainer.sort_feeds(strategy);
		const auto sorted_feeds = feedcontainer.get_all_feeds();

		std::vector<std::string> actual;
		for (const auto& feed : sorted_feeds) {
			auto title = feed->title();
			actual.push_back(title);
		}

		const std::vector<std::string> expected = {"a", "b", "c", "d", "e"};
		REQUIRE(expected == actual);
	}

	SECTION("acsending order") {
		strategy.sd = SortDirection::ASC;
		feedcontainer.sort_feeds(strategy);
		const auto sorted_feeds = feedcontainer.get_all_feeds();

		std::vector<std::string> actual;
		for (const auto& feed : sorted_feeds) {
			auto title = feed->title();
			actual.push_back(title);
		}

		const std::vector<std::string> expected = {"c", "d", "e", "b", "a"} ;
		REQUIRE(expected == actual);
	}
}

TEST_CASE("sort_feeds() and keep in-group order when sorting by title",
	"[FeedContainer]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);

	const std::map<std::string, std::string> url_to_title = {
		{"1", "c"}, {"2", "b"}, {"3", "a"}, {"4", "a"}, {"5", "a"}
	};

	std::vector<std::shared_ptr<RssFeed>> feeds;
	for (const auto& entry : url_to_title) {
		const auto feed = std::make_shared<RssFeed>(&rsscache, entry.first);
		feed->set_title(entry.second);
		feeds.push_back(feed);
	}
	FeedContainer feedcontainer;
	feedcontainer.set_feeds(feeds);

	FeedSortStrategy strategy;
	strategy.sm = FeedSortMethod::TITLE;
	SECTION("descending order") {
		strategy.sd = SortDirection::DESC;
		feedcontainer.sort_feeds(strategy);
		const auto sorted_feeds = feedcontainer.get_all_feeds();

		std::vector<std::string> actual;
		for (const auto& feed : sorted_feeds) {
			auto url = feed->rssurl();
			actual.push_back(url);
		}

		const std::vector<std::string> expected = {"3", "4", "5", "2", "1"};
		REQUIRE(expected == actual);
	}

	SECTION("acsending order") {
		strategy.sd = SortDirection::ASC;
		feedcontainer.sort_feeds(strategy);
		const auto sorted_feeds = feedcontainer.get_all_feeds();

		std::vector<std::string> actual;
		for (const auto& feed : sorted_feeds) {
			auto url = feed->rssurl();
			actual.push_back(url);
		}

		const std::vector<std::string> expected = {"1", "2", "3", "4", "5"} ;
		REQUIRE(expected == actual);
	}
}

TEST_CASE("sort_feeds() sorts by number of unread articles if `feed-sort-order` "
	"is \"unreadarticlecount\"",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	const auto feeds = get_five_empty_feeds(&rsscache);
	feedcontainer.set_feeds(feeds);

	feeds[0]->add_item(std::make_shared<RssItem>(&rsscache));
	feeds[0]->add_item(std::make_shared<RssItem>(&rsscache));

	auto item = std::make_shared<RssItem>(&rsscache);
	item->set_unread_nowrite(false);
	feeds[1]->add_item(item);
	feeds[1]->add_item(std::make_shared<RssItem>(&rsscache));
	feeds[1]->add_item(std::make_shared<RssItem>(&rsscache));
	feeds[1]->add_item(std::make_shared<RssItem>(&rsscache));

	feeds[2]->add_item(std::make_shared<RssItem>(&rsscache));
	feeds[2]->add_item(std::make_shared<RssItem>(&rsscache));

	feeds[3]->add_item(std::make_shared<RssItem>(&rsscache));

	item = std::make_shared<RssItem>(&rsscache);
	item->set_unread_nowrite(false);
	feeds[4]->add_item(item);

	SECTION("`unreadarticlecount-asc` -- feed with most unread items is at the top") {
		cfg.set_configvalue(
			"feed-sort-order", "unreadarticlecount-asc");
		feedcontainer.sort_feeds(cfg.get_feed_sort_strategy());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0]->unread_item_count() == 3);
		REQUIRE(sorted_feeds[1]->unread_item_count() == 2);
		REQUIRE(sorted_feeds[2]->unread_item_count() == 2);
		REQUIRE(sorted_feeds[3]->unread_item_count() == 1);
		REQUIRE(sorted_feeds[4]->unread_item_count() == 0);
	}

	SECTION("`unreadarticlecount-desc` -- feed with most unread items is at the bottom") {
		cfg.set_configvalue(
			"feed-sort-order", "unreadarticlecount-desc");
		feedcontainer.sort_feeds(cfg.get_feed_sort_strategy());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0]->unread_item_count() == 0);
		REQUIRE(sorted_feeds[1]->unread_item_count() == 1);
		REQUIRE(sorted_feeds[2]->unread_item_count() == 2);
		REQUIRE(sorted_feeds[3]->unread_item_count() == 2);
		REQUIRE(sorted_feeds[4]->unread_item_count() == 3);
	}
}

TEST_CASE("sort_feeds() sorts by publish date of newest item "
	"if `feed-sort-order` is \"lastupdated\"",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	const auto feeds = get_five_empty_feeds(&rsscache);
	feedcontainer.set_feeds(feeds);

	auto item = std::make_shared<RssItem>(&rsscache);
	item->set_pubDate(93);
	feeds[0]->add_item(item);

	item = std::make_shared<RssItem>(&rsscache);
	item->set_pubDate(42);
	feeds[1]->add_item(item);

	item = std::make_shared<RssItem>(&rsscache);
	item->set_pubDate(69);
	feeds[2]->add_item(item);

	item = std::make_shared<RssItem>(&rsscache);
	item->set_pubDate(23);
	feeds[3]->add_item(item);

	item = std::make_shared<RssItem>(&rsscache);
	item->set_pubDate(1);
	feeds[4]->add_item(item);

	SECTION("`lastupdated-asc` -- most recently updated feed is at the bottom") {
		cfg.set_configvalue("feed-sort-order", "lastupdated-asc");
		feedcontainer.sort_feeds(cfg.get_feed_sort_strategy());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0] == feeds[4]);
		REQUIRE(sorted_feeds[1] == feeds[3]);
		REQUIRE(sorted_feeds[2] == feeds[1]);
		REQUIRE(sorted_feeds[3] == feeds[2]);
		REQUIRE(sorted_feeds[4] == feeds[0]);
	}

	SECTION("`lastupdated-desc` -- most recently updated feed is at the top") {
		cfg.set_configvalue("feed-sort-order", "lastupdated-desc");
		feedcontainer.sort_feeds(cfg.get_feed_sort_strategy());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0] == feeds[0]);
		REQUIRE(sorted_feeds[1] == feeds[2]);
		REQUIRE(sorted_feeds[2] == feeds[1]);
		REQUIRE(sorted_feeds[3] == feeds[3]);
		REQUIRE(sorted_feeds[4] == feeds[4]);
	}
}

TEST_CASE("Sorting by firsttag-asc puts empty tags on top", "[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	const auto feeds = get_five_empty_feeds(&rsscache);
	feedcontainer.set_feeds(feeds);

	feeds[0]->set_tags({""});
	feeds[1]->set_tags({"Taggy"});
	feeds[2]->set_tags({"tag10"});
	feeds[3]->set_tags({"taggy"});
	feeds[4]->set_tags({"tag2"});

	SECTION("by firsttag asc, empty tag is first") {
		cfg.set_configvalue("feed-sort-order", "firsttag-asc");
		feedcontainer.sort_feeds(cfg.get_feed_sort_strategy());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0]->get_firsttag() == "");
		REQUIRE(sorted_feeds[1]->get_firsttag() == "taggy");
		REQUIRE(sorted_feeds[2]->get_firsttag() == "tag10");
		REQUIRE(sorted_feeds[3]->get_firsttag() == "tag2");
		REQUIRE(sorted_feeds[4]->get_firsttag() == "Taggy");
	}

	SECTION("by firsttag desc, empty tag is last") {
		cfg.set_configvalue("feed-sort-order", "firsttag-desc");
		feedcontainer.sort_feeds(cfg.get_feed_sort_strategy());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0]->get_firsttag() == "Taggy");
		REQUIRE(sorted_feeds[1]->get_firsttag() == "tag2");
		REQUIRE(sorted_feeds[2]->get_firsttag() == "tag10");
		REQUIRE(sorted_feeds[3]->get_firsttag() == "taggy");
		REQUIRE(sorted_feeds[4]->get_firsttag() == "");
	}
}

TEST_CASE("Sorting by lastupdated-asc puts empty feeds on top",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	const auto feeds = get_five_empty_feeds(&rsscache);
	feedcontainer.set_feeds(feeds);

	auto item = std::make_shared<RssItem>(&rsscache);
	item->set_pubDate(93);
	feeds[0]->add_item(item);

	item = std::make_shared<RssItem>(&rsscache);
	item->set_pubDate(42);
	feeds[1]->add_item(item);

	item = std::make_shared<RssItem>(&rsscache);
	item->set_pubDate(69);
	feeds[2]->add_item(item);

	item = std::make_shared<RssItem>(&rsscache);
	item->set_pubDate(23);
	feeds[3]->add_item(item);

	// feeds[4] is empty

	SECTION("by lastupdated asc") {
		cfg.set_configvalue("feed-sort-order", "lastupdated-asc");
		feedcontainer.sort_feeds(cfg.get_feed_sort_strategy());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0] == feeds[4]);
		REQUIRE(sorted_feeds[1] == feeds[3]);
		REQUIRE(sorted_feeds[2] == feeds[1]);
		REQUIRE(sorted_feeds[3] == feeds[2]);
		REQUIRE(sorted_feeds[4] == feeds[0]);
	}

	SECTION("by lastupdated desc") {
		cfg.set_configvalue("feed-sort-order", "lastupdated-desc");
		feedcontainer.sort_feeds(cfg.get_feed_sort_strategy());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0] == feeds[0]);
		REQUIRE(sorted_feeds[1] == feeds[2]);
		REQUIRE(sorted_feeds[2] == feeds[1]);
		REQUIRE(sorted_feeds[3] == feeds[3]);
		REQUIRE(sorted_feeds[4] == feeds[4]);
	}
}

TEST_CASE("Sorting by firsttag-asc reverses the order of feeds with the same first tag",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	const auto feeds = get_five_empty_feeds(&rsscache);
	feedcontainer.set_feeds(feeds);

	feeds[0]->set_tags({""});
	feeds[1]->set_tags({"Taggy"});
	feeds[2]->set_tags({"~tag10"});
	feeds[3]->set_tags({"~AAA", "Taggy"});
	feeds[4]->set_tags({"tag2"});

	SECTION("by firsttag asc, feeds with the same first tag are sorted in reverse") {
		cfg.set_configvalue("feed-sort-order", "firsttag-asc");
		feedcontainer.sort_feeds(cfg.get_feed_sort_strategy());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0] == feeds[2]);
		REQUIRE(sorted_feeds[1] == feeds[0]);
		REQUIRE(sorted_feeds[2] == feeds[4]);
		REQUIRE(sorted_feeds[3] == feeds[3]);
		REQUIRE(sorted_feeds[4] == feeds[1]);
	}

	SECTION("by firsttag desc, feeds with the same first tag are sorted in order") {
		cfg.set_configvalue("feed-sort-order", "firsttag-desc");
		feedcontainer.sort_feeds(cfg.get_feed_sort_strategy());
		const auto sorted_feeds = feedcontainer.get_all_feeds();
		REQUIRE(sorted_feeds[0] == feeds[1]);
		REQUIRE(sorted_feeds[1] == feeds[3]);
		REQUIRE(sorted_feeds[2] == feeds[4]);
		REQUIRE(sorted_feeds[3] == feeds[0]);
		REQUIRE(sorted_feeds[4] == feeds[2]);
	}
}

TEST_CASE("mark_all_feed_items_read() marks all of feed's items as read",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	const auto feeds = get_five_empty_feeds(&rsscache);
	const auto feed = feeds.at(0);
	for (int j = 0; j < 5; ++j) {
		const auto item = std::make_shared<RssItem>(&rsscache);
		item->set_unread_nowrite(true);
		feed->add_item(item);
	}
	feedcontainer.set_feeds(feeds);

	feedcontainer.mark_all_feed_items_read(feed);

	for (const auto& item : feed->items()) {
		REQUIRE_FALSE(item->unread());
	}
}

TEST_CASE("mark_all_feeds_read() marks all items in all feeds as read",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	const auto feeds = get_five_empty_feeds(&rsscache);

	for (const auto& feed : feeds) {
		for (int j = 0; j < 3; ++j) {
			const auto item = std::make_shared<RssItem>(&rsscache);
			item->set_unread_nowrite(true);
			feed->add_item(item);
		}
	}
	feedcontainer.set_feeds(feeds);

	feedcontainer.mark_all_feeds_read();

	for (const auto& feed : feeds) {
		for (const auto& item : feed->items()) {
			REQUIRE(item->unread() == false);
		}
	}
}

TEST_CASE(
	"reset_feeds_status() changes status of all feeds to \"to be "
	"downloaded\"",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	const auto feeds = get_five_empty_feeds(&rsscache);
	feeds[0]->set_status(DlStatus::SUCCESS);
	feeds[1]->set_status(DlStatus::TO_BE_DOWNLOADED);
	feeds[2]->set_status(DlStatus::DURING_DOWNLOAD);
	feeds[3]->set_status(DlStatus::DL_ERROR);
	feedcontainer.set_feeds(feeds);

	feedcontainer.reset_feeds_status();

	for (const auto& feed : feeds) {
		REQUIRE(feed->get_status() == "_");
	}
}

TEST_CASE(
	"unread_feed_count() returns number of feeds that have unread items in "
	"them",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	const auto feeds = get_five_empty_feeds(&rsscache);
	for (int j = 0; j < 5; ++j) {
		// Make sure that number of unread items in feed doesn't matter
		const auto item = std::make_shared<RssItem>(&rsscache);
		const auto item2 = std::make_shared<RssItem>(&rsscache);
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

TEST_CASE("unread_item_count() returns number of distinct unread items "
	"in non-hidden feeds",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);

	SECTION("No query feeds") {
		const auto feeds = get_five_empty_feeds(&rsscache);
		for (int j = 0; j < 5; ++j) {
			const auto item = std::make_shared<RssItem>(&rsscache);
			item->set_guid(std::to_string(j) + "item1");
			const auto item2 = std::make_shared<RssItem>(&rsscache);
			item2->set_guid(std::to_string(j) + "item2");
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

	SECTION("Query feeds do not affect the total count") {
		// This is a regression test for https://github.com/newsboat/newsboat/issues/1120

		auto feeds = get_five_empty_feeds(&rsscache);

		auto query1 = std::make_shared<RssFeed>(&rsscache,
				"query:Title contains word:title # \"word\"");
		auto query2 = std::make_shared<RssFeed>(&rsscache,
				"query:Posts by John Doe:author = \"John Doe\"");

		feeds.push_back(query1);
		feeds.push_back(query2);

		for (int j = 0; j < 5; ++j) {
			const auto item = std::make_shared<RssItem>(&rsscache);
			item->set_title("All titles contain word");
			item->set_author("John Doe");
			item->set_guid(std::to_string(j) + "item1");
			const auto item2 = std::make_shared<RssItem>(&rsscache);
			item2->set_title("All titles contain word");
			item2->set_author("John Doe");
			item2->set_guid(std::to_string(j) + "item2");
			if ((j % 2) == 0) {
				item->set_unread_nowrite(false);
				item2->set_unread_nowrite(false);
			}
			feeds[j]->add_item(item);
			feeds[j]->add_item(item2);
		}

		feedcontainer.set_feeds(feeds);
		feedcontainer.populate_query_feeds();

		REQUIRE(feedcontainer.unread_item_count() == 4);
	}

	SECTION("Items in hidden feeds are not counted") {
		// This is a regression test for https://github.com/newsboat/newsboat/issues/444

		auto feeds = get_five_empty_feeds(&rsscache);

		// First feed is hidden and contains five unread items
		feeds[0]->set_tags({"!hidden"});
		for (int i = 0; i < 5; ++i) {
			const auto item = std::make_shared<RssItem>(&rsscache);
			item->set_guid(std::to_string(i) + "feed1");
			item->set_unread(true);
			feeds[0]->add_item(item);
		}

		// Second feed is **not** hidden and contains two unread items
		for (int i = 0; i < 2; ++i) {
			const auto item = std::make_shared<RssItem>(&rsscache);
			item->set_guid(std::to_string(i) + "feed2");
			item->set_unread(true);
			feeds[1]->add_item(item);
		}

		// Third feed contains three read items -- they shouldn't affect the count
		for (int i = 0; i < 3; ++i) {
			const auto item = std::make_shared<RssItem>(&rsscache);
			item->set_guid(std::to_string(i) + "feed3");
			item->set_unread(false);
			feeds[2]->add_item(item);
		}

		feedcontainer.set_feeds(feeds);

		// Only the two items of the second feed are counted
		REQUIRE(feedcontainer.unread_item_count() == 2);

		SECTION("...unless they are also in some query feed(s)") {
			const auto query_feed = std::make_shared<RssFeed>(&rsscache,
					"query:Posts from the hidden feed:guid =~ \".*feed1\"");
			feeds.push_back(query_feed);
			feedcontainer.set_feeds(feeds);
			feedcontainer.populate_query_feeds();

			// Two items from the second feeds plus five items from the first (hidden) feed
			REQUIRE(feedcontainer.unread_item_count() == 7);
		}
	}
}

TEST_CASE("get_unread_feed_count_per_tag returns 0 if there are no feeds "
	"with given tag",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;

	SECTION("Empty FeedContainer") {
		REQUIRE(feedcontainer.get_unread_feed_count_per_tag("unknown") == 0);
		REQUIRE(feedcontainer.get_unread_feed_count_per_tag("test") == 0);
		REQUIRE(feedcontainer.get_unread_feed_count_per_tag("with space") == 0);
	}

	SECTION("Non-empty FeedContainer, but no feeds are tagged") {
		ConfigContainer cfg;
		Cache rsscache(":memory:", &cfg);

		feedcontainer.add_feed(std::make_shared<RssFeed>(&rsscache, ""));
		feedcontainer.add_feed(std::make_shared<RssFeed>(&rsscache, ""));
		feedcontainer.add_feed(std::make_shared<RssFeed>(&rsscache, ""));

		REQUIRE(feedcontainer.get_unread_feed_count_per_tag("unknown") == 0);
		REQUIRE(feedcontainer.get_unread_feed_count_per_tag("test") == 0);
		REQUIRE(feedcontainer.get_unread_feed_count_per_tag("with space") == 0);
	}

	SECTION("Non-empty FeedContainer, no feeds are tagged with our desired tag") {
		ConfigContainer cfg;
		Cache rsscache(":memory:", &cfg);

		auto feed = std::make_shared<RssFeed>(&rsscache, "");
		feed->set_tags({"one", "two", "three"});
		feedcontainer.add_feed(feed);

		feed = std::make_shared<RssFeed>(&rsscache, "");
		feed->set_tags({"some", "different", "tags"});
		feedcontainer.add_feed(feed);

		feed = std::make_shared<RssFeed>(&rsscache, "");
		feed->set_tags({"here's one with spaces"});
		feedcontainer.add_feed(feed);

		REQUIRE(feedcontainer.get_unread_feed_count_per_tag("unknown") == 0);
		REQUIRE(feedcontainer.get_unread_feed_count_per_tag("test") == 0);
		REQUIRE(feedcontainer.get_unread_feed_count_per_tag("with space") == 0);
	}
}

TEST_CASE("get_unread_feed_count_per_tag returns 0 if feeds with given tag "
	"contain no unread items",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;

	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);

	const auto desired_tag = std::string("target");
	const auto different_tag = std::string("something else entirely");

	auto feed = std::make_shared<RssFeed>(&rsscache, "");
	feed->set_tags({desired_tag});
	for (int i = 0; i < 10; ++i) {
		auto item = std::make_shared<RssItem>(&rsscache);
		item->set_unread(false);
		feed->add_item(item);
	}
	feedcontainer.add_feed(feed);

	feed = std::make_shared<RssFeed>(&rsscache, "");
	feed->set_tags({different_tag});
	for (int i = 0; i < 5; ++i) {
		auto item = std::make_shared<RssItem>(&rsscache);
		item->set_unread(true);
		feed->add_item(item);
	}
	for (int i = 0; i < 5; ++i) {
		auto item = std::make_shared<RssItem>(&rsscache);
		item->set_unread(false);
		feed->add_item(item);
	}
	feedcontainer.add_feed(feed);

	REQUIRE(feedcontainer.get_unread_feed_count_per_tag(desired_tag) == 0);
}

TEST_CASE("get_unread_feed_count_per_tag returns the number of feeds that have "
	"a given tag and also have unread items",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;

	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);

	const auto desired_tag = std::string("target");
	const auto different_tag = std::string("something else entirely");

	const auto add_feed_with_tag_and_unreads =
		[&feedcontainer, &rsscache]
	(std::string tag, unsigned int unreads) {
		auto feed = std::make_shared<RssFeed>(&rsscache, "");

		feed->set_tags({tag});

		for (unsigned int i = 0; i < unreads; ++i) {
			auto item = std::make_shared<RssItem>(&rsscache);
			item->set_unread(true);
			feed->add_item(item);
		}
		for (int i = 0; i < 10; ++i) {
			auto item = std::make_shared<RssItem>(&rsscache);
			item->set_unread(false);
			feed->add_item(item);
		}

		feedcontainer.add_feed(std::move(feed));
	};

	SECTION("One feed") {
		add_feed_with_tag_and_unreads(different_tag, 1);
		add_feed_with_tag_and_unreads(desired_tag, 1);
		REQUIRE(feedcontainer.get_unread_feed_count_per_tag(desired_tag)
			== 1);
	}

	SECTION("Two feeds") {
		add_feed_with_tag_and_unreads(different_tag, 4);

		add_feed_with_tag_and_unreads(desired_tag, 5);
		add_feed_with_tag_and_unreads(desired_tag, 8);

		REQUIRE(feedcontainer.get_unread_feed_count_per_tag(desired_tag)
			== 2);
	}

	SECTION("Three feeds") {
		add_feed_with_tag_and_unreads(different_tag, 4);

		add_feed_with_tag_and_unreads(desired_tag, 5);
		add_feed_with_tag_and_unreads(desired_tag, 8);
		add_feed_with_tag_and_unreads(desired_tag, 11);

		REQUIRE(feedcontainer.get_unread_feed_count_per_tag(desired_tag)
			== 3);
	}
}

TEST_CASE("get_unread_item_count_per_tag returns 0 if there are no feeds "
	"with given tag",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;

	SECTION("Empty FeedContainer") {
		REQUIRE(feedcontainer.get_unread_item_count_per_tag("unknown") == 0);
		REQUIRE(feedcontainer.get_unread_item_count_per_tag("test") == 0);
		REQUIRE(feedcontainer.get_unread_item_count_per_tag("with space") == 0);
	}

	SECTION("Non-empty FeedContainer, but no feeds are tagged") {
		ConfigContainer cfg;
		Cache rsscache(":memory:", &cfg);

		feedcontainer.add_feed(std::make_shared<RssFeed>(&rsscache, ""));
		feedcontainer.add_feed(std::make_shared<RssFeed>(&rsscache, ""));
		feedcontainer.add_feed(std::make_shared<RssFeed>(&rsscache, ""));

		REQUIRE(feedcontainer.get_unread_item_count_per_tag("unknown") == 0);
		REQUIRE(feedcontainer.get_unread_item_count_per_tag("test") == 0);
		REQUIRE(feedcontainer.get_unread_item_count_per_tag("with space") == 0);
	}

	SECTION("Non-empty FeedContainer, no feeds are tagged with our desired tag") {
		ConfigContainer cfg;
		Cache rsscache(":memory:", &cfg);

		auto feed = std::make_shared<RssFeed>(&rsscache, "");
		feed->set_tags({"one", "two", "three"});
		feedcontainer.add_feed(feed);

		feed = std::make_shared<RssFeed>(&rsscache, "");
		feed->set_tags({"some", "different", "tags"});
		feedcontainer.add_feed(feed);

		feed = std::make_shared<RssFeed>(&rsscache, "");
		feed->set_tags({"here's one with spaces"});
		feedcontainer.add_feed(feed);

		REQUIRE(feedcontainer.get_unread_item_count_per_tag("unknown") == 0);
		REQUIRE(feedcontainer.get_unread_item_count_per_tag("test") == 0);
		REQUIRE(feedcontainer.get_unread_item_count_per_tag("with space") == 0);
	}
}

TEST_CASE("get_unread_item_count_per_tag returns 0 if feeds with given tag "
	"contain no unread items",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;

	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);

	const auto desired_tag = std::string("target");
	const auto different_tag = std::string("something else entirely");

	auto feed = std::make_shared<RssFeed>(&rsscache, "");
	feed->set_tags({desired_tag});
	for (int i = 0; i < 10; ++i) {
		auto item = std::make_shared<RssItem>(&rsscache);
		item->set_unread(false);
		feed->add_item(item);
	}
	feedcontainer.add_feed(feed);

	feed = std::make_shared<RssFeed>(&rsscache, "");
	feed->set_tags({different_tag});
	for (int i = 0; i < 5; ++i) {
		auto item = std::make_shared<RssItem>(&rsscache);
		item->set_unread(true);
		feed->add_item(item);
	}
	for (int i = 0; i < 5; ++i) {
		auto item = std::make_shared<RssItem>(&rsscache);
		item->set_unread(false);
		feed->add_item(item);
	}
	feedcontainer.add_feed(feed);

	REQUIRE(feedcontainer.get_unread_item_count_per_tag(desired_tag) == 0);
}

TEST_CASE("get_unread_item_count_per_tag returns the number of unread items "
	"in feeds that have a given tag",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;

	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);

	const auto desired_tag = std::string("target");
	const auto different_tag = std::string("something else entirely");

	const auto add_feed_with_tag_and_unreads =
		[&feedcontainer, &rsscache]
	(std::string tag, unsigned int unreads) {
		auto feed = std::make_shared<RssFeed>(&rsscache, "");

		feed->set_tags({tag});

		for (unsigned int i = 0; i < unreads; ++i) {
			auto item = std::make_shared<RssItem>(&rsscache);
			item->set_unread(true);
			feed->add_item(item);
		}
		for (int i = 0; i < 10; ++i) {
			auto item = std::make_shared<RssItem>(&rsscache);
			item->set_unread(false);
			feed->add_item(item);
		}

		feedcontainer.add_feed(std::move(feed));
	};

	SECTION("One feed, one unread") {
		add_feed_with_tag_and_unreads(different_tag, 1);
		add_feed_with_tag_and_unreads(desired_tag, 1);
		REQUIRE(feedcontainer.get_unread_item_count_per_tag(desired_tag)
			== 1);
	}

	SECTION("Two feeds, 13 unreads") {
		add_feed_with_tag_and_unreads(different_tag, 4);

		add_feed_with_tag_and_unreads(desired_tag, 5);
		add_feed_with_tag_and_unreads(desired_tag, 8);

		REQUIRE(feedcontainer.get_unread_item_count_per_tag(desired_tag)
			== 13);
	}

	SECTION("Three feeds, 24 unreads") {
		add_feed_with_tag_and_unreads(different_tag, 4);

		add_feed_with_tag_and_unreads(desired_tag, 5);
		add_feed_with_tag_and_unreads(desired_tag, 8);
		add_feed_with_tag_and_unreads(desired_tag, 11);

		REQUIRE(feedcontainer.get_unread_item_count_per_tag(desired_tag)
			== 24);
	}
}

TEST_CASE("replace_feed() puts given feed into the specified position",
	"[FeedContainer]")
{
	FeedContainer feedcontainer;

	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	const auto feeds = get_five_empty_feeds(&rsscache);

	const auto first_feed = *feeds.begin();
	const auto four_feeds = std::vector<std::shared_ptr<RssFeed>>(feeds.begin() + 1,
			feeds.end());

	feedcontainer.set_feeds(four_feeds);

	const auto position = 2;
	const auto feed_before_replacement = feedcontainer.get_feed(position);
	REQUIRE(feed_before_replacement != first_feed);

	feedcontainer.replace_feed(position, first_feed);
	const auto feed_after_replacement = feedcontainer.get_feed(position);

	REQUIRE(feed_before_replacement != feed_after_replacement);
	REQUIRE(feed_after_replacement == first_feed);
}

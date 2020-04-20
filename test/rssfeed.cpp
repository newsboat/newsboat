#include "rssfeed.h"

#include "3rd-party/catch.hpp"
#include "cache.h"
#include "configcontainer.h"
#include "rssparser.h"

using namespace newsboat;

TEST_CASE("RssFeed::set_rssurl() checks if query feed has a valid query",
	"[RssFeed]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	RssFeed f(&rsscache);

	SECTION("invalid query results in exception") {
		REQUIRE_THROWS(f.set_rssurl("query:a title:unread ="));
		REQUIRE_THROWS(f.set_rssurl("query:a title:between 1:3"));
	}

	SECTION("valid query doesn't throw an exception") {
		REQUIRE_NOTHROW(f.set_rssurl("query:a title:unread = \"yes\""));
		REQUIRE_NOTHROW(f.set_rssurl(
				"query:Title:unread = \"yes\" and age between 0:7"));
	}
}

TEST_CASE("RssFeed::sort() correctly sorts articles", "[RssFeed]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	RssFeed f(&rsscache);
	for (int i = 0; i < 5; ++i) {
		const auto item = std::make_shared<RssItem>(&rsscache);
		item->set_guid(std::to_string(i));
		f.add_item(item);
	}

	SECTION("title") {
		auto articles = f.items();
		articles[0]->set_title("Read me");
		articles[1]->set_title("Wow tests are great");
		articles[2]->set_title("Article 1: A boring article");
		articles[3]->set_title("Article 10: Another great article");
		articles[4]->set_title("Article 2: Article you must read");

		ArticleSortStrategy ss;
		ss.sm = ArtSortMethod::TITLE;
		ss.sd = SortDirection::ASC;
		f.sort(ss);
		articles = f.items();
		REQUIRE(articles[0]->title() == "Article 1: A boring article");
		REQUIRE(articles[1]->title() == "Article 2: Article you must read");
		REQUIRE(articles[2]->title() == "Article 10: Another great article");
		REQUIRE(articles[3]->title() == "Read me");
		REQUIRE(articles[4]->title() == "Wow tests are great");

		ss.sd = SortDirection::DESC;
		f.sort(ss);
		articles = f.items();
		REQUIRE(articles[0]->title() == "Wow tests are great");
		REQUIRE(articles[1]->title() == "Read me");
		REQUIRE(articles[2]->title() == "Article 10: Another great article");
		REQUIRE(articles[3]->title() == "Article 2: Article you must read");
		REQUIRE(articles[4]->title() == "Article 1: A boring article");
	}

	SECTION("flags") {
		auto articles = f.items();
		articles[0]->set_flags("Aabde");
		articles[1]->set_flags("Zadel");
		articles[2]->set_flags("Ksuy");
		articles[3]->set_flags("Efgpu");
		articles[4]->set_flags("Ceimu");

		ArticleSortStrategy ss;
		ss.sm = ArtSortMethod::FLAGS;
		ss.sd = SortDirection::ASC;
		f.sort(ss);
		articles = f.items();
		REQUIRE(articles[0]->flags() == "Aabde");
		REQUIRE(articles[1]->flags() == "Ceimu");
		REQUIRE(articles[2]->flags() == "Efgpu");
		REQUIRE(articles[3]->flags() == "Ksuy");
		REQUIRE(articles[4]->flags() == "Zadel");

		ss.sd = SortDirection::DESC;
		f.sort(ss);
		articles = f.items();
		REQUIRE(articles[0]->flags() == "Zadel");
		REQUIRE(articles[1]->flags() == "Ksuy");
		REQUIRE(articles[2]->flags() == "Efgpu");
		REQUIRE(articles[3]->flags() == "Ceimu");
		REQUIRE(articles[4]->flags() == "Aabde");
	}

	SECTION("author") {
		auto articles = f.items();
		articles[0]->set_author("Anonymous");
		articles[1]->set_author("Socrates");
		articles[2]->set_author("Platon");
		articles[3]->set_author("Spinoza");
		articles[4]->set_author("Sartre");

		ArticleSortStrategy ss;
		ss.sm = ArtSortMethod::AUTHOR;
		ss.sd = SortDirection::ASC;
		f.sort(ss);
		articles = f.items();
		REQUIRE(articles[0]->author() == "Anonymous");
		REQUIRE(articles[1]->author() == "Platon");
		REQUIRE(articles[2]->author() == "Sartre");
		REQUIRE(articles[3]->author() == "Socrates");
		REQUIRE(articles[4]->author() == "Spinoza");

		ss.sd = SortDirection::DESC;
		f.sort(ss);
		articles = f.items();
		REQUIRE(articles[0]->author() == "Spinoza");
		REQUIRE(articles[1]->author() == "Socrates");
		REQUIRE(articles[2]->author() == "Sartre");
		REQUIRE(articles[3]->author() == "Platon");
		REQUIRE(articles[4]->author() == "Anonymous");
	}

	SECTION("link") {
		auto articles = f.items();
		articles[0]->set_link("www.example.com");
		articles[1]->set_link("www.anotherexample.org");
		articles[2]->set_link("www.example.org");
		articles[3]->set_link("www.test.org");
		articles[4]->set_link("withoutwww.org");

		ArticleSortStrategy ss;
		ss.sm = ArtSortMethod::LINK;
		ss.sd = SortDirection::ASC;
		f.sort(ss);
		articles = f.items();
		REQUIRE(articles[0]->link() == "withoutwww.org");
		REQUIRE(articles[1]->link() == "www.anotherexample.org");
		REQUIRE(articles[2]->link() == "www.example.com");
		REQUIRE(articles[3]->link() == "www.example.org");
		REQUIRE(articles[4]->link() == "www.test.org");

		ss.sd = SortDirection::DESC;
		f.sort(ss);
		articles = f.items();
		REQUIRE(articles[0]->link() == "www.test.org");
		REQUIRE(articles[1]->link() == "www.example.org");
		REQUIRE(articles[2]->link() == "www.example.com");
		REQUIRE(articles[3]->link() == "www.anotherexample.org");
		REQUIRE(articles[4]->link() == "withoutwww.org");
	}

	SECTION("guid") {
		ArticleSortStrategy ss;
		ss.sm = ArtSortMethod::GUID;
		ss.sd = SortDirection::ASC;
		f.sort(ss);
		auto articles = f.items();
		REQUIRE(articles[0]->guid() == "0");
		REQUIRE(articles[1]->guid() == "1");
		REQUIRE(articles[2]->guid() == "2");
		REQUIRE(articles[3]->guid() == "3");
		REQUIRE(articles[4]->guid() == "4");

		ss.sd = SortDirection::DESC;
		f.sort(ss);
		articles = f.items();
		REQUIRE(articles[0]->guid() == "4");
		REQUIRE(articles[1]->guid() == "3");
		REQUIRE(articles[2]->guid() == "2");
		REQUIRE(articles[3]->guid() == "1");
		REQUIRE(articles[4]->guid() == "0");
	}

	SECTION("date") {
		auto articles = f.items();
		articles[0]->set_pubDate(93);
		articles[1]->set_pubDate(42);
		articles[2]->set_pubDate(69);
		articles[3]->set_pubDate(23);
		articles[4]->set_pubDate(7);

		ArticleSortStrategy ss;
		ss.sm = ArtSortMethod::DATE;
		ss.sd = SortDirection::DESC;
		f.sort(ss);
		articles = f.items();
		REQUIRE(articles[0]->pubDate_timestamp() == 7);
		REQUIRE(articles[1]->pubDate_timestamp() == 23);
		REQUIRE(articles[2]->pubDate_timestamp() == 42);
		REQUIRE(articles[3]->pubDate_timestamp() == 69);
		REQUIRE(articles[4]->pubDate_timestamp() == 93);

		ss.sd = SortDirection::ASC;
		f.sort(ss);
		articles = f.items();
		REQUIRE(articles[0]->pubDate_timestamp() == 93);
		REQUIRE(articles[1]->pubDate_timestamp() == 69);
		REQUIRE(articles[2]->pubDate_timestamp() == 42);
		REQUIRE(articles[3]->pubDate_timestamp() == 23);
		REQUIRE(articles[4]->pubDate_timestamp() == 7);
	}
}

TEST_CASE("RssFeed::unread_item_count() returns number of unread articles",
	"[RssFeed]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	RssFeed f(&rsscache);
	for (int i = 0; i < 5; ++i) {
		const auto item = std::make_shared<RssItem>(&rsscache);
		item->set_guid(std::to_string(i));
		f.add_item(item);
	}

	REQUIRE(f.unread_item_count() == 5);

	f.get_item_by_guid("0")->set_unread_nowrite(false);
	REQUIRE(f.unread_item_count() == 4);

	f.get_item_by_guid("1")->set_unread_nowrite(false);
	REQUIRE(f.unread_item_count() == 3);

	f.get_item_by_guid("2")->set_unread_nowrite(false);
	REQUIRE(f.unread_item_count() == 2);

	f.get_item_by_guid("3")->set_unread_nowrite(false);
	REQUIRE(f.unread_item_count() == 1);

	f.get_item_by_guid("4")->set_unread_nowrite(false);
	REQUIRE(f.unread_item_count() == 0);
}

TEST_CASE("RssFeed::matches_tag() returns true if article has a specified tag",
	"[RssFeed]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	RssFeed f(&rsscache);
	const std::vector<std::string> tags = {"One", "Two", "Three", "Four"};
	f.set_tags(tags);

	REQUIRE_FALSE(f.matches_tag("Zero"));
	REQUIRE(f.matches_tag("One"));
	REQUIRE(f.matches_tag("Two"));
	REQUIRE(f.matches_tag("Three"));
	REQUIRE(f.matches_tag("Four"));
	REQUIRE_FALSE(f.matches_tag("Five"));
}

TEST_CASE("RssFeed::get_firsttag() returns first tag", "[RssFeed]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	RssFeed f(&rsscache);

	REQUIRE(f.get_firsttag() == "");

	std::vector<std::string> tags = {"One", "Two", "Three", "Four"};
	f.set_tags(tags);
	REQUIRE(f.get_firsttag() == "One");

	tags = {"Five", "Six", "Seven", "Eight"};
	f.set_tags(tags);
	REQUIRE(f.get_firsttag() == "Five");

	tags = {"Nine", "Ten", "Eleven", "Twelve"};
	f.set_tags(tags);
	REQUIRE(f.get_firsttag() == "Nine");

	tags = {"Orange", "Apple", "Kiwi", "Banana"};
	f.set_tags(tags);
	REQUIRE(f.get_firsttag() == "Orange");
}

TEST_CASE(
	"RssFeed::hidden() returns true if feed has a tag starting with \"!\"",
	"[RssFeed]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	RssFeed f(&rsscache);

	REQUIRE_FALSE(f.hidden());

	std::vector<std::string> tags = {"One", "Two", "!Three"};
	f.set_tags(tags);
	REQUIRE(f.hidden());

	tags = {"One"};
	f.set_tags(tags);
	REQUIRE_FALSE(f.hidden());

	tags = {"One", "!"};
	f.set_tags(tags);
	REQUIRE(f.hidden());
}

TEST_CASE(
	"RssFeed::mark_all_items_read() marks all items within a feed as read",
	"[RssFeed]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	RssFeed f(&rsscache);
	for (int i = 0; i < 5; ++i) {
		const auto item = std::make_shared<RssItem>(&rsscache);
		REQUIRE(item->unread());
		f.add_item(item);
	}

	f.mark_all_items_read();

	for (const auto& item : f.items()) {
		REQUIRE_FALSE(item->unread());
	}
}

TEST_CASE("RssFeed::set_tags() sets tags for a feed", "[RssFeed]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	RssFeed f(&rsscache);

	std::vector<std::string> tags = {"One", "Two"};
	f.set_tags(tags);
	tags = {"One", "Three"};
	REQUIRE(f.get_tags() == "One Two ");
	f.set_tags(tags);
	REQUIRE(f.get_tags() == "One Three ");
}

TEST_CASE(
	"RssFeed::purge_deleted_items() deletes all items that have "
	"\"deleted\" property set up",
	"[RssFeed]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	RssFeed f(&rsscache);
	for (int i = 0; i < 5; ++i) {
		const auto item = std::make_shared<RssItem>(&rsscache);
		if (i % 2) {
			item->set_deleted(true);
		}
		f.add_item(item);
	}

	f.purge_deleted_items();
	REQUIRE(f.total_item_count() == 3);
}

TEST_CASE("If item's <title> is empty, try to deduce it from the URL",
	"[RssFeed]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	RssParser p("file://data/items_without_titles.xml",
		&rsscache,
		&cfg,
		nullptr,
		nullptr);
	auto feed = p.parse();

	REQUIRE(feed->items()[0]->title() ==
		"A gentle introduction to testing");
	REQUIRE(feed->items()[1]->title() == "A missing rel attribute");
	REQUIRE(feed->items()[2]->title() == "Alternate link isnt first");
	REQUIRE(feed->items()[3]->title() == "A test for htm extension");
	REQUIRE(feed->items()[4]->title() == "Alternate link isn't first");
}

TEST_CASE(
	"RssFeed::is_query_feed() return true if feed is a query feed, i.e. "
	"its \"rssurl\" starts with \"query:\" string",
	"[RssFeed]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	RssFeed f(&rsscache);

	f.set_rssurl("query... no wait");
	REQUIRE_FALSE(f.is_query_feed());
	f.set_rssurl("  query:");
	REQUIRE_FALSE(f.is_query_feed());

	f.set_rssurl("query:a title:unread = \"yes\"");
	REQUIRE(f.is_query_feed());
	f.set_rssurl("query:Title:unread = \"yes\" and age between 0:7");
	REQUIRE(f.is_query_feed());
}

#include "rssfeed.h"

#include "3rd-party/catch.hpp"
#include "cache.h"
#include "configcontainer.h"
#include "curlhandle.h"
#include "feedretriever.h"
#include "rssparser.h"

#include "test_helpers/envvar.h"
#include "test_helpers/stringmaker/optional.h"

using namespace Newsboat;

TEST_CASE("RssFeed constructor checks if query feed has a valid query",
	"[RssFeed]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);

	SECTION("invalid query results in exception") {
		REQUIRE_THROWS(RssFeed(&rsscache, "query:a title:unread ="));
		REQUIRE_THROWS(RssFeed(&rsscache, "query:a title:between 1:3"));
	}

	SECTION("valid query doesn't throw an exception") {
		REQUIRE_NOTHROW(RssFeed(&rsscache, "query:a title:unread = \"yes\""));
		REQUIRE_NOTHROW(RssFeed(&rsscache,
				"query:Title:unread = \"yes\" and age between 0:7"));

		REQUIRE_NOTHROW(RssFeed(&rsscache, R"_(query:a title:title =~ "media:")_"));
	}
}

TEST_CASE("RssFeed::sort() correctly sorts articles", "[RssFeed]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	RssFeed f(&rsscache, "");
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
	RssFeed f(&rsscache, "");
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
	RssFeed f(&rsscache, "");
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
	RssFeed f(&rsscache, "");

	SECTION("Empty tag array") {
		REQUIRE(f.get_firsttag() == "");
	}

	SECTION("Ordinary tags") {
		f.set_tags({"One", "Two", "Three", "Four"});
		REQUIRE(f.get_firsttag() == "One");
	}

	SECTION("Title tag exclusion") {
		f.set_tags({"~Five", "Six", "Seven", "Eight"});
		REQUIRE(f.get_firsttag() == "Six");
	}

	SECTION("Non exclusion of tag prefixed by a special character other than tilde") {
		f.set_tags({"~Nine", "!Ten", "Eleven", "Twelve"});
		REQUIRE(f.get_firsttag() == "!Ten");
	}

	SECTION("Array with only title tags") {
		f.set_tags({"~Orange", "~Apple", "~Kiwi", "~Banana"});
		REQUIRE(f.get_firsttag() == "");
	}
}

TEST_CASE(
	"RssFeed::hidden() returns true if feed has a tag starting with \"!\"",
	"[RssFeed]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	RssFeed f(&rsscache, "");

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
	RssFeed f(&rsscache, "");
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
	RssFeed f(&rsscache, "");

	std::vector<std::string> tags = {"One", "Two"};
	f.set_tags(tags);
	REQUIRE(f.get_tags() == tags);
	tags = {"One", "Three"};
	f.set_tags(tags);
	REQUIRE(f.get_tags() == tags);
}

TEST_CASE(
	"RssFeed::purge_deleted_items() deletes all items that have "
	"\"deleted\" property set up",
	"[RssFeed]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	RssFeed f(&rsscache, "");
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
	CurlHandle easyHandle;
	FeedRetriever feed_retriever(cfg, rsscache, easyHandle);
	const std::string uri = "file://data/items_without_titles.xml";
	RssParser p(uri,
		rsscache,
		cfg,
		nullptr);
	auto feed = p.parse(feed_retriever.retrieve(uri));

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

	const auto check_if_query_feed = [&](const std::string& rssurl) {
		RssFeed f(&rsscache, rssurl);
		return f.is_query_feed();
	};

	REQUIRE_FALSE(check_if_query_feed("query... no wait"));
	REQUIRE_FALSE(check_if_query_feed("  query:"));

	REQUIRE(check_if_query_feed("query:a title:unread = \"yes\""));
	REQUIRE(check_if_query_feed("query:Title:unread = \"yes\" and age between 0:7"));
}

TEST_CASE("RssFeed contains a number of matchable attributes", "[RssFeed]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	RssFeed f(&rsscache, "");

	SECTION("`feedtitle`, containing feed's title") {
		const auto title = std::string("Example feed");
		f.set_title(title);

		const auto attr = "feedtitle";
		REQUIRE(f.attribute_value(attr) == title);

		SECTION("it is encoded to the locale's charset") {
			// Due to differences in how platforms handle //TRANSLIT in iconv,
			// we can't compare results to a known-good value. Instead, we
			// merely check that the result is *not* UTF-8.

			test_helpers::LcCtypeEnvVar lc_ctype;
			lc_ctype.set("C"); // This means ASCII

			const auto title = "こんにちは";// "good afternoon" in Japanese
			f.set_title(title);

			REQUIRE_FALSE(f.attribute_value(attr) == title);
		}
	}

	SECTION("description") {
		const auto desc = std::string("A temporary empty feed, used for testing.");
		f.set_description(desc);

		const auto attr = "description";
		REQUIRE(f.attribute_value(attr) == desc);

		SECTION("it is encoded to the locale's charset") {
			// Due to differences in how platforms handle //TRANSLIT in iconv,
			// we can't compare results to a known-good value. Instead, we
			// merely check that the result is *not* UTF-8.

			test_helpers::LcCtypeEnvVar lc_ctype;
			lc_ctype.set("C"); // This means ASCII

			const auto description = "こんにちは";// "good afternoon" in Japanese
			f.set_description(description);

			REQUIRE_FALSE(f.attribute_value(attr) == description);
		}
	}

	SECTION("feedlink, feed's notion of its own location") {
		const auto feedlink = std::string("https://example.com/feed.xml");
		f.set_link(feedlink);

		const auto attr = "feedlink";
		REQUIRE(f.attribute_value(attr) == feedlink);
	}

	SECTION("feeddate, feed's publication date") {
		test_helpers::TzEnvVar tzEnv;
		tzEnv.set("UTC");

		f.set_pubDate(1); // one second into the Unix epoch

		const auto attr = "feeddate";
		REQUIRE(f.attribute_value(attr) == "Thu, 01 Jan 1970 00:00:01 +0000");
	}

	SECTION("rssurl, the URL by which this feed is fetched (specified in the urls file)") {
		const auto url = std::string("https://example.com/news.atom");
		RssFeed f2(&rsscache, url);

		const auto attr = "rssurl";
		REQUIRE(f2.attribute_value(attr) == url);
	}

	SECTION("unread_count, the number of items in the feed that aren't read yet") {
		const auto attr = "unread_count";

		SECTION("empty feed => unread_count == 0") {
			REQUIRE(f.attribute_value(attr) == "0");
		}

		SECTION("feed with two items") {
			auto item1 = std::make_shared<RssItem>(&rsscache);
			auto item2 = std::make_shared<RssItem>(&rsscache);

			f.add_item(item1);
			f.add_item(item2);

			SECTION("all unread => unread_count == 2") {
				item1->set_unread(true);
				item2->set_unread(true);

				REQUIRE(f.attribute_value(attr) == "2");
			}

			SECTION("one unread => unread_count == 1") {
				item1->set_unread(false);

				REQUIRE(f.attribute_value(attr) == "1");
			}

			SECTION("all read => unread_count == 0") {
				item1->set_unread(false);
				item2->set_unread(false);

				REQUIRE(f.attribute_value(attr) == "0");
			}
		}
	}

	SECTION("total_count, the number of items in the feed") {
		const auto attr = "total_count";

		SECTION("empty feed => total_count == 0") {
			REQUIRE(f.attribute_value(attr) == "0");
		}

		SECTION("feed with two items => total_count == 2") {
			auto item1 = std::make_shared<RssItem>(&rsscache);
			auto item2 = std::make_shared<RssItem>(&rsscache);

			f.add_item(item1);
			f.add_item(item2);

			REQUIRE(f.attribute_value(attr) == "2");
		}
	}

	SECTION("tags") {
		const auto attr = "tags";

		SECTION("no tags => attribute empty") {
			REQUIRE(f.attribute_value(attr) == "");
		}

		SECTION("multiple tags => a space-separated list of tags, ending with space") {
			f.set_tags({"first", "second", "third", "tags"});

			REQUIRE(f.attribute_value(attr) == "first second third tags ");
		}

		SECTION("spaces inside tags are not escaped") {
			f.set_tags({"first", "another with spaces", "final"});

			REQUIRE(f.attribute_value(attr) == "first another with spaces final ");
		}
	}

	SECTION("feedindex, feed's position in the feedlist") {
		const auto attr = "feedindex";

		const auto check = [&attr, &f](unsigned int index) {
			f.set_index(index);

			REQUIRE(f.attribute_value(attr) == std::to_string(index));
		};

		check(1);
		check(100);
		check(65536);
		check(100500);
	}

	SECTION("latest_article_age, the number of days since the most recent articles publish date") {
		const auto attr = "latest_article_age";
		const auto current_time = ::time(nullptr);
		const auto seconds_per_day = 24 * 60 * 60;

		SECTION("empty feed => latest_article_age == 0") {
			REQUIRE(f.attribute_value(attr) == "0");
		}

		SECTION("feed with two items => latest_article_age == <days since most recent publish date>") {
			auto item1 = std::make_shared<RssItem>(&rsscache);
			auto item2 = std::make_shared<RssItem>(&rsscache);

			item1->set_pubDate(current_time - 3 * seconds_per_day);
			item2->set_pubDate(current_time - 5 * seconds_per_day);

			f.add_item(item1);
			f.add_item(item2);

			REQUIRE(f.attribute_value(attr) == "3");
		}
	}
}

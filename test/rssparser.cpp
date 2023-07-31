#include "rssparser.h"

#include "3rd-party/catch.hpp"

#include "cache.h"
#include "rss/feed.h"
#include "rss/item.h"
#include "rssfeed.h"
#include "rssignores.h"

using namespace newsboat;

TEST_CASE("parse() ignores uninitialized upstream feed", "[RssParser]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	RssIgnores ignores;
	RssParser parser("http://example.com", rsscache, cfg, &ignores);

	rsspp::Feed upstream_feed;
	const auto feed = parser.parse(upstream_feed);
	REQUIRE(feed == nullptr);
}

TEST_CASE("parse() with no item GUID falls back to link+pubdate, link, and title",
	"[RssParser]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	RssIgnores ignores;
	RssParser parser("http://example.com", rsscache, cfg, &ignores);

	rsspp::Feed upstream_feed;
	upstream_feed.rss_version = rsspp::Feed::ATOM_1_0;
	upstream_feed.items.push_back({});
	rsspp::Item& upstream_item = upstream_feed.items[0];
	upstream_item.guid = "a real GUID";
	upstream_item.title = "title of article";
	upstream_item.link = "https://example.com/blog/post";
	upstream_item.pubDate = "2023-07-31";

	SECTION("uses GUID if it is available") {
		const auto feed = parser.parse(upstream_feed);
		REQUIRE(feed != nullptr);
		REQUIRE(feed->items().size() == 1);
		REQUIRE(feed->items().front()->guid() == "a real GUID");
	}

	SECTION("uses link+pubdate if GUID is not available") {
		upstream_item.guid.clear();

		const auto feed = parser.parse(upstream_feed);
		REQUIRE(feed != nullptr);
		REQUIRE(feed->items().size() == 1);
		REQUIRE(feed->items().front()->guid() == "https://example.com/blog/post2023-07-31");
	}

	SECTION("uses link if GUID and pubdate are not available") {
		upstream_item.guid.clear();
		upstream_item.pubDate.clear();

		const auto feed = parser.parse(upstream_feed);
		REQUIRE(feed != nullptr);
		REQUIRE(feed->items().size() == 1);
		REQUIRE(feed->items().front()->guid() == "https://example.com/blog/post");
	}

	SECTION("uses title if other options are not available") {
		upstream_item.guid.clear();
		upstream_item.pubDate.clear();
		upstream_item.link.clear();

		const auto feed = parser.parse(upstream_feed);
		REQUIRE(feed != nullptr);
		REQUIRE(feed->items().size() == 1);
		REQUIRE(feed->items().front()->guid() == "title of article");
	}
}

TEST_CASE("parse() renders html titles into plaintext if type indicates html",
	"[RssParser]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	RssIgnores ignores;
	RssParser parser("http://example.com", rsscache, cfg, &ignores);

	rsspp::Feed upstream_feed;
	upstream_feed.rss_version = rsspp::Feed::ATOM_1_0;
	upstream_feed.items.push_back({});
	rsspp::Item& upstream_item = upstream_feed.items[0];

	upstream_feed.title = "<b>title</b> of feed";
	upstream_item.title = "<b>title</b> of article";

	SECTION("uses feed title varbatim if no html type is indicated") {
		const auto feed = parser.parse(upstream_feed);
		REQUIRE(feed != nullptr);
		REQUIRE(feed->title() == "<b>title</b> of feed");
	}

	SECTION("renders out feed title if html type is indicated") {
		upstream_feed.title_type = "html";
		const auto feed = parser.parse(upstream_feed);
		REQUIRE(feed != nullptr);
		REQUIRE(feed->title() == "title of feed");
	}

	SECTION("uses item title varbatim if no html type is indicated") {
		const auto feed = parser.parse(upstream_feed);
		REQUIRE(feed != nullptr);
		REQUIRE(feed->items().size() == 1);
		REQUIRE(feed->items().front()->title() == "<b>title</b> of article");
	}

	SECTION("renders out item title if html type is indicated") {
		upstream_item.title_type = "html";
		const auto feed = parser.parse(upstream_feed);
		REQUIRE(feed != nullptr);
		REQUIRE(feed->items().size() == 1);
		REQUIRE(feed->items().front()->title() == "title of article");
	}
}

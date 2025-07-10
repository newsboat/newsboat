#include "rssparser.h"

#include "3rd-party/catch.hpp"

#include "cache.h"
#include "rss/feed.h"
#include "rss/item.h"
#include "rssfeed.h"
#include "rssignores.h"

using namespace Newsboat;

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

TEST_CASE("parse() generates a title when title element is missing",
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
	upstream_item.description = "<b>Just saying hello</b>";

	SECTION("creates a title from the URL") {
		upstream_item.link = "http://example.com/2023/08/29/hello-world.html";
		const auto feed = parser.parse(upstream_feed);
		auto item = feed->items()[0];
		REQUIRE(item->title() == "Hello world");
	}

	SECTION("creates a title from the content if the URL is numeric") {
		upstream_item.link = "http://example.com/1234567";

		SECTION("title from description") {
			const auto feed = parser.parse(upstream_feed);
			auto item = feed->items()[0];
			REQUIRE(item->title() == "Just saying hello");
		}

		SECTION("title from content_encoded") {
			upstream_item.description.clear();
			upstream_item.content_encoded = "<b>article text</b>";
			const auto feed = parser.parse(upstream_feed);
			auto item = feed->items()[0];
			REQUIRE(item->title() == "article text");
		}
	}
}

TEST_CASE("parse() extracts best enclosure", "[RssParser]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	RssIgnores ignores;
	RssParser parser("http://example.com", rsscache, cfg, &ignores);

	rsspp::Feed upstream_feed;
	upstream_feed.rss_version = rsspp::Feed::ATOM_1_0;
	upstream_feed.items.push_back({});
	rsspp::Item& upstream_item = upstream_feed.items[0];

	const auto make_enclosure = [](
			const std::string& url,
			const std::string& type,
			const std::string& description,
			const std::string& mime
	) -> rsspp::Enclosure {
		rsspp::Enclosure result;
		result.url = url;
		result.type = type;
		result.description = description;
		result.description_mime_type = mime;
		return result;
	};

	const auto image_enclosure1 = make_enclosure(
			"http://example.com/enclosure1",
			"image/png",
			"description1",
			"text/plain"
		);
	const auto image_enclosure2 = make_enclosure(
			"http://example.com/enclosure2",
			"image/jpg",
			"description2",
			"text/plain"
		);
	const auto audio_enclosure = make_enclosure(
			"http://example.com/enclosure3",
			"audio/ogg",
			"description3",
			"text/plain"
		);

	SECTION("podcast preferred over non-podcast enclosure") {
		const auto run_validation = [&]() {
			const auto feed = parser.parse(upstream_feed);
			REQUIRE(feed != nullptr);
			REQUIRE(feed->items().size() == 1);
			REQUIRE(feed->items()[0]->enclosure_url() == "http://example.com/enclosure3");
			REQUIRE(feed->items()[0]->enclosure_type() == "audio/ogg");
			REQUIRE(feed->items()[0]->enclosure_description() == "description3");
			REQUIRE(feed->items()[0]->enclosure_description_mime_type() == "text/plain");
		};

		SECTION("podcast first") {
			upstream_item.enclosures.push_back(audio_enclosure);
			upstream_item.enclosures.push_back(image_enclosure1);
			run_validation();
		}

		SECTION("podcast last") {
			upstream_item.enclosures.push_back(image_enclosure1);
			upstream_item.enclosures.push_back(audio_enclosure);
			run_validation();
		}
	}

	SECTION("last enclosure picked if both are non-podcast enclosures") {
		upstream_item.enclosures.push_back(image_enclosure1);
		upstream_item.enclosures.push_back(image_enclosure2);

		const auto feed = parser.parse(upstream_feed);
		REQUIRE(feed != nullptr);
		REQUIRE(feed->items().size() == 1);
		REQUIRE(feed->items()[0]->enclosure_url() == "http://example.com/enclosure2");
		REQUIRE(feed->items()[0]->enclosure_type() == "image/jpg");
		REQUIRE(feed->items()[0]->enclosure_description() == "description2");
		REQUIRE(feed->items()[0]->enclosure_description_mime_type() == "text/plain");
	}
}

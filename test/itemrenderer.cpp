#include "3rd-party/catch.hpp"

#include "cache.h"
#include "configcontainer.h"
#include "itemrenderer.h"
#include "test-helpers.h"

using namespace newsboat;

static const auto FEED_TITLE = std::string("Funniest jokes ever");
std::shared_ptr<RssFeed> create_test_feed(Cache* c)
{
	auto feed = std::make_shared<RssFeed>(c);

	feed->set_title(FEED_TITLE);

	return feed;
}

static const auto ITEM_TITLE = std::string("A frivolous test item");
static const auto ITEM_AUTHOR = std::string("Johnny Doe Jr.");
// Sun Sep 30 19:34:25 UTC 2018
static const auto ITEM_PUBDATE = time_t{1538336065};
static const auto ITEM_LINK = std::string("https://example.com/see-more");
static const auto ITEM_DESCRIPTON = std::string("<p>Hello, world!</p>");
static const auto ITEM_DESCRIPTON_RENDERED = std::string("Hello, world!");
static const auto ITEM_ENCLOSURE_URL =
	std::string("https://example.com/see-more.mp3");
static const auto ITEM_ENCLOSURE_TYPE = std::string("audio/mpeg");
static const auto ITEM_FLAGS = std::string("wasdhjkl");
// Flags are sorted for rendering.
static const auto ITEM_FLAGS_RENDERED = std::string("adhjklsw");
std::pair<std::shared_ptr<RssItem>, std::shared_ptr<RssFeed>> create_test_item(Cache* c)
{
	const auto feed = create_test_feed(c);

	auto item = std::make_shared<RssItem>(c);

	item->set_feedptr(feed);
	item->set_title(ITEM_TITLE);
	item->set_author(ITEM_AUTHOR);
	item->set_pubDate(ITEM_PUBDATE);
	item->set_link(ITEM_LINK);
	item->set_flags(ITEM_FLAGS);

	return {item, feed};
}

TEST_CASE("item_renderer::to_plain_text() produces a rendered representation "
		"of an RSS item, ready to be displayed to the user",
		"[item_renderer]")
{
	TestHelpers::EnvVar tzEnv("TZ");
	tzEnv.set("UTC");

	ConfigContainer cfg;
	// item_renderer uses that setting, so let's fix its value to make the test
	// reproducible
	cfg.set_configvalue("text-width", "80");

	Cache rsscache(":memory:", &cfg);

	std::shared_ptr<RssItem> item;
	std::shared_ptr<RssFeed> feed;
	std::tie(item, feed) = create_test_item(&rsscache);

	SECTION("Item without an enclosure") {
		item->set_description(ITEM_DESCRIPTON);

		const auto result = item_renderer::to_plain_text(cfg, item);

		const auto expected = std::string() +
			"Feed: " + FEED_TITLE + '\n' +
			"Title: " + ITEM_TITLE + '\n' +
			"Author: " + ITEM_AUTHOR + '\n' +
			"Date: Sun, 30 Sep 2018 19:34:25 +0000\n" +
			"Link: " + ITEM_LINK + '\n' +
			"Flags: " + ITEM_FLAGS_RENDERED + '\n' +
			" \n" +
			ITEM_DESCRIPTON_RENDERED + '\n';

		REQUIRE(result == expected);
	}

	SECTION("Item with an enclosure") {
		item->set_description(ITEM_DESCRIPTON);
		item->set_enclosure_url(ITEM_ENCLOSURE_URL);

		const auto result = item_renderer::to_plain_text(cfg, item);

		const auto expected = std::string() +
			"Feed: " + FEED_TITLE + '\n' +
			"Title: " + ITEM_TITLE + '\n' +
			"Author: " + ITEM_AUTHOR + '\n' +
			"Date: Sun, 30 Sep 2018 19:34:25 +0000\n" +
			"Link: " + ITEM_LINK + '\n' +
			"Flags: " + ITEM_FLAGS_RENDERED + '\n' +
			"Podcast Download URL: " + ITEM_ENCLOSURE_URL + '\n' +
			" \n" +
			ITEM_DESCRIPTON_RENDERED + '\n';

		REQUIRE(result == expected);
	}

	SECTION("Item with an enclosure that has a MIME type") {
		item->set_description(ITEM_DESCRIPTON);
		item->set_enclosure_url(ITEM_ENCLOSURE_URL);
		item->set_enclosure_type(ITEM_ENCLOSURE_TYPE);

		const auto result = item_renderer::to_plain_text(cfg, item);

		const auto expected = std::string() +
			"Feed: " + FEED_TITLE + '\n' +
			"Title: " + ITEM_TITLE + '\n' +
			"Author: " + ITEM_AUTHOR + '\n' +
			"Date: Sun, 30 Sep 2018 19:34:25 +0000\n" +
			"Link: " + ITEM_LINK + '\n' +
			"Flags: " + ITEM_FLAGS_RENDERED + '\n' +
			"Podcast Download URL: " + ITEM_ENCLOSURE_URL
				+ " (type: " + ITEM_ENCLOSURE_TYPE + ")\n" +
			" \n" +
			ITEM_DESCRIPTON_RENDERED + '\n';

		REQUIRE(result == expected);
	}

	SECTION("Item with some links in the description") {
		item->set_description(
				ITEM_DESCRIPTON +
				"<p>See also <a href='https://example.com'>this site</a>.</p>");

		const auto result = item_renderer::to_plain_text(cfg, item);

		const auto expected = std::string() +
			"Feed: " + FEED_TITLE + '\n' +
			"Title: " + ITEM_TITLE + '\n' +
			"Author: " + ITEM_AUTHOR + '\n' +
			"Date: Sun, 30 Sep 2018 19:34:25 +0000\n" +
			"Link: " + ITEM_LINK + '\n' +
			"Flags: " + ITEM_FLAGS_RENDERED + '\n' +
			" \n" +
			ITEM_DESCRIPTON_RENDERED + '\n' +
			" \n" +
			"See also this site[1].\n" +
			" \n" +
			"Links: \n" +
			"[1]: https://example.com/ (link)\n";

		REQUIRE(result == expected);
	}
}

TEST_CASE("item_renderer::to_plain_text() renders text to the width specified "
		"in `text-width` setting",
		"[item_renderer]")
{
	TestHelpers::EnvVar tzEnv("TZ");
	tzEnv.set("UTC");

	ConfigContainer cfg;

	Cache rsscache(":memory:", &cfg);

	std::shared_ptr<RssItem> item;
	std::shared_ptr<RssFeed> feed;
	std::tie(item, feed) = create_test_item(&rsscache);

	item->set_description(
			"Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
			"Pellentesque nisl massa, luctus ut ligula vitae, suscipit tempus "
			"velit. Vivamus sodales, quam in convallis posuere, libero nisi "
			"ultricies orci, nec lobortis.");

	const auto header = std::string() +
		"Feed: " + FEED_TITLE + '\n' +
		"Title: " + ITEM_TITLE + '\n' +
		"Author: " + ITEM_AUTHOR + '\n' +
		"Date: Sun, 30 Sep 2018 19:34:25 +0000\n" +
		"Link: " + ITEM_LINK + '\n' +
		"Flags: " + ITEM_FLAGS_RENDERED + '\n' +
		" \n";

	SECTION("If `text-width` is not set, text is rendered in 80 columns") {
		const auto result = item_renderer::to_plain_text(cfg, item);

		const auto expected = header +
			"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Pellentesque nisl \n" +
			"massa, luctus ut ligula vitae, suscipit tempus velit. Vivamus sodales, quam in \n" +
			"convallis posuere, libero nisi ultricies orci, nec lobortis.\n";

		REQUIRE(result == expected);
	}

	SECTION("If `text-width` is set to zero, text is rendered in 80 columns") {
		cfg.set_configvalue("text-width", "0");

		const auto result = item_renderer::to_plain_text(cfg, item);

		const auto expected = header +
			"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Pellentesque nisl \n" +
			"massa, luctus ut ligula vitae, suscipit tempus velit. Vivamus sodales, quam in \n" +
			"convallis posuere, libero nisi ultricies orci, nec lobortis.\n";

		REQUIRE(result == expected);
	}

	SECTION("Text is rendered in 37 columns") {
		cfg.set_configvalue("text-width", "37");

		const auto result = item_renderer::to_plain_text(cfg, item);

		const auto expected = header +
			"Lorem ipsum dolor sit amet, \n"
			"consectetur adipiscing elit. \n"
			"Pellentesque nisl massa, luctus ut \n"
			"ligula vitae, suscipit tempus velit. \n"
			"Vivamus sodales, quam in convallis \n"
			"posuere, libero nisi ultricies orci, \n"
			"nec lobortis.\n";

		REQUIRE(result == expected);
	}

	SECTION("Text is rendered in 120 columns") {
		cfg.set_configvalue("text-width", "120");

		const auto result = item_renderer::to_plain_text(cfg, item);

		const auto expected = header +
			"Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
			"Pellentesque nisl massa, luctus ut ligula vitae, suscipit \n"

			"tempus velit. Vivamus sodales, quam in convallis posuere, libero "
			"nisi ultricies orci, nec lobortis.\n";

		REQUIRE(result == expected);
	}
}

TEST_CASE("Empty fields are not rendered", "[item_renderer]")
{
	TestHelpers::EnvVar tzEnv("TZ");
	tzEnv.set("UTC");

	ConfigContainer cfg;
	// item_renderer uses that setting, so let's fix its value to make the test
	// reproducible
	cfg.set_configvalue("text-width", "80");

	Cache rsscache(":memory:", &cfg);

	std::shared_ptr<RssItem> item;
	std::shared_ptr<RssFeed> feed;
	std::tie(item, feed) = create_test_item(&rsscache);

	SECTION("Item without a feed title") {
		item->get_feedptr()->set_title("");

		const auto result = item_renderer::to_plain_text(cfg, item);

		const auto expected = std::string() +
			"Title: " + ITEM_TITLE + '\n' +
			"Author: " + ITEM_AUTHOR + '\n' +
			"Date: Sun, 30 Sep 2018 19:34:25 +0000\n" +
			"Link: " + ITEM_LINK + '\n' +
			"Flags: " + ITEM_FLAGS_RENDERED + '\n' +
			" \n";

		REQUIRE(result == expected);
	}

	SECTION("Item without a title") {
		item->set_title("");

		const auto result = item_renderer::to_plain_text(cfg, item);

		const auto expected = std::string() +
			"Feed: " + FEED_TITLE + '\n' +
			"Author: " + ITEM_AUTHOR + '\n' +
			"Date: Sun, 30 Sep 2018 19:34:25 +0000\n" +
			"Link: " + ITEM_LINK + '\n' +
			"Flags: " + ITEM_FLAGS_RENDERED + '\n' +
			" \n";

		REQUIRE(result == expected);
	}

	SECTION("Item without an author") {
		item->set_author("");

		const auto result = item_renderer::to_plain_text(cfg, item);

		const auto expected = std::string() +
			"Feed: " + FEED_TITLE + '\n' +
			"Title: " + ITEM_TITLE + '\n' +
			"Date: Sun, 30 Sep 2018 19:34:25 +0000\n" +
			"Link: " + ITEM_LINK + '\n' +
			"Flags: " + ITEM_FLAGS_RENDERED + '\n' +
			" \n";

		REQUIRE(result == expected);
	}

	SECTION("Item without a link") {
		item->set_link("");

		const auto result = item_renderer::to_plain_text(cfg, item);

		const auto expected = std::string() +
			"Feed: " + FEED_TITLE + '\n' +
			"Title: " + ITEM_TITLE + '\n' +
			"Author: " + ITEM_AUTHOR + '\n' +
			"Date: Sun, 30 Sep 2018 19:34:25 +0000\n" +
			"Flags: " + ITEM_FLAGS_RENDERED + '\n' +
			" \n";

		REQUIRE(result == expected);
	}

	SECTION("Item without an enclosure") {
		item->set_description(ITEM_DESCRIPTON);

		const auto result = item_renderer::to_plain_text(cfg, item);

		const auto expected = std::string() +
			"Feed: " + FEED_TITLE + '\n' +
			"Title: " + ITEM_TITLE + '\n' +
			"Author: " + ITEM_AUTHOR + '\n' +
			"Date: Sun, 30 Sep 2018 19:34:25 +0000\n" +
			"Link: " + ITEM_LINK + '\n' +
			"Flags: " + ITEM_FLAGS_RENDERED + '\n' +
			" \n" +
			ITEM_DESCRIPTON_RENDERED + '\n';

		REQUIRE(result == expected);
	}

	SECTION("Item without flags") {
		item->set_flags("");

		const auto result = item_renderer::to_plain_text(cfg, item);

		const auto expected = std::string() +
			"Feed: " + FEED_TITLE + '\n' +
			"Title: " + ITEM_TITLE + '\n' +
			"Author: " + ITEM_AUTHOR + '\n' +
			"Date: Sun, 30 Sep 2018 19:34:25 +0000\n" +
			"Link: " + ITEM_LINK + '\n' +
			" \n";

		REQUIRE(result == expected);
	}
}

TEST_CASE("item_renderer::to_plain_text honours `html-renderer` setting",
		"[item_renderer][broken]")
{
	TestHelpers::EnvVar tzEnv("TZ");
	tzEnv.set("UTC");

	ConfigContainer cfg;
	cfg.set_configvalue("text-width", "80");

	Cache rsscache(":memory:", &cfg);

	std::shared_ptr<RssItem> item;
	std::shared_ptr<RssFeed> feed;
	std::tie(item, feed) = create_test_item(&rsscache);

	SECTION("Single-paragraph description") {
		const auto description = std::string() +
			"<p>Hello, world! Check out "
			"<a href='https://example.com'>our site</a>.</p>";
		item->set_description(description);

		SECTION("internal renderer") {
			cfg.set_configvalue("html-renderer", "internal");

			const auto result = item_renderer::to_plain_text(cfg, item);

			const auto expected = std::string() +
				"Feed: " + FEED_TITLE + '\n' +
				"Title: " + ITEM_TITLE + '\n' +
				"Author: " + ITEM_AUTHOR + '\n' +
				"Date: Sun, 30 Sep 2018 19:34:25 +0000\n" +
				"Link: " + ITEM_LINK + '\n' +
				"Flags: " + ITEM_FLAGS_RENDERED + '\n' +
				" \n" +
				"Hello, world! Check out our site[1].\n" +
				" \n" +
				"Links: \n" +
				"[1]: https://example.com/ (link)\n";

			REQUIRE(result == expected);
		}

		// cat is pretty much guaranteed to be present in any Unix-like
		// environment, so let's use that instead of the less common w3m.
		SECTION("/bin/cat as a renderer") {
			cfg.set_configvalue("html-renderer", "/bin/cat");

			const auto result = item_renderer::to_plain_text(cfg, item);

			const auto expected = std::string() +
				"Feed: " + FEED_TITLE + '\n' +
				"Title: " + ITEM_TITLE + '\n' +
				"Author: " + ITEM_AUTHOR + '\n' +
				"Date: Sun, 30 Sep 2018 19:34:25 +0000\n" +
				"Link: " + ITEM_LINK + '\n' +
				"Flags: " + ITEM_FLAGS_RENDERED + '\n' +
				" \n" +
				description + '\n';

			REQUIRE(result == expected);
		}
	}


	SECTION("Multi-paragraph description") {
		const auto description = std::string() +
			"<p>Hello, world!</p>\n\n"
			"<p>Check out <a href='https://example.com'>our site</a>.</p>";
		item->set_description(description);

		SECTION("internal renderer") {
			cfg.set_configvalue("html-renderer", "internal");

			const auto result = item_renderer::to_plain_text(cfg, item);

			const auto expected = std::string() +
				"Feed: " + FEED_TITLE + '\n' +
				"Title: " + ITEM_TITLE + '\n' +
				"Author: " + ITEM_AUTHOR + '\n' +
				"Date: Sun, 30 Sep 2018 19:34:25 +0000\n" +
				"Link: " + ITEM_LINK + '\n' +
				"Flags: " + ITEM_FLAGS_RENDERED + '\n' +
				" \n" +
				"Hello, world!\n" +
				" \n" +
				"Check out our site[1].\n" +
				" \n" +
				"Links: \n" +
				"[1]: https://example.com/ (link)\n";

			REQUIRE(result == expected);
		}

		// cat is pretty much guaranteed to be present in any Unix-like
		// environment, so let's use that instead of the less common w3m.
		SECTION("/bin/cat as a renderer") {
			cfg.set_configvalue("html-renderer", "/bin/cat");

			const auto result = item_renderer::to_plain_text(cfg, item);

			const auto expected = std::string() +
				"Feed: " + FEED_TITLE + '\n' +
				"Title: " + ITEM_TITLE + '\n' +
				"Author: " + ITEM_AUTHOR + '\n' +
				"Date: Sun, 30 Sep 2018 19:34:25 +0000\n" +
				"Link: " + ITEM_LINK + '\n' +
				"Flags: " + ITEM_FLAGS_RENDERED + '\n' +
				" \n" +
				"<p>Hello, world!</p>\n" +
				" \n" +
				"<p>Check out <a href='https://example.com'>our site</a>.</p>\n";

			REQUIRE(result == expected);
		}
	}
}

TEST_CASE("item_renderer::get_feedtitle() returns item's feed title without "
		"soft hyphens if that's available",
		"[item_renderer]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);

	std::shared_ptr<RssItem> item;
	std::shared_ptr<RssFeed> feed;
	std::tie(item, feed) = create_test_item(&rsscache);

	SECTION("Title without soft hyphens") {
		feed->set_title("Welcome, lovely strangers!");
	}

	SECTION("Title containing soft hyphens") {
		feed->set_title("Wel\u00ADcome, lo\u00ADve\u00ADly stran\u00ADgers!");
	}

	const auto result = item_renderer::get_feedtitle(item);
	REQUIRE(result == "Welcome, lovely strangers!");
}

TEST_CASE("item_renderer::get_feedtitle() returns item's feed self-link "
		"if its title is empty",
		"[item_renderer]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);

	std::shared_ptr<RssItem> item;
	std::shared_ptr<RssFeed> feed;
	std::tie(item, feed) = create_test_item(&rsscache);

	const auto feedlink = std::string("https://rss.example.com/~joe/");

	feed->set_title("");
	feed->set_link(feedlink);

	const auto result = item_renderer::get_feedtitle(item);
	REQUIRE(result == feedlink);
}

TEST_CASE("item_renderer::get_feedtitle() returns item's feed URL "
		"if both the title and self-link are empty",
		"[item_renderer]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);

	std::shared_ptr<RssItem> item;
	std::shared_ptr<RssFeed> feed;
	std::tie(item, feed) = create_test_item(&rsscache);

	const auto feedurl = std::string("https://example.com/~joe/entries.rss");

	feed->set_title("");
	feed->set_link("");
	feed->set_rssurl(feedurl);

	const auto result = item_renderer::get_feedtitle(item);
	REQUIRE(result == feedurl);
}

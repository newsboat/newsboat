#include "3rd-party/catch.hpp"

#include "cache.h"
#include "configcontainer.h"
#include "itemrenderer.h"
#include "test-helpers.h"

using namespace newsboat;

static const auto ITEM_TITLE = std::string("A frivolous test item");
static const auto ITEM_AUTHOR = std::string("Johnny Doe Jr.");
// Sun Sep 30 19:34:25 UTC 2018
static const auto ITEM_PUBDATE = time_t{1538336065};
static const auto ITEM_LINK = std::string("https://example.com/see-more");
static const auto ITEM_DESCRIPTON = std::string("<p>Hello, world!</p>");
static const auto ITEM_DESCRIPTON_RENDERED = std::string("Hello, world!");
static const auto ITEM_ENCLOSURE_URL =
	std::string("https://example.com/see-more.mp3");
std::shared_ptr<RssItem> test_item(Cache* c)
{
	auto item = std::make_shared<RssItem>(c);

	item->set_title(ITEM_TITLE);
	item->set_author(ITEM_AUTHOR);
	item->set_pubDate(ITEM_PUBDATE);
	item->set_link(ITEM_LINK);

	return item;
}

TEST_CASE("ItemRenderer::to_plain_text() produces a rendered representation "
		"of an RSS item, ready to be displayed to the user",
		"[ItemRenderer]")
{
	TestHelpers::EnvVar tzEnv("TZ");
	tzEnv.set("UTC");

	ConfigContainer cfg;
	// ItemRenderer uses that setting, so let's fix its value to make the test
	// reproducible
	cfg.set_configvalue("text-width", "80");

	Cache rsscache(":memory:", &cfg);
	ItemRenderer renderer(&cfg);

	auto item = test_item(&rsscache);

	SECTION("Item without an enclosure") {
		item->set_description(ITEM_DESCRIPTON);

		const auto result = renderer.to_plain_text(item);

		const auto expected = std::string() +
			"Title: " + ITEM_TITLE + '\n' +
			"Author: " + ITEM_AUTHOR + '\n' +
			"Date: Sun, 30 Sep 2018 19:34:25 +0000\n" +
			"Link: " + ITEM_LINK + '\n' +
			" \n" +
			ITEM_DESCRIPTON_RENDERED + '\n';

		REQUIRE(result == expected);
	}

	SECTION("Item with an enclosure") {
		item->set_description(ITEM_DESCRIPTON);
		item->set_enclosure_url(ITEM_ENCLOSURE_URL);

		const auto result = renderer.to_plain_text(item);

		const auto expected = std::string() +
			"Title: " + ITEM_TITLE + '\n' +
			"Author: " + ITEM_AUTHOR + '\n' +
			"Date: Sun, 30 Sep 2018 19:34:25 +0000\n" +
			"Link: " + ITEM_LINK + '\n' +
			"Podcast Download URL: " + ITEM_ENCLOSURE_URL + '\n' +
			" \n" +
			ITEM_DESCRIPTON_RENDERED + '\n';

		REQUIRE(result == expected);
	}

	SECTION("Item with some links in the description") {
		item->set_description(
				ITEM_DESCRIPTON +
				"<p>See also <a href='https://example.com'>this site</a>.</p>");

		const auto result = renderer.to_plain_text(item);

		const auto expected = std::string() +
			"Title: " + ITEM_TITLE + '\n' +
			"Author: " + ITEM_AUTHOR + '\n' +
			"Date: Sun, 30 Sep 2018 19:34:25 +0000\n" +
			"Link: " + ITEM_LINK + '\n' +
			" \n" +
			ITEM_DESCRIPTON_RENDERED + '\n' +
			" \n" +
			"See also this site[1].\n" +
			" \n" +
			"Links: \n" +
			"[1]: https://example.com (link)\n";

		REQUIRE(result == expected);
	}
}

TEST_CASE("ItemRenderer::to_plain_text() renders text to the width specified "
		"in `text-width` setting",
		"[ItemRenderer]")
{
	TestHelpers::EnvVar tzEnv("TZ");
	tzEnv.set("UTC");

	ConfigContainer cfg;

	Cache rsscache(":memory:", &cfg);
	ItemRenderer renderer(&cfg);

	auto item = test_item(&rsscache);

	item->set_description(
			"Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
			"Pellentesque nisl massa, luctus ut ligula vitae, suscipit tempus "
			"velit. Vivamus sodales, quam in convallis posuere, libero nisi "
			"ultricies orci, nec lobortis.");

	const auto header = std::string() +
		"Title: " + ITEM_TITLE + '\n' +
		"Author: " + ITEM_AUTHOR + '\n' +
		"Date: Sun, 30 Sep 2018 19:34:25 +0000\n" +
		"Link: " + ITEM_LINK + '\n' +
		" \n";

	SECTION("If `text-width` is not set, text is rendered in 80 columns") {
		const auto result = renderer.to_plain_text(item);

		const auto expected = header +
			"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Pellentesque nisl \n" +
			"massa, luctus ut ligula vitae, suscipit tempus velit. Vivamus sodales, quam in \n" +
			"convallis posuere, libero nisi ultricies orci, nec lobortis.\n";

		REQUIRE(result == expected);
	}

	SECTION("If `text-width` is set to zero, text is rendered in 80 columns") {
		cfg.set_configvalue("text-width", "0");

		const auto result = renderer.to_plain_text(item);

		const auto expected = header +
			"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Pellentesque nisl \n" +
			"massa, luctus ut ligula vitae, suscipit tempus velit. Vivamus sodales, quam in \n" +
			"convallis posuere, libero nisi ultricies orci, nec lobortis.\n";

		REQUIRE(result == expected);
	}

	SECTION("Text is rendered in 37 columns") {
		cfg.set_configvalue("text-width", "37");

		const auto result = renderer.to_plain_text(item);

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

		const auto result = renderer.to_plain_text(item);

		const auto expected = header +
			"Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
			"Pellentesque nisl massa, luctus ut ligula vitae, suscipit \n"

			"tempus velit. Vivamus sodales, quam in convallis posuere, libero "
			"nisi ultricies orci, nec lobortis.\n";

		REQUIRE(result == expected);
	}
}

TEST_CASE("Empty fields are not rendered", "[ItemRenderer]") {
	TestHelpers::EnvVar tzEnv("TZ");
	tzEnv.set("UTC");

	ConfigContainer cfg;
	// ItemRenderer uses that setting, so let's fix its value to make the test
	// reproducible
	cfg.set_configvalue("text-width", "80");

	Cache rsscache(":memory:", &cfg);
	ItemRenderer renderer(&cfg);

	auto item = test_item(&rsscache);

	SECTION("Item without a title") {
		item->set_title("");

		const auto result = renderer.to_plain_text(item);

		const auto expected = std::string() +
			"Author: " + ITEM_AUTHOR + '\n' +
			"Date: Sun, 30 Sep 2018 19:34:25 +0000\n" +
			"Link: " + ITEM_LINK + '\n' +
			" \n";

		REQUIRE(result == expected);
	}

	SECTION("Item without an author") {
		item->set_author("");

		const auto result = renderer.to_plain_text(item);

		const auto expected = std::string() +
			"Title: " + ITEM_TITLE + '\n' +
			"Date: Sun, 30 Sep 2018 19:34:25 +0000\n" +
			"Link: " + ITEM_LINK + '\n' +
			" \n";

		REQUIRE(result == expected);
	}

	SECTION("Item without a link") {
		item->set_link("");

		const auto result = renderer.to_plain_text(item);

		const auto expected = std::string() +
			"Title: " + ITEM_TITLE + '\n' +
			"Author: " + ITEM_AUTHOR + '\n' +
			"Date: Sun, 30 Sep 2018 19:34:25 +0000\n" +
			" \n";

		REQUIRE(result == expected);
	}

	SECTION("Item without an enclosure") {
		item->set_description(ITEM_DESCRIPTON);

		const auto result = renderer.to_plain_text(item);

		const auto expected = std::string() +
			"Title: " + ITEM_TITLE + '\n' +
			"Author: " + ITEM_AUTHOR + '\n' +
			"Date: Sun, 30 Sep 2018 19:34:25 +0000\n" +
			"Link: " + ITEM_LINK + '\n' +
			" \n" +
			ITEM_DESCRIPTON_RENDERED + '\n';

		REQUIRE(result == expected);
	}
}

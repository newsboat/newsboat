#define ENABLE_IMPLICIT_FILEPATH_CONVERSIONS

#include "rssitem.h"

#include <unistd.h>

#include "3rd-party/catch.hpp"
#include "cache.h"
#include "configcontainer.h"
#include "rssfeed.h"

#include "test_helpers/envvar.h"
#include "test_helpers/stringmaker/optional.h"

using namespace newsboat;

TEST_CASE("RssItem::sort_flags() cleans up flags", "[RssItem]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", cfg);
	RssItem item(&rsscache);

	SECTION("Repeated letters do not erase other letters") {
		std::string inputflags = "Abcdecf";
		std::string result = "Abcdef";
		item.set_flags(inputflags);
		REQUIRE(result == item.flags());
	}

	SECTION("Non alpha characters in input flags are ignored") {
		std::string inputflags = "Abcd";
		item.set_flags(inputflags + "1234568790^\"#'é(£");
		REQUIRE(inputflags == item.flags());
	}
}

TEST_CASE("RssItem contains a number of matchable attributes", "[RssItem]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", cfg);
	RssItem item(&rsscache);

	SECTION("title") {
		const auto attr = "title";

		const auto title = "Example title";
		item.set_title(title);

		REQUIRE(item.attribute_value(attr) == title);

		SECTION("it is encoded to the locale's charset") {
			// Due to differences in how platforms handle //TRANSLIT in iconv,
			// we can't compare results to a known-good value. Instead, we
			// merely check that the result is *not* UTF-8.

			test_helpers::LcCtypeEnvVar lc_ctype;
			lc_ctype.set("C"); // This means ASCII

			const auto title = "こんにちは"; // "good afternoon"

			item.set_title(title);

			REQUIRE_FALSE(item.attribute_value(attr) == title);
		}
	}

	SECTION("link") {
		const auto attr = "link";

		const auto url = "http://example.com/newest-update.html";
		item.set_link(url);

		REQUIRE(item.attribute_value(attr) == url);
	}

	SECTION("author") {
		const auto attr = "author";

		const auto name = "John Doe";
		item.set_author(name);

		REQUIRE(item.attribute_value(attr) == name);

		SECTION("it is encoded to the locale's charset") {
			// Due to differences in how platforms handle //TRANSLIT in iconv,
			// we can't compare results to a known-good value. Instead, we
			// merely check that the result is *not* UTF-8.

			test_helpers::LcCtypeEnvVar lc_ctype;
			lc_ctype.set("C"); // This means ASCII

			const auto author = "李白"; // "Li Bai"

			item.set_author(author);

			REQUIRE_FALSE(item.attribute_value(attr) == author);
		}
	}

	SECTION("content") {
		const auto attr = "content";

		const auto description = "First line.\nSecond one.\nAnd finally the third";
		item.set_description(description, "text/plain");

		REQUIRE(item.attribute_value(attr) == description);

		SECTION("it is encoded to the locale's charset") {
			// Due to differences in how platforms handle //TRANSLIT in iconv,
			// we can't compare results to a known-good value. Instead, we
			// merely check that the result is *not* UTF-8.

			test_helpers::LcCtypeEnvVar lc_ctype;
			lc_ctype.set("C"); // This means ASCII

			const auto description = "こんにちは"; // "good afternoon"

			item.set_description(description, "text/plain");

			REQUIRE_FALSE(item.attribute_value(attr) == description);
		}
	}

	SECTION("date") {
		test_helpers::TzEnvVar tzEnv;
		tzEnv.set("UTC");

		const auto attr = "date";

		item.set_pubDate(1); // 1 second into the Unix epoch

		REQUIRE(item.attribute_value(attr) == "Thu, 01 Jan 1970 00:00:01 +0000");
	}

	SECTION("guid") {
		const auto attr = "guid";

		const auto guid = "unique-identifier-of-this-item";
		item.set_guid(guid);

		REQUIRE(item.attribute_value(attr) == guid);
	}

	SECTION("unread") {
		const auto attr = "unread";

		SECTION("for read items, attribute equals \"no\"") {
			item.set_unread(false);

			REQUIRE(item.attribute_value(attr) == "no");
		}

		SECTION("for unread items, attribute equals \"yes\"") {
			item.set_unread(true);

			REQUIRE(item.attribute_value(attr) == "yes");
		}
	}

	SECTION("enclosure_url") {
		const auto attr = "enclosure_url";

		const auto url = "https://example.com/podcast-ep-01.mp3";
		item.set_enclosure_url(url);

		REQUIRE(item.attribute_value(attr) == url);
	}

	SECTION("enclosure_type, MIME type of the enclosure") {
		const auto attr = "enclosure_type";

		const auto type = "audio/ogg";
		item.set_enclosure_type(type);

		REQUIRE(item.attribute_value(attr) == type);
	}

	SECTION("flags") {
		const auto attr = "flags";

		const auto flags = "abcdefg";
		item.set_flags(flags);

		REQUIRE(item.attribute_value(attr) == flags);
	}

	SECTION("age, the number of days since publication") {
		const auto attr = "age";

		item.set_pubDate(0); // beginning of Unix epoch

		const auto check = [&item, &attr]() {
			// time() returns seconds since Unix epoch, too
			const auto current_time = ::time(nullptr);
			const auto seconds_per_day = 24 * 60 * 60;
			const auto unix_days = current_time / seconds_per_day;

			REQUIRE(item.attribute_value(attr) == std::to_string(unix_days));
		};

		// check() evaluates current time twice: once explicitly by calling
		// time(), once implicitly by calling attribute_value("age"). On
		// a midnight, it's possible for the first call to execute on one day,
		// and the second call to execute on the next day. That'll lead to the
		// test failing. To avoid that, we perform a dirty trick: we run the
		// test once, and if it fails, we immediately re-run it. If the failure
		// was indeed caused by the aforementioned corner case, the second run
		// can't fail (we're already in the next day, and it's inconceivable
		// for code to run so slow that we're at the next midnight already). If
		// the second call fails, that failure is going to fail the whole
		// TEST_CASE, as it should.
		try {
			check();
		} catch (...) {
			check();
		}
	}

	SECTION("articleindex, article's position in the itemlist") {
		const auto attr = "articleindex";

		const auto check = [&attr, &item](unsigned int index) {
			item.set_index(index);

			REQUIRE(item.attribute_value(attr) == std::to_string(index));
		};

		check(1);
		check(3);
		check(65535);
		check(100500);
	}

	SECTION("unknown attributes are forwarded to parent feed") {
		auto feed = std::make_shared<RssFeed>(&rsscache, "");
		auto item = std::make_shared<RssItem>(&rsscache);
		feed->add_item(item);

		const auto feedindex = 42;
		feed->set_index(feedindex);

		const auto attr = "feedindex";

		SECTION("no parent feed => attribute unavailable") {
			REQUIRE(item->attribute_value(attr) == std::nullopt);
		}

		SECTION("request forwarded to parent feed") {
			item->set_feedptr(feed);

			REQUIRE(item->attribute_value(attr) == std::to_string(feedindex));
		}
	}
}

TEST_CASE("set_title() removes superfluous whitespace", "[RssItem]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", cfg);
	RssItem item(&rsscache);

	SECTION("duplicate whitespace") {
		item.set_title("lorem        ipsum");
		REQUIRE(item.title() == "lorem ipsum");

		item.set_title("abc\n\r \tdef");
		REQUIRE(item.title() == "abc def");
	}

	SECTION("leading whitespace") {
		item.set_title("\n\r\t lorem ipsum");
		REQUIRE(item.title() == "lorem ipsum");
	}

	SECTION("trailing whitespace") {
		item.set_title("lorem ipsum\n\r\t ");
		REQUIRE(item.title() == "lorem ipsum");
	}
}

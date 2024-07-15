#define ENABLE_IMPLICIT_FILEPATH_CONVERSIONS

#include "queuemanager.h"

#include "3rd-party/catch.hpp"
#include "test_helpers/chmod.h"
#include "test_helpers/envvar.h"
#include "test_helpers/misc.h"
#include "test_helpers/tempfile.h"

#include "cache.h"
#include "configcontainer.h"
#include "rssfeed.h"
#include "rssitem.h"

using namespace newsboat;

SCENARIO("Smoke test for QueueManager", "[QueueManager]")
{
	GIVEN("A fresh instance of QueueManager") {
		ConfigContainer cfg;

		auto rsscache = Cache::in_memory(cfg);

		RssItem item(rsscache.get());
		const std::string enclosure_url("https://example.com/podcast.mp3");
		item.set_enclosure_url(enclosure_url);
		item.set_enclosure_type("audio/mpeg");

		RssFeed feed(rsscache.get(), "https://example.com/news.atom");

		test_helpers::TempFile queue_file;
		QueueManager manager(&cfg, queue_file.get_path());

		THEN("the queue file is not automatically created") {
			REQUIRE_FALSE(test_helpers::file_exists(queue_file.get_path()));
		}

		WHEN("enqueue_url() is called") {
			const auto result = manager.enqueue_url(item, feed);

			THEN("the return value indicates success") {
				REQUIRE(result.status == EnqueueStatus::QUEUED_SUCCESSFULLY);
				REQUIRE(result.extra_info == "");
			}

			THEN("the queue file contains an entry") {
				REQUIRE(test_helpers::file_exists(queue_file.get_path()));

				const auto lines = test_helpers::file_contents(queue_file.get_path());
				REQUIRE(lines.size() == 2);
				REQUIRE(test_helpers::starts_with(enclosure_url, lines[0]));
				REQUIRE(test_helpers::ends_with(R"(/podcast.mp3")", lines[0]));

				REQUIRE(lines[1] == "");
			}

			THEN("the item is marked as enqueued") {
				REQUIRE(item.enqueued());
			}
		}

		WHEN("enqueue_url() is called on the same item twice in a row") {
			manager.enqueue_url(item, feed);
			const auto result = manager.enqueue_url(item, feed);

			THEN("the second call indicates that the enclosure is already in the queue") {
				REQUIRE(result.status == EnqueueStatus::URL_QUEUED_ALREADY);
				REQUIRE(result.extra_info == enclosure_url);
			}

			THEN("the queue file contains a single entry") {
				REQUIRE(test_helpers::file_exists(queue_file.get_path()));

				const auto lines = test_helpers::file_contents(queue_file.get_path());
				REQUIRE(lines.size() == 2);
				REQUIRE(test_helpers::starts_with(enclosure_url, lines[0]));
				REQUIRE(test_helpers::ends_with(R"(/podcast.mp3")", lines[0]));

				REQUIRE(lines[1] == "");
			}

			THEN("the item is marked as enqueued") {
				REQUIRE(item.enqueued());
			}
		}

		WHEN("enqueue_url() is called for multiple different items") {
			const auto result = manager.enqueue_url(item, feed);

			RssItem item2(rsscache.get());
			item2.set_enclosure_url("https://www.example.com/another.mp3");
			item2.set_enclosure_type("audio/mpeg");
			const auto result2 = manager.enqueue_url(item2, feed);

			RssItem item3(rsscache.get());
			item3.set_enclosure_url("https://joe.example.com/vacation.jpg");
			item3.set_enclosure_type("image/jpeg");
			const auto result3 = manager.enqueue_url(item3, feed);

			THEN("return values indicate success") {
				REQUIRE(result.status == EnqueueStatus::QUEUED_SUCCESSFULLY);
				REQUIRE(result.extra_info == "");

				REQUIRE(result2.status == EnqueueStatus::QUEUED_SUCCESSFULLY);
				REQUIRE(result2.extra_info == "");

				REQUIRE(result3.status == EnqueueStatus::QUEUED_SUCCESSFULLY);
				REQUIRE(result3.extra_info == "");
			}

			THEN("the queue file contains three entries") {
				REQUIRE(test_helpers::file_exists(queue_file.get_path()));

				const auto lines = test_helpers::file_contents(queue_file.get_path());
				REQUIRE(lines.size() == 4);
				REQUIRE(lines[0] != "");
				REQUIRE(lines[1] != "");
				REQUIRE(lines[2] != "");
				REQUIRE(lines[3] == "");
			}

			THEN("items are marked as enqueued") {
				REQUIRE(item.enqueued());
				REQUIRE(item2.enqueued());
				REQUIRE(item3.enqueued());
			}
		}
	}
}

SCENARIO("enqueue_url() errors if the filename is already used", "[QueueManager]")
{
	GIVEN("Pristine QueueManager and two RssItems") {
		ConfigContainer cfg;

		auto rsscache = Cache::in_memory(cfg);

		RssItem item1(rsscache.get());
		const std::string enclosure_url1("https://example.com/podcast.mp3");
		item1.set_enclosure_url(enclosure_url1);
		item1.set_enclosure_type("audio/mpeg");

		RssItem item2(rsscache.get());
		const std::string enclosure_url2("https://example.com/~joe/podcast.mp3");
		item2.set_enclosure_url(enclosure_url2);
		item2.set_enclosure_type("audio/mpeg");

		RssFeed feed(rsscache.get(), "https://example.com/news.atom");

		test_helpers::TempFile queue_file;
		QueueManager manager(&cfg, queue_file.get_path());

		WHEN("first item is enqueued") {
			const auto result = manager.enqueue_url(item1, feed);

			THEN("the return value indicates success") {
				REQUIRE(result.status == EnqueueStatus::QUEUED_SUCCESSFULLY);
				REQUIRE(result.extra_info == "");
			}

			THEN("the queue file contains a corresponding entry") {
				REQUIRE(test_helpers::file_exists(queue_file.get_path()));

				const auto lines = test_helpers::file_contents(queue_file.get_path());
				REQUIRE(lines.size() == 2);
				REQUIRE(test_helpers::starts_with(enclosure_url1, lines[0]));
				REQUIRE(test_helpers::ends_with(R"(/podcast.mp3")", lines[0]));

				REQUIRE(lines[1] == "");
			}

			THEN("the item is marked as enqueued") {
				REQUIRE(item1.enqueued());
			}

			AND_WHEN("second item is enqueued") {
				const auto result = manager.enqueue_url(item2, feed);

				THEN("the return value indicates that the filename is already used") {
					REQUIRE(result.status == EnqueueStatus::OUTPUT_FILENAME_USED_ALREADY);
					// That field contains a path to the temporary directory,
					// so we simply check that it's not empty.
					REQUIRE(result.extra_info != "");
				}

				THEN("the queue file still contains a single entry") {
					REQUIRE(test_helpers::file_exists(queue_file.get_path()));

					const auto lines = test_helpers::file_contents(queue_file.get_path());
					REQUIRE(lines.size() == 2);
					REQUIRE(test_helpers::starts_with(enclosure_url1, lines[0]));
					REQUIRE(test_helpers::ends_with(R"(/podcast.mp3")", lines[0]));

					REQUIRE(lines[1] == "");
				}

				THEN("the item is NOT marked as enqueued") {
					REQUIRE_FALSE(item2.enqueued());
				}
			}

		}
	}
}

SCENARIO("enqueue_url() errors if the queue file can't be opened for writing",
	"[QueueManager]")
{
	GIVEN("Pristine QueueManager, an RssItem, and an uneditable queue file") {
		ConfigContainer cfg;

		auto rsscache = Cache::in_memory(cfg);

		RssItem item(rsscache.get());
		item.set_enclosure_url("https://example.com/podcast.mp3");
		item.set_enclosure_type("audio/mpeg");

		RssFeed feed(rsscache.get(), "https://example.com/news.atom");

		test_helpers::TempFile queue_file;
		QueueManager manager(&cfg, queue_file.get_path());

		test_helpers::copy_file("data/empty-file", queue_file.get_path());
		// The file is read-only
		test_helpers::Chmod uneditable_queue_file(queue_file.get_path(), 0444);

		WHEN("enqueue_url() is called") {
			const auto result = manager.enqueue_url(item, feed);

			THEN("the return value indicates the file couldn't be written to") {
				REQUIRE(result.status == EnqueueStatus::QUEUE_FILE_OPEN_ERROR);
				REQUIRE(result.extra_info == queue_file.get_path().to_locale_string());
			}

			THEN("the item is NOT marked as enqueued") {
				REQUIRE_FALSE(item.enqueued());
			}
		}
	}
}

TEST_CASE("QueueManager puts files into a location configured by `download-path`",
	"[QueueManager]")
{
	ConfigContainer cfg;
	SECTION("path with a slash at the end") {
		cfg.set_configvalue("download-path", "/tmp/nonexistent-newsboat/");
	}
	SECTION("path without a slash at the end") {
		cfg.set_configvalue("download-path", "/tmp/nonexistent-newsboat");
	}

	auto rsscache = Cache::in_memory(cfg);

	RssItem item1(rsscache.get());
	const std::string enclosure_url1("https://example.com/podcast.mp3");
	item1.set_enclosure_url(enclosure_url1);
	item1.set_enclosure_type("audio/mpeg");

	RssItem item2(rsscache.get());
	const std::string enclosure_url2("https://example.com/~joe/podcast.ogg");
	item2.set_enclosure_url(enclosure_url2);
	item2.set_enclosure_type("audio/vorbis");

	RssFeed feed(rsscache.get(), "https://example.com/podcasts.atom");

	test_helpers::TempFile queue_file;
	QueueManager manager(&cfg, queue_file.get_path());

	const auto result1 = manager.enqueue_url(item1, feed);
	REQUIRE(result1.status == EnqueueStatus::QUEUED_SUCCESSFULLY);
	REQUIRE(result1.extra_info == "");

	REQUIRE(item1.enqueued());

	const auto result2 = manager.enqueue_url(item2, feed);
	REQUIRE(result2.status == EnqueueStatus::QUEUED_SUCCESSFULLY);
	REQUIRE(result2.extra_info == "");

	REQUIRE(item2.enqueued());

	REQUIRE(test_helpers::file_exists(queue_file.get_path()));

	const auto lines = test_helpers::file_contents(queue_file.get_path());
	REQUIRE(lines.size() == 3);
	REQUIRE(lines[0] ==
		R"(https://example.com/podcast.mp3 "/tmp/nonexistent-newsboat/podcast.mp3")");
	REQUIRE(lines[1] ==
		R"(https://example.com/~joe/podcast.ogg "/tmp/nonexistent-newsboat/podcast.ogg")");
	REQUIRE(lines[2] == "");
}

TEST_CASE("QueueManager names files according to the `download-filename-format` setting",
	"[QueueManager]")
{
	ConfigContainer cfg;
	// We set the download-path to a fixed value to ensure that we know
	// *exactly* how the result should look.
	cfg.set_configvalue("download-path", "/example/");

	auto rsscache = Cache::in_memory(cfg);

	RssItem item(rsscache.get());
	item.set_enclosure_url("https://example.com/~adam/podcast.mp3");
	item.set_enclosure_type("audio/mpeg");

	auto feed = std::make_shared<RssFeed>(rsscache.get(), "https://example.com/podcasts.atom");

	test_helpers::TempFile queue_file;
	QueueManager manager(&cfg, queue_file.get_path());

	SECTION("%n for current feed title, with slashes replaced by underscores") {
		cfg.set_configvalue("download-filename-format", "%n");
		feed->set_title("Feed title/theme");

		manager.enqueue_url(item, *feed);

		const auto lines = test_helpers::file_contents(queue_file.get_path());
		REQUIRE(lines.size() == 2);
		REQUIRE(lines[0] ==
			R"(https://example.com/~adam/podcast.mp3 "/example/Feed title_theme")");
		REQUIRE(lines[1] == "");
	}

	SECTION("%h for the enclosure URL's hostname") {
		cfg.set_configvalue("download-filename-format", "%h");

		manager.enqueue_url(item, *feed);

		const auto lines = test_helpers::file_contents(queue_file.get_path());
		REQUIRE(lines.size() == 2);
		REQUIRE(lines[0] == R"(https://example.com/~adam/podcast.mp3 "/example/example.com")");
		REQUIRE(lines[1] == "");
	}

	SECTION("%u for the enclosure URL's basename") {
		cfg.set_configvalue("download-filename-format", "%u");

		manager.enqueue_url(item, *feed);

		const auto lines = test_helpers::file_contents(queue_file.get_path());
		REQUIRE(lines.size() == 2);
		REQUIRE(lines[0] == R"(https://example.com/~adam/podcast.mp3 "/example/podcast.mp3")");
		REQUIRE(lines[1] == "");
	}

	SECTION("%F, %m, %b, %d, %H, %M, %S, %y, and %Y to render items's publication date with strftime") {
		// %H is sensitive to the timezone, so reset it to UTC for a time being
		test_helpers::TzEnvVar tzEnv;
		tzEnv.set("UTC");

		cfg.set_configvalue("download-filename-format", "%F, %m, %b, %d, %H, %M, %S, %y, and %Y");
		// Tue, 06 Apr 2021 15:38:19 +0000
		item.set_pubDate(1617723499);

		manager.enqueue_url(item, *feed);

		const auto lines = test_helpers::file_contents(queue_file.get_path());
		REQUIRE(lines.size() == 2);
		REQUIRE(lines[0] ==
			R"(https://example.com/~adam/podcast.mp3 "/example/2021-04-06, 04, Apr, 06, 15, 38, 19, 21, and 2021")");
		REQUIRE(lines[1] == "");
	}

	SECTION("%t for item title, with slashes replaced by underscores") {
		cfg.set_configvalue("download-filename-format", "%t");
		item.set_title("Rain/snow/sun in a single day");

		manager.enqueue_url(item, *feed);

		const auto lines = test_helpers::file_contents(queue_file.get_path());
		REQUIRE(lines.size() == 2);
		REQUIRE(lines[0] ==
			R"(https://example.com/~adam/podcast.mp3 "/example/Rain_snow_sun in a single day")");
		REQUIRE(lines[1] == "");
	}

	SECTION("%e for enclosure's filename extension") {
		cfg.set_configvalue("download-filename-format", "%e");

		manager.enqueue_url(item, *feed);

		const auto lines = test_helpers::file_contents(queue_file.get_path());
		REQUIRE(lines.size() == 2);
		REQUIRE(lines[0] ==
			R"(https://example.com/~adam/podcast.mp3 "/example/mp3")");
		REQUIRE(lines[1] == "");
	}

	SECTION("%N for the feed's title (even if `feed` passed into a function is different)") {
		cfg.set_configvalue("download-filename-format", "%N");

		SECTION("`feed` argument is irrelevant") {
			feed->set_title("Relevant feed");

			item.set_feedptr(feed);

			RssFeed irrelevant_feed(rsscache.get(), "https://example.com/podcasts.atom");
			irrelevant_feed.set_title("Irrelevant");

			manager.enqueue_url(item, irrelevant_feed);

			const auto lines = test_helpers::file_contents(queue_file.get_path());
			REQUIRE(lines.size() == 2);
			REQUIRE(lines[0] ==
				R"(https://example.com/~adam/podcast.mp3 "/example/Relevant feed")");
			REQUIRE(lines[1] == "");
		}

		SECTION("`feed` argument is relevant") {
			feed->set_title("Relevant feed");
			item.set_feedptr(feed);

			manager.enqueue_url(item, *feed);

			const auto lines = test_helpers::file_contents(queue_file.get_path());
			REQUIRE(lines.size() == 2);
			REQUIRE(lines[0] ==
				R"(https://example.com/~adam/podcast.mp3 "/example/Relevant feed")");
			REQUIRE(lines[1] == "");
		}
	}
}

TEST_CASE("autoenqueue() adds all enclosures of all items to the queue", "[QueueManager]")
{
	GIVEN("Pristine QueueManager and a feed of three items") {
		ConfigContainer cfg;

		auto rsscache = Cache::in_memory(cfg);

		RssFeed feed(rsscache.get(), "https://example.com/podcasts.atom");

		auto item1 = std::make_shared<RssItem>(rsscache.get());
		item1->set_enclosure_url("https://example.com/~adam/podcast.mp3");
		item1->set_enclosure_type("audio/mpeg");
		feed.add_item(item1);

		auto item2 = std::make_shared<RssItem>(rsscache.get());
		item2->set_enclosure_url("https://example.com/episode.ogg");
		item2->set_enclosure_type("audio/vorbis");
		feed.add_item(item2);

		auto item3 = std::make_shared<RssItem>(rsscache.get());
		item3->set_enclosure_url("https://example.com/~fae/painting.jpg");
		item3->set_enclosure_type("");
		feed.add_item(item3);

		test_helpers::TempFile queue_file;
		QueueManager manager(&cfg, queue_file.get_path());

		WHEN("autoenqueue() is called") {
			const auto result = manager.autoenqueue(feed);

			THEN("the return value indicates success") {
				REQUIRE(result.status == EnqueueStatus::QUEUED_SUCCESSFULLY);
				REQUIRE(result.extra_info == "");
			}

			THEN("the queue file contains three entries") {
				REQUIRE(test_helpers::file_exists(queue_file.get_path()));

				const auto lines = test_helpers::file_contents(queue_file.get_path());
				REQUIRE(lines.size() == 4);
				REQUIRE(lines[0] != "");
				REQUIRE(lines[1] != "");
				REQUIRE(lines[2] != "");
				REQUIRE(lines[3] == "");
			}

			THEN("items are marked as enqueued") {
				REQUIRE(item1->enqueued());
				REQUIRE(item2->enqueued());
				REQUIRE(item3->enqueued());
			}
		}
	}
}

SCENARIO("autoenqueue() errors if the filename is already used", "[QueueManager]")
{
	GIVEN("Pristine QueueManager and a feed of two items") {
		ConfigContainer cfg;

		auto rsscache = Cache::in_memory(cfg);

		RssFeed feed(rsscache.get(), "https://example.com/news.atom");

		auto item1 = std::make_shared<RssItem>(rsscache.get());
		const std::string enclosure_url1("https://example.com/podcast.mp3");
		item1->set_enclosure_url(enclosure_url1);
		item1->set_enclosure_type("audio/mpeg");
		feed.add_item(item1);

		auto item2 = std::make_shared<RssItem>(rsscache.get());
		const std::string enclosure_url2("https://example.com/~joe/podcast.mp3");
		item2->set_enclosure_url(enclosure_url2);
		item2->set_enclosure_type("audio/mpeg");
		feed.add_item(item2);

		test_helpers::TempFile queue_file;
		QueueManager manager(&cfg, queue_file.get_path());

		WHEN("autoenqueue() is called") {
			const auto result = manager.autoenqueue(feed);

			THEN("the return value indicates that the filename is already used") {
				REQUIRE(result.status == EnqueueStatus::OUTPUT_FILENAME_USED_ALREADY);
				// That field contains a path to the temporary directory,
				// so we simply check that it's not empty.
				REQUIRE(result.extra_info != "");
			}

			THEN("the queue file still contains a single entry") {
				REQUIRE(test_helpers::file_exists(queue_file.get_path()));

				const auto lines = test_helpers::file_contents(queue_file.get_path());
				REQUIRE(lines.size() == 2);
				REQUIRE(test_helpers::starts_with(enclosure_url1, lines[0]));
				REQUIRE(test_helpers::ends_with(R"(/podcast.mp3")", lines[0]));

				REQUIRE(lines[1] == "");
			}

			THEN("the first item is enqueued, the second one isn't") {
				REQUIRE(item1->enqueued());
				REQUIRE_FALSE(item2->enqueued());
			}
		}
	}
}

SCENARIO("autoenqueue() errors if the queue file can't be opened for writing",
	"[QueueManager]")
{
	GIVEN("Pristine QueueManager, a single-item feed, and an uneditable queue file") {
		ConfigContainer cfg;

		auto rsscache = Cache::in_memory(cfg);

		RssFeed feed(rsscache.get(), "https://example.com/news.atom");

		auto item = std::make_shared<RssItem>(rsscache.get());
		item->set_enclosure_url("https://example.com/podcast.mp3");
		item->set_enclosure_type("audio/mpeg");
		feed.add_item(item);

		test_helpers::TempFile queue_file;
		QueueManager manager(&cfg, queue_file.get_path());

		test_helpers::copy_file("data/empty-file", queue_file.get_path());
		// The file is read-only
		test_helpers::Chmod uneditable_queue_file(queue_file.get_path(), 0444);

		WHEN("autoenqueue() is called") {
			const auto result = manager.autoenqueue(feed);

			THEN("the return value indicates the file couldn't be written to") {
				REQUIRE(result.status == EnqueueStatus::QUEUE_FILE_OPEN_ERROR);
				REQUIRE(result.extra_info == queue_file.get_path().to_locale_string());
			}

			THEN("the item is NOT marked as enqueued") {
				REQUIRE_FALSE(item->enqueued());
			}
		}
	}
}

TEST_CASE("autoenqueue() skips already-enqueued items", "[QueueManager]")
{
	ConfigContainer cfg;
	// We set the download-path to a fixed value to ensure that we know
	// *exactly* how the result should look.
	cfg.set_configvalue("download-path", "/example/");

	auto rsscache = Cache::in_memory(cfg);

	RssFeed feed(rsscache.get(), "https://example.com/news.atom");

	auto item1 = std::make_shared<RssItem>(rsscache.get());
	item1->set_enclosure_url("https://example.com/podcast.mp3");
	item1->set_enclosure_type("audio/mpeg");
	feed.add_item(item1);

	auto item2 = std::make_shared<RssItem>(rsscache.get());
	item2->set_enclosure_url("https://example.com/podcast2.mp3");
	item2->set_enclosure_type("audio/mpeg");
	item2->set_enqueued(true);
	feed.add_item(item2);

	auto item3 = std::make_shared<RssItem>(rsscache.get());
	item3->set_enclosure_url("https://example.com/podcast3.mp3");
	item3->set_enclosure_type("audio/mpeg");
	feed.add_item(item3);

	test_helpers::TempFile queue_file;
	QueueManager manager(&cfg, queue_file.get_path());

	const auto result = manager.autoenqueue(feed);
	REQUIRE(result.status == EnqueueStatus::QUEUED_SUCCESSFULLY);
	REQUIRE(result.extra_info == "");

	REQUIRE(test_helpers::file_exists(queue_file.get_path()));

	const auto lines = test_helpers::file_contents(queue_file.get_path());
	REQUIRE(lines.size() == 3);
	REQUIRE(lines[0] == R"(https://example.com/podcast.mp3 "/example/podcast.mp3")");
	REQUIRE(lines[1] == R"(https://example.com/podcast3.mp3 "/example/podcast3.mp3")");
	REQUIRE(lines[2] == "");
}

TEST_CASE("autoenqueue() only enqueues HTTP and HTTPS URLs", "[QueueManager]")
{
	ConfigContainer cfg;
	// We set the download-path to a fixed value to ensure that we know
	// *exactly* how the result should look.
	cfg.set_configvalue("download-path", "/example/");

	auto rsscache = Cache::in_memory(cfg);

	RssFeed feed(rsscache.get(), "https://example.com/news.atom");

	auto item1 = std::make_shared<RssItem>(rsscache.get());
	item1->set_enclosure_url("https://example.com/podcast.mp3");
	item1->set_enclosure_type("audio/mpeg");
	feed.add_item(item1);

	auto item2 = std::make_shared<RssItem>(rsscache.get());
	item2->set_enclosure_url("http://example.com/podcast2.mp3");
	item2->set_enclosure_type("audio/mpeg");
	feed.add_item(item2);

	auto item3 = std::make_shared<RssItem>(rsscache.get());
	item3->set_enclosure_url("ftp://user@example.com/podcast3.mp3");
	item3->set_enclosure_type("audio/mpeg");
	feed.add_item(item3);

	test_helpers::TempFile queue_file;
	QueueManager manager(&cfg, queue_file.get_path());

	const auto result = manager.autoenqueue(feed);
	REQUIRE(result.status == EnqueueStatus::QUEUED_SUCCESSFULLY);
	REQUIRE(result.extra_info == "");

	REQUIRE(test_helpers::file_exists(queue_file.get_path()));

	const auto lines = test_helpers::file_contents(queue_file.get_path());
	REQUIRE(lines.size() == 3);
	REQUIRE(lines[0] == R"(https://example.com/podcast.mp3 "/example/podcast.mp3")");
	REQUIRE(lines[1] == R"(http://example.com/podcast2.mp3 "/example/podcast2.mp3")");
	REQUIRE(lines[2] == "");
}

TEST_CASE("autoenqueue() does not enqueue items with an invalid podcast type",
	"[QueueManager]")
{
	GIVEN("Pristine QueueManager and a feed of three items with one of them having an image enclosure") {
		ConfigContainer cfg;
		auto rsscache = Cache::in_memory(cfg);

		RssFeed feed(rsscache.get(), "https://example.com/news.atom");

		auto item1 = std::make_shared<RssItem>(rsscache.get());
		item1->set_enclosure_url("https://example.com/podcast1.mp3");
		item1->set_enclosure_type("audio/mpeg");
		feed.add_item(item1);

		auto item2 = std::make_shared<RssItem>(rsscache.get());
		item2->set_enclosure_url("http://example.com/not-a-podcast.jpg");
		item2->set_enclosure_type("image/jpeg");
		feed.add_item(item2);

		auto item3 = std::make_shared<RssItem>(rsscache.get());
		item3->set_enclosure_url("https://example.com/podcast2.mp3");
		item3->set_enclosure_type("audio/mpeg");
		feed.add_item(item3);

		auto item4 = std::make_shared<RssItem>(rsscache.get());
		item4->set_enclosure_url("https://example.com/podcast3.mp3");
		item4->set_enclosure_type("");
		feed.add_item(item4);

		test_helpers::TempFile queue_file;
		QueueManager manager(&cfg, queue_file.get_path());

		WHEN("autoenqueue() is called") {
			const auto result = manager.autoenqueue(feed);

			THEN("the return value indicates success") {
				REQUIRE(result.status == EnqueueStatus::QUEUED_SUCCESSFULLY);
				REQUIRE(result.extra_info == "");
			}

			THEN("the queue file contains two entries") {
				REQUIRE(test_helpers::file_exists(queue_file.get_path()));

				const auto lines = test_helpers::file_contents(queue_file.get_path());
				REQUIRE(lines.size() == 4);
				REQUIRE(lines[0] != "");
				REQUIRE(lines[1] != "");
				REQUIRE(lines[2] != "");
				REQUIRE(lines[3] == "");
			}

			THEN("items with an empty or a valid podcast type are marked as enqueued") {
				REQUIRE(item1->enqueued());
				REQUIRE_FALSE(item2->enqueued());
				REQUIRE(item3->enqueued());
				REQUIRE(item4->enqueued());
			}
		}
	}
}

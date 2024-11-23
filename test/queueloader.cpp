#define ENABLE_IMPLICIT_FILEPATH_CONVERSIONS

#include "queueloader.h"

#include "3rd-party/catch.hpp"
#include "test_helpers/chmod.h"
#include "test_helpers/misc.h"
#include "test_helpers/tempfile.h"

#include "configcontainer.h"
#include "download.h"

using namespace newsboat;
using namespace podboat;

bool contains_download_with_status(const std::vector<Download>& downloads,
	DlStatus status)
{
	return std::any_of(
			std::begin(downloads),
			std::end(downloads),
			[status](const Download& d) -> bool { return d.status() == status; });
}

TEST_CASE("Passes the callback to Download objects", "[QueueLoader]")
{
	bool sentry = false;
	const auto callback = [&sentry]() {
		sentry = true;
	};

	test_helpers::TempFile queueFile;
	test_helpers::copy_file("data/nonempty-queue-file", queueFile.get_path());

	ConfigContainer cfg;

	QueueLoader queue_loader(queueFile.get_path(), cfg, callback);
	std::vector<Download> downloads;
	queue_loader.reload(downloads);

	REQUIRE(downloads.size() == 5);

	sentry = false;
	// Download calls the callback when the download's status chagnes.
	downloads[0].set_status(DlStatus::DOWNLOADING);
	downloads[0].set_status(DlStatus::CANCELLED);
	REQUIRE(sentry);
}

TEST_CASE("reload() removes downloads iff they are marked as finished or deleted",
	"[QueueLoader]")
{
	auto empty_callback = []() {};
	newsboat::ConfigContainer cfg;
	test_helpers::TempFile queueFile;

	QueueLoader queue_loader(queueFile.get_path(), cfg, empty_callback);

	const std::vector<DlStatus> possible_statuses = {
		DlStatus::QUEUED,
		DlStatus::CANCELLED,
		DlStatus::DELETED,
		DlStatus::FINISHED,
		DlStatus::FAILED,
		DlStatus::MISSING,
		DlStatus::READY,
		DlStatus::PLAYED,
		DlStatus::RENAME_FAILED
		// Not including DlStatus::Downloading because that aborts the reload
	};

	GIVEN("A vector with a download in every possible state") {
		std::vector<Download> downloads;
		for (DlStatus status : possible_statuses) {
			downloads.emplace_back(empty_callback);
			downloads.back().set_status(status);
		}

		WHEN("reload() is called with also_remove_finished == false") {
			queue_loader.reload(downloads, false);

			THEN("only files marked as deleted are removed") {
				for (DlStatus status : possible_statuses) {
					INFO("status: " << static_cast<int>(status));
					if (status == DlStatus::DELETED) {
						REQUIRE_FALSE(contains_download_with_status(downloads, status));
					} else {
						REQUIRE(contains_download_with_status(downloads, status));
					}
				}
			}
		}

		WHEN("reload() is called with also_remove_finished == true") {
			queue_loader.reload(downloads, true);

			THEN("only files marked as finished or deleted are removed") {
				for (DlStatus status : possible_statuses) {
					INFO("status: " << static_cast<int>(status));
					if (status == DlStatus::FINISHED || status == DlStatus::DELETED) {
						REQUIRE_FALSE(contains_download_with_status(downloads, status));
					} else {
						REQUIRE(contains_download_with_status(downloads, status));
					}
				}
			}
		}
	}
}

TEST_CASE("reload() appends downloads from the array to the queue file",
	"[QueueLoader]")
{
	test_helpers::TempFile queueFile;
	test_helpers::copy_file("data/sentry-queue-file", queueFile.get_path());

	ConfigContainer cfg;
	auto empty_callback = []() {};

	QueueLoader queue_loader(queueFile.get_path(), cfg, empty_callback);

	std::vector<Download> downloads;

	const auto urls = std::vector<std::string> {
		"https://example.com/url1",
		"https://www.example.com/url2",
		"https://pods.example.com/new%20one",
		"https://pods.example.com/another",
		"https://example.com/sample"
	};
	const auto filenames = std::vector<std::string> {
		"first.mp4",
		"another.mp3",
		"a different one.ogg",
		"episode 0024.ogg",
		"another one.mp3"
	};
	const auto statuses = std::vector<DlStatus> {
		DlStatus::QUEUED,
		DlStatus::FINISHED,
		DlStatus::PLAYED,
		DlStatus::READY,
		DlStatus::RENAME_FAILED
	};

	for (size_t i = 0; i < urls.size(); ++i) {
		downloads.emplace_back(empty_callback);
		downloads.back().set_url(urls[i]);
		downloads.back().set_filename(filenames[i]);
		downloads.back().set_status(statuses[i]);
	}

	REQUIRE(downloads.size() == 5);

	queue_loader.reload(downloads);

	REQUIRE(downloads.size() == 6);

	const auto queue_contents = test_helpers::file_contents(queueFile.get_path());
	REQUIRE(queue_contents.size() == 7);
	REQUIRE(queue_contents[0] == R"(https://example.com/url1 "first.mp4")");
	REQUIRE(queue_contents[1] ==
		R"(https://www.example.com/url2 "another.mp3" finished)");
	REQUIRE(queue_contents[2] ==
		R"(https://pods.example.com/new%20one "a different one.ogg" played)");
	REQUIRE(queue_contents[3] ==
		R"(https://pods.example.com/another "episode 0024.ogg" downloaded)");
	REQUIRE(queue_contents[4] == R"(https://example.com/sample "another one.mp3")");
	REQUIRE(queue_contents[5] == R"(https://example.com/sentry.mp4 "sentry.mp4")");
	REQUIRE(queue_contents[6] == R"()");
}

TEST_CASE("reload() adds downloads from the queue file to the array",
	"[QueueLoader]")
{
	test_helpers::TempFile queueFile;
	test_helpers::copy_file("data/nonempty-queue-file", queueFile.get_path());

	ConfigContainer cfg;
	auto empty_callback = []() {};

	QueueLoader queue_loader(queueFile.get_path(), cfg, empty_callback);

	std::vector<Download> downloads;
	// This bogus download is a sentry value, to ensure that `reload()`
	// *appends* to the vector rather than overwriting its contents.
	downloads.emplace_back(empty_callback);

	queue_loader.reload(downloads);

	REQUIRE(downloads.size() == 6);

	REQUIRE(downloads[1].url() == "https://example.com/podcast/episode-001.ogg");
	REQUIRE(downloads[1].filename() == "nonexistent-file.ogg");
	REQUIRE(downloads[1].status() == DlStatus::QUEUED);

	REQUIRE(downloads[2].url() ==
		"https://wwww.example.com/another-podcast/episode-421.mp3");
	REQUIRE(downloads[2].filename() == "data/podcast-standin.mp3");
	REQUIRE(downloads[2].status() == DlStatus::READY);

	REQUIRE(downloads[3].url() == "https://pods.example.com/that_one/");
	REQUIRE(downloads[3].filename() == "data/podcast-standin.mp4");
	REQUIRE(downloads[3].status() == DlStatus::PLAYED);

	REQUIRE(downloads[4].url() == "https://pods.example.com/this%20one/audio.ogg");
	REQUIRE(downloads[4].filename() == "data/podcast-standin.ogg");
	REQUIRE(downloads[4].status() == DlStatus::FINISHED);

	REQUIRE(downloads[5].url() == "https://pods.example.com/partial.ogg");
	// Note that this file doesn't exist, but data/partial.ogg.part does.
	REQUIRE(downloads[5].filename() == "data/partial.ogg");
	REQUIRE(downloads[5].status() == DlStatus::FAILED);
}

TEST_CASE("reload() merges downloads in the queue file and the array", "[QueueLoader]")
{
	test_helpers::TempFile queueFile;
	test_helpers::copy_file("data/queue-file-for-merging", queueFile.get_path());

	ConfigContainer cfg;
	auto empty_callback = []() {};

	QueueLoader queue_loader(queueFile.get_path(), cfg, empty_callback);

	std::vector<Download> downloads;
	// This download exists in the queue file already, but it will be deleted
	// because of its status.
	downloads.emplace_back(empty_callback);
	downloads.back().set_url("https://example.com/podcast/episode-001.ogg");
	downloads.back().set_filename("nonexistent-file.ogg");
	downloads.back().set_status(DlStatus::DELETED);
	// This download exists in the queue file already, and it will be kept in
	// the array.
	downloads.emplace_back(empty_callback);
	downloads.back().set_url("https://example.com/podcast/episode-002.ogg");
	downloads.back().set_filename("a different episode.ogg");
	downloads.back().set_status(DlStatus::QUEUED);
	// This download doesn't exist in the queue file.
	downloads.emplace_back(empty_callback);
	downloads.back().set_url("https://example.com/another.mp3");
	downloads.back().set_filename("another.mp3");
	downloads.back().set_status(DlStatus::QUEUED);

	queue_loader.reload(downloads);

	REQUIRE(downloads.size() == 3);

	// This download was present in both the file and the array, and was kept.
	REQUIRE(downloads[0].url() == "https://example.com/podcast/episode-002.ogg");
	REQUIRE(downloads[0].filename() == "a different episode.ogg");
	REQUIRE(downloads[0].status() == DlStatus::QUEUED);

	REQUIRE(downloads[1].url() == "https://example.com/another.mp3");
	REQUIRE(downloads[1].filename() == "another.mp3");
	REQUIRE(downloads[1].status() == DlStatus::QUEUED);

	// This download was read from the queue file
	REQUIRE(downloads[2].url() == "https://example.com/this_doesnt_exist_in_code.mp3");
	REQUIRE(downloads[2].filename() == "data/podcast-standin.mp3");
	REQUIRE(downloads[2].status() == DlStatus::READY);
}

TEST_CASE("Overrides status in the queue with `MISSING` if file if the podcast is missing from the filesystem",
	"[QueueLoader]")
{
	test_helpers::TempFile queueFile;
	test_helpers::copy_file("data/queue-with-missing-files", queueFile.get_path());

	ConfigContainer cfg;
	auto empty_callback = []() {};

	QueueLoader queue_loader(queueFile.get_path(), cfg, empty_callback);
	std::vector<Download> downloads;
	queue_loader.reload(downloads);

	REQUIRE(downloads.size() == 3);
	REQUIRE(downloads[0].status() == DlStatus::MISSING);
	REQUIRE(downloads[1].status() == DlStatus::MISSING);
	REQUIRE(downloads[2].status() == DlStatus::MISSING);
}

TEST_CASE(
	"reload() sets `READY` status if the destination file "
	"is already present in the filesystem",
	"[QueueFile]")
{
	test_helpers::TempFile queueFile;
	test_helpers::copy_file("data/queue-with-unmarked-downloaded-file", queueFile.get_path());

	ConfigContainer cfg;
	auto empty_callback = []() {};

	QueueLoader queue_loader(queueFile.get_path(), cfg, empty_callback);
	std::vector<Download> downloads;
	queue_loader.reload(downloads);

	REQUIRE(downloads.size() == 1);
	REQUIRE(downloads[0].url() == "https://example.com/this-got-downloaded-earlier.mp3");
	REQUIRE(downloads[0].filename() == "data/podcast-standin.ogg");
	REQUIRE(downloads[0].status() == DlStatus::READY);
}

TEST_CASE("Generates filename if it's absent from the queue file",
	"[QueueLoader]")
{
	test_helpers::TempFile queueFile;
	test_helpers::copy_file("data/queue-without-filenames", queueFile.get_path());

	ConfigContainer cfg;

	Filepath download_path;
	SECTION("No `download-path` set") {
		download_path = cfg.get_configvalue_as_filepath("download-path");
	}
	SECTION("`download-path` set without a trailing slash") {
		cfg.set_configvalue("download-path", "/some/bogus value");
		// QueueLoader should append a slash if a setting doesn't contain it.
		download_path = Filepath::from_locale_string("/some/bogus value/");
	}
	SECTION("`download-path` set with a trailing slash") {
		cfg.set_configvalue("download-path",
			"/yet another/fictional path for downloads/");
		download_path = Filepath::from_locale_string("/yet another/fictional path for downloads/");
	}

	auto empty_callback = []() {};

	QueueLoader queue_loader(queueFile.get_path(), cfg, empty_callback);
	std::vector<Download> downloads;
	queue_loader.reload(downloads);

	REQUIRE(downloads.size() == 5);
	REQUIRE(downloads[0].filename() == download_path.join("filename.mp3"));
	REQUIRE(downloads[1].filename() == download_path.join("hello_world.ogg"));
	REQUIRE(downloads[2].filename() == download_path.join("here%27s_one_with_a_quote.mp4"));
	// These two downloads should have filenames based on current time, so we
	// only check their prefixes.
	REQUIRE(test_helpers::starts_with(download_path, downloads[3].filename()));
	REQUIRE(test_helpers::starts_with(download_path, downloads[4].filename()));
}

TEST_CASE("reload() removes files corresponding to \"DELETED\" downloads "
	"if `delete-played-files` is set",
	"[QueueLoader]")
{
	test_helpers::TempFile queueFile;
	auto empty_callback = []() {};

	ConfigContainer cfg;
	cfg.set_configvalue("delete-played-files", "yes");

	QueueLoader queue_loader(queueFile.get_path(), cfg, empty_callback);

	test_helpers::TempFile fileToBeDeleted;
	test_helpers::copy_file("data/empty-file", fileToBeDeleted.get_path());

	test_helpers::TempFile fileToBePreserved;
	test_helpers::copy_file("data/empty-file", fileToBePreserved.get_path());

	std::vector<Download> downloads;
	downloads.emplace_back(empty_callback);
	downloads.back().set_filename(fileToBeDeleted.get_path());
	downloads.back().set_url("https://nonempty.example.com");
	downloads.back().set_status(DlStatus::DELETED);

	// This list misses two statuses: DELETED, which is handled above, and
	// DOWNLOADING, which aborts the `reload()`.
	const std::vector<DlStatus> other_statuses = {
		DlStatus::QUEUED,
		DlStatus::CANCELLED,
		DlStatus::FAILED,
		DlStatus::MISSING,
		DlStatus::READY,
		DlStatus::PLAYED,
		DlStatus::FINISHED,
		DlStatus::RENAME_FAILED
	};
	for (const auto status : other_statuses) {
		downloads.emplace_back(empty_callback);
		downloads.back().set_filename(fileToBePreserved.get_path());
		downloads.back().set_url("https://eps.example.com/" + std::to_string(rand()));
		downloads.back().set_status(status);
	}

	REQUIRE(test_helpers::file_exists(fileToBeDeleted.get_path()));
	REQUIRE(test_helpers::file_exists(fileToBePreserved.get_path()));

	queue_loader.reload(downloads);

	REQUIRE_FALSE(test_helpers::file_exists(fileToBeDeleted.get_path()));
	REQUIRE(test_helpers::file_exists(fileToBePreserved.get_path()));
}

TEST_CASE("reload() removes files corresponding to \"FINISHED\" downloads "
	"if passed `true` as a second parameter and `delete-played-files` is set",
	"[QueueLoader]")
{
	test_helpers::TempFile queueFile;
	auto empty_callback = []() {};

	ConfigContainer cfg;
	cfg.set_configvalue("delete-played-files", "yes");

	QueueLoader queue_loader(queueFile.get_path(), cfg, empty_callback);

	test_helpers::TempFile fileInDeletedStatte;
	test_helpers::copy_file("data/empty-file", fileInDeletedStatte.get_path());

	test_helpers::TempFile fileInFinishedState;
	test_helpers::copy_file("data/empty-file", fileInFinishedState.get_path());

	test_helpers::TempFile fileToBePreserved;
	test_helpers::copy_file("data/empty-file", fileToBePreserved.get_path());

	std::vector<Download> downloads;
	downloads.emplace_back(empty_callback);
	downloads.back().set_filename(fileInDeletedStatte.get_path());
	downloads.back().set_url("https://nonempty.example.com/1");
	downloads.back().set_status(DlStatus::DELETED);

	downloads.emplace_back(empty_callback);
	downloads.back().set_filename(fileInFinishedState.get_path());
	downloads.back().set_url("https://nonempty.example.com/2");
	downloads.back().set_status(DlStatus::FINISHED);

	// This list misses three statuses: DELETED and FINISHED, which are handled
	// above, and DOWNLOADING, which aborts the `reload()`.
	const std::vector<DlStatus> other_statuses = {
		DlStatus::QUEUED,
		DlStatus::CANCELLED,
		DlStatus::FAILED,
		DlStatus::MISSING,
		DlStatus::READY,
		DlStatus::PLAYED,
		DlStatus::RENAME_FAILED
	};
	for (const auto status : other_statuses) {
		downloads.emplace_back(empty_callback);
		downloads.back().set_filename(fileToBePreserved.get_path());
		downloads.back().set_url("https://eps.example.com/" + std::to_string(rand()));
		downloads.back().set_status(status);
	}

	REQUIRE(test_helpers::file_exists(fileInDeletedStatte.get_path()));
	REQUIRE(test_helpers::file_exists(fileInFinishedState.get_path()));
	REQUIRE(test_helpers::file_exists(fileToBePreserved.get_path()));

	queue_loader.reload(downloads, true);

	REQUIRE_FALSE(test_helpers::file_exists(fileInDeletedStatte.get_path()));
	REQUIRE_FALSE(test_helpers::file_exists(fileInFinishedState.get_path()));
	REQUIRE(test_helpers::file_exists(fileToBePreserved.get_path()));
}

TEST_CASE("reload() does nothing if one of the downloads in the vector "
	"is in DOWNLOADING state",
	"[QueueLoader]")
{
	test_helpers::TempFile queueFile;
	test_helpers::copy_file("data/nonempty-queue-file", queueFile.get_path());

	auto empty_callback = []() {};
	ConfigContainer cfg;
	QueueLoader queue_loader(queueFile.get_path(), cfg, empty_callback);

	std::vector<Download> downloads;
	downloads.emplace_back(empty_callback);
	downloads.back().set_filename("whatever1");
	downloads.back().set_url("https://nonempty.example.com/1");
	downloads.back().set_status(DlStatus::DOWNLOADING);

	downloads.emplace_back(empty_callback);
	downloads.back().set_filename("whatever2");
	downloads.back().set_url("https://nonempty.example.com/2");
	downloads.back().set_status(DlStatus::FINISHED);

	SECTION("also-remove-finished = false") {
		queue_loader.reload(downloads, false);
	}

	SECTION("also-remove-finished = true") {
		queue_loader.reload(downloads, true);
	}

	REQUIRE(test_helpers::file_contents(queueFile.get_path()) ==
		test_helpers::file_contents("data/nonempty-queue-file"));
	REQUIRE(downloads.size() == 2);
	REQUIRE(downloads[0].filename() == "whatever1");
	REQUIRE(downloads[0].url() == "https://nonempty.example.com/1");
	REQUIRE(downloads[0].status() == DlStatus::DOWNLOADING);
	REQUIRE(downloads[1].filename() == "whatever2");
	REQUIRE(downloads[1].url() == "https://nonempty.example.com/2");
	REQUIRE(downloads[1].status() == DlStatus::FINISHED);
}

TEST_CASE("reload() skips empty lines in the queue file", "[QueueLoader]")
{
	test_helpers::TempFile queueFile;
	test_helpers::copy_file("data/queue-file-with-empty-lines", queueFile.get_path());

	auto empty_callback = []() {};
	ConfigContainer cfg;
	QueueLoader queue_loader(queueFile.get_path(), cfg, empty_callback);

	std::vector<Download> downloads;
	downloads.emplace_back(empty_callback);
	downloads.back().set_filename("newest.mp3");
	downloads.back().set_url("https://example.com/newest_episode.mp3");
	downloads.back().set_status(DlStatus::READY);

	SECTION("also-remove-finished = false") {
		queue_loader.reload(downloads, false);
	}

	SECTION("also-remove-finished = true") {
		queue_loader.reload(downloads, true);
	}

	REQUIRE(downloads.size() == 6);

	REQUIRE(downloads[0].filename() == "newest.mp3");
	REQUIRE(downloads[0].url() == "https://example.com/newest_episode.mp3");
	REQUIRE(downloads[0].status() == DlStatus::READY);

	REQUIRE(downloads[1].filename() == "first.mp3");
	REQUIRE(downloads[1].url() == "https://example.com/episode01.mp3");
	REQUIRE(downloads[1].status() == DlStatus::QUEUED);

	REQUIRE(downloads[2].filename() == "second.mp3");
	REQUIRE(downloads[2].url() == "https://example.com/episode02.mp3");
	REQUIRE(downloads[2].status() == DlStatus::QUEUED);

	REQUIRE(downloads[3].filename() == "third.mp3");
	REQUIRE(downloads[3].url() == "https://example.com/episode03.mp3");
	REQUIRE(downloads[3].status() == DlStatus::QUEUED);

	REQUIRE(downloads[4].filename() == "fourth.mp3");
	REQUIRE(downloads[4].url() == "https://example.com/episode04.mp3");
	REQUIRE(downloads[4].status() == DlStatus::QUEUED);

	REQUIRE(downloads[5].filename() == "fifth.mp3");
	REQUIRE(downloads[5].url() == "https://example.com/episode05.mp3");
	REQUIRE(downloads[5].status() == DlStatus::QUEUED);
}

TEST_CASE("reload() removes empty lines from the queue file", "[QueueLoader]")
{
	test_helpers::TempFile queueFile;
	test_helpers::copy_file("data/queue-file-with-empty-lines", queueFile.get_path());

	auto empty_callback = []() {};
	ConfigContainer cfg;
	QueueLoader queue_loader(queueFile.get_path(), cfg, empty_callback);

	std::vector<Download> downloads;

	SECTION("also-remove-finished = false") {
		queue_loader.reload(downloads, false);
	}

	SECTION("also-remove-finished = true") {
		queue_loader.reload(downloads, true);
	}

	REQUIRE(test_helpers::file_contents(queueFile.get_path()) ==
		test_helpers::file_contents("data/queue-file-with-empty-lines-removed"));
}

TEST_CASE("No exceptions are thrown if reload() can't read the queue file",
	"[QueueLoader]")
{
	test_helpers::TempFile queueFile;
	test_helpers::copy_file("data/empty-file", queueFile.get_path());
	// Make the file write-only.
	test_helpers::Chmod queueFileMode(queueFile.get_path(), 0200);

	auto empty_callback = []() {};
	ConfigContainer cfg;
	QueueLoader queue_loader(queueFile.get_path(), cfg, empty_callback);

	std::vector<Download> downloads;

	REQUIRE_NOTHROW(queue_loader.reload(downloads));
}

TEST_CASE("No exceptions are thrown if reload() can't write the queue file",
	"[QueueLoader]")
{
	test_helpers::TempFile queueFile;
	test_helpers::copy_file("data/empty-file", queueFile.get_path());
	// Make the file read-only.
	test_helpers::Chmod queueFileMode(queueFile.get_path(), 0400);

	auto empty_callback = []() {};
	ConfigContainer cfg;
	QueueLoader queue_loader(queueFile.get_path(), cfg, empty_callback);

	std::vector<Download> downloads;

	REQUIRE_NOTHROW(queue_loader.reload(downloads));
}

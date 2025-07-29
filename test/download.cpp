#include "download.h"

#include <set>
#include <vector>

#include "3rd-party/catch.hpp"

using namespace podboat;

using newsboat::operator""_path;

TEST_CASE("Require-view-update callback gets called when download progress or status changes",
	"[Download]")
{
	GIVEN("A Download object") {
		bool got_called = false;
		std::function<void()> callback = [&got_called]() {
			got_called = true;
		};

		Download d(callback);

		REQUIRE(d.status() == DlStatus::QUEUED);

		WHEN("its progress is updated (increased)") {
			d.set_progress(1.0, 1.0);

			THEN("the require-view-update callback gets called") {
				REQUIRE(got_called);
			}
		}

		WHEN("its progress has not changed") {
			d.set_progress(0.0, 1.0);

			THEN("the require-view-update callback does not get called") {
				REQUIRE_FALSE(got_called);
			}
		}

		WHEN("its status is changed") {
			d.set_status(DlStatus::DOWNLOADING);

			THEN("the require-view-update callback gets called") {
				REQUIRE(got_called);
			}
		}

		WHEN("when its status is not changed") {
			d.set_status(DlStatus::QUEUED);

			THEN("the require-view-update callback does not get called") {
				REQUIRE_FALSE(got_called);
			}
		}
	}
}

TEST_CASE("filename() returns download's target filename", "[Download]")
{
	auto emptyCallback = []() {};

	Download d(emptyCallback);

	SECTION("filename returns empty string by default") {
		REQUIRE(d.filename() == newsboat::Filepath{});
	}


	SECTION("filename returns same string which is set via set_filename") {
		const auto path = "abc"_path;
		d.set_filename(path);
		REQUIRE(d.filename() == path);
	}

	SECTION("filename will return the latest configured filename") {
		d.set_filename("abc"_path);
		const auto path = "def"_path;
		d.set_filename(path);

		REQUIRE(d.filename() == path);
	}
}

TEST_CASE("percents_finished() takes current progress into account",
	"[Download]")
{
	auto emptyCallback = []() {};

	Download d(emptyCallback);

	SECTION("percents_finished() returns 0 by default") {
		REQUIRE(d.percents_finished() == 0);
	}

	SECTION("percents_finished() updates according to set_progress's arguments") {
		const double downloaded = 3.0;
		const double total = 7.0;
		d.set_progress(downloaded, total);

		REQUIRE(d.percents_finished() == Catch::Approx(100.0 * (downloaded / total)));
	}

	SECTION("percents_finished() takes offset into account") {
		const double offset = 5.0;
		const double downloaded = 3.0;
		const double total = 12.0;

		d.set_offset(offset);
		d.set_progress(downloaded, total);

		const auto expected = Catch::Approx(100.0 * ((downloaded + offset) /
					(total + offset)));
		REQUIRE(d.percents_finished() == expected);
	}

	SECTION("percents_finished() returns 0 if total is unknown (0)") {
		const double downloaded = 3.0;
		const double total = 0.0;
		d.set_progress(downloaded, total);

		REQUIRE(d.percents_finished() == 0);
	}
}

TEST_CASE("basename() returns all text after last slash in the filename",
	"[Download]")
{
	auto emptyCallback = []() {};

	Download d(emptyCallback);

	SECTION("basename() returns empty string by default") {
		REQUIRE(d.basename() == newsboat::Filepath{});
	}

	SECTION("basename() returns full filename if it does not contain slashes") {
		const auto filename = "lorem_ipsum.txt"_path;
		d.set_filename(filename);

		REQUIRE(d.basename() == filename);
	}

	SECTION("basename() returns only text after the last slash in the filename") {
		const auto basename = "lorem_ipsum.txt"_path;
		const auto path = "/test/path/"_path;
		const auto filename = path.join(basename);
		d.set_filename(filename);

		REQUIRE(d.basename() == basename);
	}
}

TEST_CASE("status_text() does not contain obvious copy-paste errors",
	"[Download]")
{
	auto emptyCallback = []() {};

	Download d(emptyCallback);

	const std::vector<DlStatus> status_values {
		DlStatus::QUEUED,
		DlStatus::DOWNLOADING,
		DlStatus::CANCELLED,
		DlStatus::DELETED,
		DlStatus::FINISHED,
		DlStatus::FAILED,
		DlStatus::MISSING,
		DlStatus::READY,
		DlStatus::PLAYED
	};

	SECTION("status_text returns a non-empty string for each possible status") {
		for (const DlStatus& status : status_values) {
			d.set_status(status);

			REQUIRE_FALSE(d.status_text() == "");
		}
	}

	SECTION("status_text() returns a unique text for each of the possible status values") {
		std::set<std::string> status_texts;
		for (const DlStatus& status : status_values) {
			d.set_status(status);
			status_texts.insert(d.status_text());
		}

		REQUIRE(status_values.size() == status_texts.size());
	}
}

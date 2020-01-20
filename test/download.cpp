#include "download.h"

#include "test-helpers.h"

using namespace podboat;

TEST_CASE("Require-view-update callback gets called when download progress or status changes", "[Download]")
{
	GIVEN("A Download object") {
		bool got_called = false;
		std::function<void()> callback = [&got_called]() {
			got_called = true;
		};

		Download d(callback);

		REQUIRE(d.status() == DlStatus::QUEUED);

		WHEN("its progress is updated (increased)") {
			got_called = false;
			d.set_progress(1.0, 1.0);

			THEN("the require-view-update callback gets called") {
				REQUIRE(got_called);
			}
		}

		WHEN("its progress has not changed") {
			got_called = false;
			d.set_progress(0.0, 1.0);

			THEN("the require-view-update callback does not get called") {
				REQUIRE_FALSE(got_called);
			}
		}

		WHEN("its status is changed") {
			got_called = false;
			d.set_status(DlStatus::DOWNLOADING);

			THEN("the require-view-update callback gets called") {
				REQUIRE(got_called);
			}
		}

		WHEN("when its status is not changed") {
			got_called = false;
			d.set_status(DlStatus::QUEUED);

			THEN("the require-view-update callback does not get called") {
				REQUIRE_FALSE(got_called);
			}
		}
	}
}

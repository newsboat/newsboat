#include "queueloader.h"

#include <fstream>

#include "3rd-party/catch.hpp"
#include "configcontainer.h"
#include "download.h"
#include "test-helpers.h"

using namespace podboat;

bool contains_download_with_status(std::vector<Download>& downloads,
	DlStatus status)
{
	for (const Download& d : downloads) {
		if (d.status() == status) {
			return true;
		}
	}
	return false;
}

TEST_CASE("reload removes downloads iff they are marked as finished or deleted",
	"[QueueLoader]")
{
	auto empty_callback = []() {};
	newsboat::ConfigContainer cfg;
	TestHelpers::TempFile queueFile;

	QueueLoader queue_loader(queueFile.get_path(), cfg, empty_callback);

	std::vector<DlStatus> possible_statuses = {
		DlStatus::QUEUED,
		DlStatus::CANCELLED,
		DlStatus::DELETED,
		DlStatus::FINISHED,
		DlStatus::FAILED,
		DlStatus::ALREADY_DOWNLOADED,
		DlStatus::READY,
		DlStatus::PLAYED
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

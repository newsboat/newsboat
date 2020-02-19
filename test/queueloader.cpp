#include "queueloader.h"

#include "3rd-party/catch.hpp"
#include "configcontainer.h"
#include "download.h"
#include "test-helpers.h"

using namespace podboat;

TEST_CASE("reload(also_remove_finished == true) removes files iff they are marked as finished or deleted",
	"[QueueLoader]")
{
	auto emptyCallback = []() {};
	newsboat::ConfigContainer cfg;
	TestHelpers::TempFile queueFile;

	std::vector<Download> downloads;
	downloads.emplace_back(emptyCallback);

	QueueLoader queue_loader(queueFile.get_path(), cfg, emptyCallback);

	REQUIRE(downloads.size() == 1);

	SECTION("reload removes download marked as DELETED") {
		downloads.back().set_status(DlStatus::DELETED);
		queue_loader.reload(downloads, true);
		REQUIRE(downloads.size() == 0);
	}

	SECTION("reload removes download marked as FINISHED") {
		downloads.back().set_status(DlStatus::FINISHED);
		queue_loader.reload(downloads, true);
		REQUIRE(downloads.size() == 0);
	}


	SECTION("reload does not remove download marked as QUEUED") {
		downloads.back().set_status(DlStatus::QUEUED);
		queue_loader.reload(downloads, true);
		REQUIRE(downloads.size() == 1);
	}

	SECTION("reload does not remove download marked as DOWNLOADING") {
		downloads.back().set_status(DlStatus::DOWNLOADING);
		queue_loader.reload(downloads, true);
		REQUIRE(downloads.size() == 1);
	}

	SECTION("reload does not remove download marked as CANCELLED") {
		downloads.back().set_status(DlStatus::CANCELLED);
		queue_loader.reload(downloads, true);
		REQUIRE(downloads.size() == 1);
	}

	SECTION("reload does not remove download marked as FAILED") {
		downloads.back().set_status(DlStatus::FAILED);
		queue_loader.reload(downloads, true);
		REQUIRE(downloads.size() == 1);
	}

	SECTION("reload does not remove download marked as ALREADY_DOWNLOADED") {
		downloads.back().set_status(DlStatus::ALREADY_DOWNLOADED);
		queue_loader.reload(downloads, true);
		REQUIRE(downloads.size() == 1);
	}

	SECTION("reload does not remove download marked as READY") {
		downloads.back().set_status(DlStatus::READY);
		queue_loader.reload(downloads, true);
		REQUIRE(downloads.size() == 1);
	}

	SECTION("reload does not remove download marked as PLAYED") {
		downloads.back().set_status(DlStatus::PLAYED);
		queue_loader.reload(downloads, true);
		REQUIRE(downloads.size() == 1);
	}
}

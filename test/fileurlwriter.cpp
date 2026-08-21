#include "fileurlwriter.h"

#include <fstream>
#include <unistd.h>

#include "3rd-party/catch.hpp"
#include "fileurlreader.h"
#include "test_helpers/chmod.h"
#include "test_helpers/misc.h"
#include "test_helpers/tempfile.h"

using namespace newsboat;

TEST_CASE("FileUrlWriter writes files that are understood by FileUrlReader",
	"[FileUrlWriter]")
{
	const auto testDataPath = "data/test-urls.txt"_path;
	test_helpers::TempFile urlsFile;

	test_helpers::copy_file(testDataPath, urlsFile.get_path());

	FileUrlReader u(urlsFile.get_path());
	u.reload();
	REQUIRE_FALSE(u.get_urls().empty());
	REQUIRE_FALSE(u.get_alltags().empty());

	std::ofstream urlsFileStream(urlsFile.get_path().to_locale_string());
	REQUIRE(urlsFileStream.is_open());
	urlsFileStream << std::string();
	urlsFileStream.close();

	FileUrlWriter::write_urls(u.get_urls(), u.get_path());

	FileUrlReader u2(urlsFile.get_path());
	u2.reload();
	REQUIRE_FALSE(u2.get_urls().empty());
	REQUIRE_FALSE(u2.get_alltags().empty());
	REQUIRE(u.get_alltags() == u2.get_alltags());

	auto u_urls = u.get_urls();
	auto u2_urls = u2.get_urls();
	REQUIRE(u_urls.size() == u2_urls.size());
	for (std::size_t i = 0; i < u_urls.size(); i++) {
		REQUIRE(u_urls[i].url == u2_urls[i].url);
	}
	for (const auto& feed_url : u.get_urls()) {
		REQUIRE(u.get_entry(feed_url.url)->tags == u2.get_entry(feed_url.url)->tags);
	}

}

TEST_CASE("UrlFileWriter quotes exec: and filter: urls", "[FileUrlWriter]")
{
	test_helpers::TempFile urlsFile;
	FileUrlReader u(urlsFile.get_path());

	GIVEN("a url reader with an exec: and filter: feed") {
		u.add_url(R"(exec:cat header body footer)", {"tag1", "tag 2"});
		u.add_url(R"(filter:sed "s/foo/bar":https://example.com/feed.xml)", {"tag1", "tag 2"});

		WHEN("the urls are saved to disk") {
			FileUrlWriter::write_urls(u.get_urls(), u.get_path());

			THEN("the exec: and filter: feeds are properly quoted") {
				const auto content = test_helpers::file_contents(urlsFile.get_path());
				// Looks like there is an extra newline at the end so allowing more than 2 lines
				REQUIRE(content.size() >= 2);
				REQUIRE(content[0] == R"("exec:cat header body footer" "tag1" "tag 2")");
				REQUIRE(content[1] ==
					R"("filter:sed \"s/foo/bar\":https://example.com/feed.xml" "tag1" "tag 2")");
			}
		}
	}
}

TEST_CASE("UrlFileWriter works fine when file does not exist", "[FileUrlWriter]")
{
	test_helpers::TempFile urlsFile;
	const auto error_message = FileUrlWriter::write_urls({}, urlsFile.get_path());
	REQUIRE_FALSE(error_message.has_value());

	SECTION("after writing file, reload() succeeds") {
		FileUrlReader u(urlsFile.get_path());
		const auto error_message = u.reload();
		REQUIRE_FALSE(error_message.has_value());
	}
}

TEST_CASE("UrlFileWriter returns error when urls file is not writable", "[FileUrlWriter]")
{
	const auto testDataPath = "data/test-urls.txt"_path;

	test_helpers::TempFile urlsFile;
	test_helpers::copy_file(testDataPath, urlsFile.get_path());

	GIVEN("urls file is not writable") {
		test_helpers::Chmod notWritable(urlsFile.get_path(), S_IRUSR);

		THEN("write_urls() returns an error message") {
			const auto error_message = FileUrlWriter::write_urls({}, urlsFile.get_path());
			REQUIRE(error_message.has_value());

			SECTION("the error message contains the filename") {
				INFO("error_message: " + error_message.value());
				REQUIRE(error_message.value().find(urlsFile.get_path().to_locale_string()) !=
					std::string::npos);
			}
		}
	}
}

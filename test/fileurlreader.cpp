#define ENABLE_IMPLICIT_FILEPATH_CONVERSIONS

#include "fileurlreader.h"

#include <fstream>
#include <unistd.h>

#include "3rd-party/catch.hpp"
#include "test_helpers/chmod.h"
#include "test_helpers/misc.h"
#include "test_helpers/tempfile.h"

using namespace newsboat;

TEST_CASE("URL reader remembers the file name from which it read the URLs",
	"[FileUrlReader]")
{
	const std::string url("data/test-urls.txt");

	FileUrlReader u(url);
	REQUIRE(u.get_source() == url);
	u.reload();
	REQUIRE(u.get_source() == url);
}

TEST_CASE("URL reader extracts all URLs from the file", "[FileUrlReader]")
{
	FileUrlReader u("data/test-urls.txt");
	u.reload();

	REQUIRE(u.get_urls().size() == 3);
	REQUIRE(u.get_urls()[0] == "http://test1.url.cc/feed.xml");
	REQUIRE(u.get_urls()[1] == "http://anotherfeed.com/");
	REQUIRE(u.get_urls()[2] == "http://onemorefeed.at/feed/");
}

TEST_CASE("URL reader extracts feeds' tags", "[FileUrlReader]")
{
	FileUrlReader u("data/test-urls.txt");
	u.reload();

	REQUIRE(u.get_tags("http://test1.url.cc/feed.xml").size() == 3);
	REQUIRE(u.get_tags("http://test1.url.cc/feed.xml")[0] == "~title");
	REQUIRE(u.get_tags("http://test1.url.cc/feed.xml")[1] == "tag1");
	REQUIRE(u.get_tags("http://test1.url.cc/feed.xml")[2] == "tag2");

	REQUIRE(u.get_tags("http://anotherfeed.com/").size() == 0);

	REQUIRE(u.get_tags("http://onemorefeed.at/feed/").size() == 3);
	REQUIRE(u.get_tags("http://onemorefeed.at/feed/")[0] == "tag1");
	REQUIRE(u.get_tags("http://onemorefeed.at/feed/")[1] == "~another title");
	REQUIRE(u.get_tags("http://onemorefeed.at/feed/")[2] == "tag3");
}

TEST_CASE("URL reader keeps track of unique tags", "[FileUrlReader]")
{
	FileUrlReader u("data/test-urls.txt");
	u.reload();

	REQUIRE(u.get_alltags().size() == 3);
}

TEST_CASE("URL reader writes files that it can understand later",
	"[FileUrlReader]")
{
	const std::string testDataPath("data/test-urls.txt");
	test_helpers::TempFile urlsFile;

	test_helpers::copy_file(testDataPath, urlsFile.get_path());

	FileUrlReader u(urlsFile.get_path());
	u.reload();
	REQUIRE_FALSE(u.get_urls().empty());
	REQUIRE_FALSE(u.get_alltags().empty());

	std::ofstream urlsFileStream(urlsFile.get_path());
	REQUIRE(urlsFileStream.is_open());
	urlsFileStream << std::string();
	urlsFileStream.close();

	u.write_config();

	FileUrlReader u2(urlsFile.get_path());
	u2.reload();
	REQUIRE_FALSE(u2.get_urls().empty());
	REQUIRE_FALSE(u2.get_alltags().empty());
	REQUIRE(u.get_alltags() == u2.get_alltags());
	REQUIRE(u.get_urls() == u2.get_urls());
	for (const auto& url : u.get_urls()) {
		REQUIRE(u.get_tags(url) == u2.get_tags(url));
	}
}

TEST_CASE("Preserves URLs as-is", "[FileUrlReader][issue926]")
{
	const std::string testDataPath("data/926-urls");

	FileUrlReader u(testDataPath);
	u.reload();

	REQUIRE(u.get_urls().size() == 1);
	REQUIRE(u.get_urls()[0] ==
		R"_(exec:curl --silent https://feeds.metaebene.me/raumzeit/m4a  | sed 's#\(</guid>\|</id>\)#-M4A&#')_");
}

TEST_CASE("URL reader returns error structure if file cannot be opened",
	"[FileUrlReader]")
{
	const std::string testDataPath("data/test-urls.txt");

	test_helpers::TempFile urlsFile;
	FileUrlReader u(urlsFile.get_path());

	SECTION("reload() returns error structure if file does not exist") {
		const auto error_message = u.reload();
		REQUIRE(error_message.has_value());
		SECTION("the error structure contains error message and kind of an error") {
			INFO("error_message: " + error_message.value().message);
			REQUIRE_FALSE(error_message.value().message.empty());
		}
	}

	SECTION("write_config() works fine if file does not exist") {
		const auto error_message = u.write_config();
		REQUIRE_FALSE(error_message.has_value());

		SECTION("after writing file, reload() succeeds") {
			const auto error_message = u.reload();
			REQUIRE_FALSE(error_message.has_value());
		}
	}

	test_helpers::copy_file(testDataPath, urlsFile.get_path());

	SECTION("reload() succeeds if url file exists") {
		const auto error_message = u.reload();
		REQUIRE_FALSE(error_message.has_value());
	}

	GIVEN("that the urls file is not readable") {
		test_helpers::Chmod notReadable(urlsFile.get_path(), S_IWUSR);

		THEN("reload() returns an error message") {
			const auto error_message = u.reload();
			REQUIRE(error_message.has_value());
		}

		THEN("write_config() still works fine") {
			const auto error_message = u.write_config();
			REQUIRE_FALSE(error_message.has_value());
		}
	}

	GIVEN("that the urls file is not writable") {
		test_helpers::Chmod notWritable(urlsFile.get_path(), S_IRUSR);

		THEN("write_config() returns an error message") {
			const auto error_message = u.write_config();
			REQUIRE(error_message.has_value());

			SECTION("the error message contains the filename") {
				INFO("error_message: " + error_message.value());
				REQUIRE(error_message.value().find(urlsFile.get_path()) != std::string::npos);
			}
		}

		THEN("reload() still work fine") {
			const auto error_message = u.reload();
			REQUIRE_FALSE(error_message.has_value());
		}
	}
}

TEST_CASE("URL reader returns error message if file contains invalid UTF-8 codepoint",
	"[FileUrlReader]")
{
	test_helpers::TempFile urlsFile;

	{
		std::ofstream f(urlsFile.get_path());
		f << "http://exmample.com/atom.xml" << std::endl;
		f << "http://invalid.com/\xff.xml" << std::endl;
	}

	FileUrlReader u(urlsFile.get_path());
	auto error_message = u.reload();
	REQUIRE(error_message.has_value());
	REQUIRE(error_message.value().message.size() > 0);
}

TEST_CASE("FileUrlReader::get_alltags() returns all unique tags across all feeds, "
	"excluding the title tags",
	"[FileUrlReader]")
{
	FileUrlReader u("data/test-urls.txt");
	u.reload();

	const auto tags = u.get_alltags();
	REQUIRE(tags.size() == 3);
	REQUIRE(tags[0] == "tag1");
	REQUIRE(tags[1] == "tag2");
	REQUIRE(tags[2] == "tag3");
}

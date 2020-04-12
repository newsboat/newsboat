#include "fileurlreader.h"

#include <fstream>
#include <map>
#include <unistd.h>

#include "3rd-party/catch.hpp"
#include "test-helpers/chmod.h"
#include "test-helpers/misc.h"
#include "test-helpers/tempfile.h"

using namespace newsboat;

TEST_CASE("URL reader remembers the file name from which it read the URLs",
	"[FileUrlReader]")
{
	const std::string url("data/test-urls.txt");

	FileUrlReader u;
	REQUIRE(u.get_source() != url);
	u.load_config(url);
	REQUIRE(u.get_source() == url);
}

TEST_CASE("URL reader extracts all URLs from the file", "[FileUrlReader]")
{
	FileUrlReader u;
	u.load_config("data/test-urls.txt");

	REQUIRE(u.get_urls().size() == 3);
	REQUIRE(u.get_urls()[0] == "http://test1.url.cc/feed.xml");
	REQUIRE(u.get_urls()[1] == "http://anotherfeed.com/");
	REQUIRE(u.get_urls()[2] == "http://onemorefeed.at/feed/");
}

TEST_CASE("URL reader extracts feeds' tags", "[FileUrlReader]")
{
	FileUrlReader u;
	u.load_config("data/test-urls.txt");

	REQUIRE(u.get_tags("http://test1.url.cc/feed.xml").size() == 2);
	REQUIRE(u.get_tags("http://test1.url.cc/feed.xml")[0] == "tag1");
	REQUIRE(u.get_tags("http://test1.url.cc/feed.xml")[1] == "tag2");

	REQUIRE(u.get_tags("http://anotherfeed.com/").size() == 0);
	REQUIRE(u.get_tags("http://onemorefeed.at/feed/").size() == 2);
}

TEST_CASE("URL reader keeps track of unique tags", "[FileUrlReader]")
{
	FileUrlReader u;
	u.load_config("data/test-urls.txt");

	REQUIRE(u.get_alltags().size() == 3);
}

TEST_CASE("URL reader writes files that it can understand later",
	"[FileUrlReader]")
{
	const std::string testDataPath("data/test-urls.txt");
	TestHelpers::TempFile urlsFile;

	TestHelpers::copy_file(testDataPath, urlsFile.get_path());

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

TEST_CASE("URL reader throws exception if file cannot be opened",
	"[FileUrlReader]")
{
	const std::string testDataPath("data/test-urls.txt");
	TestHelpers::TempFile urlsFile;

	FileUrlReader u(urlsFile.get_path());

	SECTION("reload() and load_config() throw if file does not exist") {
		REQUIRE_THROWS_AS(u.reload(), std::exception);
		REQUIRE_THROWS_AS(u.load_config(urlsFile.get_path()), std::exception);
	}

	SECTION("write_config() works fine if file does not exist") {
		REQUIRE_NOTHROW(u.write_config());

		SECTION("after writing file, reload() and load_config() succeed") {
			REQUIRE_NOTHROW(u.reload());
			REQUIRE_NOTHROW(u.load_config(urlsFile.get_path()));
		}
	}

	TestHelpers::copy_file(testDataPath, urlsFile.get_path());

	SECTION("reload() and load_config() succeed if url file exists") {
		REQUIRE_NOTHROW(u.reload());
		REQUIRE_NOTHROW(u.load_config(urlsFile.get_path()));
	}

	GIVEN("that the urls file is not readable") {
		TestHelpers::Chmod notReadable(urlsFile.get_path(), S_IWUSR);

		THEN("reload() and load_config() throw") {
			REQUIRE_THROWS_AS(u.reload(), std::exception);
			REQUIRE_THROWS_AS(u.load_config(urlsFile.get_path()), std::exception);
		}

		THEN("write_config() still works fine") {
			REQUIRE_NOTHROW(u.write_config());
		}
	}

	GIVEN("that the urls file is not writable") {
		TestHelpers::Chmod notWritable(urlsFile.get_path(), S_IRUSR);

		THEN("write_config() throws") {
			REQUIRE_THROWS_AS(u.write_config(), std::exception);
		}

		THEN("reload() and load_config() still work fine") {
			REQUIRE_NOTHROW(u.reload());
			REQUIRE_NOTHROW(u.load_config(urlsFile.get_path()));
		}
	}
}

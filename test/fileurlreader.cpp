#include "fileurlreader.h"

#include <unistd.h>
#include <map>

#include "3rd-party/catch.hpp"
#include "test-helpers.h"

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

	std::ofstream urlsFileStream;
	urlsFileStream.open(urlsFile.getPath());
	REQUIRE(urlsFileStream.is_open());

	std::ifstream testData;
	testData.open(testDataPath);
	REQUIRE(testData.is_open());

	for (std::string line; std::getline(testData, line); ) {
		urlsFileStream << line << '\n';
	}

	urlsFileStream.close();
	testData.close();

	FileUrlReader u(urlsFile.getPath());
	u.reload();
	REQUIRE_FALSE(u.get_urls().empty());
	REQUIRE_FALSE(u.get_alltags().empty());

	urlsFileStream.open(urlsFile.getPath());
	REQUIRE(urlsFileStream.is_open());
	urlsFileStream << std::string();
	urlsFileStream.close();

	u.write_config();

	FileUrlReader u2(urlsFile.getPath());
	u2.reload();
	REQUIRE_FALSE(u2.get_urls().empty());
	REQUIRE_FALSE(u2.get_alltags().empty());
	REQUIRE(u.get_alltags() == u2.get_alltags());
	REQUIRE(u.get_urls() == u2.get_urls());
	for (const auto& url : u.get_urls()) {
		REQUIRE(u.get_tags(url) == u2.get_tags(url));
	}
}

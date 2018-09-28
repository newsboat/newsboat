#include "urlreader.h"

#include <unistd.h>
#include <map>

#include "3rd-party/catch.hpp"
#include "test-helpers.h"

using namespace newsboat;

TEST_CASE("OPML URL reader gets the path to input file from \"opml-url\" "
		"setting", "[opml_urlreader]")
{
	configcontainer cfg;
	opml_urlreader reader(&cfg);

	const std::string setting("opml-url");
	const std::string url1("https://example.com/feeds.opml");

	cfg.set_configvalue(setting, url1);
	REQUIRE(reader.get_source() == url1);

	const std::string url2("http://www.example.com/~henry/subscriptions.xml");
	cfg.set_configvalue(setting, url2);
	REQUIRE(reader.get_source() == url2);
}

TEST_CASE("opml_urlreader::reload() reads URLs and tags from an OPML file",
		"[opml_urlreader]")
{
	const std::string cwd(::getcwd(nullptr, 0));

	configcontainer cfg;
	cfg.set_configvalue("opml-url", "file://" + cwd + "/data/example.opml");

	opml_urlreader reader(&cfg);

	REQUIRE_NOTHROW(reader.reload());

	using URL = std::string;
	using Tag = std::string;
	using Tags = std::vector<Tag>;
	const std::map<URL, Tags> expected {
		{"https://example.com/feed.xml", {}},
		{"https://example.com/mirrors/distrowatch.rss", {}},
		{"https://example.com/feed.atom", {}},
		{"https://example.com/feed.rss09", {}},
		{"https://blogs.example.com/~john/posts.rss", {"Blogs"}},
		{"https://fred.example.com/writing/index.php?type=rss", {"eloquent"}},
		{"https://blogs.example.com/~mike/.rss", {"Blogs/friends"}},
	};

	REQUIRE(reader.get_urls().size() == expected.size());

	for (const auto& url : reader.get_urls()) {
		INFO("url = " << url);

		const auto e = expected.find(url);
		REQUIRE(e != expected.cend());

		const auto tags = reader.get_tags(url);
		REQUIRE(tags == e->second);
	}

	std::set<Tag> expected_tags;
	for (const auto& entry : expected) {
		expected_tags.insert(
				entry.second.cbegin(),
				entry.second.cend());
	}
	std::set<Tag> tags;
	const auto alltags = reader.get_alltags();
	tags.insert(alltags.cbegin(), alltags.cend());
	REQUIRE(tags == expected_tags);
}

TEST_CASE("opml_urlreader::reload() loads URLs from multiple sources",
		"[opml_urlreader]")
{
	const std::string cwd(::getcwd(nullptr, 0));

	configcontainer cfg;
	cfg.set_configvalue("opml-url",
			"file://" + cwd + "/data/example.opml"
			+ " "
			+ "file://" + cwd + "/data/example2.opml");

	opml_urlreader reader(&cfg);

	REQUIRE_NOTHROW(reader.reload());

	using URL = std::string;
	using Tag = std::string;
	using Tags = std::vector<Tag>;
	const std::map<URL, Tags> expected {
		{"https://example.com/feed.xml", {}},
		{"https://example.com/mirrors/distrowatch.rss", {}},
		{"https://example.com/feed.atom", {}},
		{"https://example.com/feed.rss09", {}},
		{"https://blogs.example.com/~john/posts.rss", {"Blogs"}},
		{"https://fred.example.com/writing/index.php?type=rss", {"eloquent"}},
		{"https://blogs.example.com/~mike/.rss", {"Blogs/friends"}},
		{"https://one.example.com/file.xml", {}},
		{"https://to.example.com/elves/tidings.rss", {}},
		{"https://third.example.com/~of/internet.rss", {"Rant boards/humans"}},
        {"https://four.example.com/~john/posts.rss", {"Rant boards"}},
		{"https://example.com/five.atom", {}},
		{"https://freddie.example.com/ritin/index.pl?format=rss", {"eloquent"}},
		{"https://example.com/feed2.rss09", {}},
	};

	REQUIRE(reader.get_urls().size() == expected.size());

	for (const auto& url : reader.get_urls()) {
		INFO("url = " << url);

		const auto e = expected.find(url);
		REQUIRE(e != expected.cend());

		const auto tags = reader.get_tags(url);
		REQUIRE(tags == e->second);
	}

	std::set<Tag> expected_tags;
	for (const auto& entry : expected) {
		expected_tags.insert(
				entry.second.cbegin(),
				entry.second.cend());
	}
	std::set<Tag> tags;
	const auto alltags = reader.get_alltags();
	tags.insert(alltags.cbegin(), alltags.cend());
	REQUIRE(tags == expected_tags);
}

TEST_CASE("opml_urlreader::reload() skips things that can't be parsed",
		"[opml_urlreader]")
{
	const std::string cwd(::getcwd(nullptr, 0));

	configcontainer cfg;
	cfg.set_configvalue("opml-url",
			"file://" + cwd + "/data/example.opml"
			+ " "
			+ "file:///dev/null" // empty file
			+ " "
			+ "file://" + cwd + "/data/guaranteed-not-to-exist.xml"
			+ " "
			+ "file://" + cwd + "/data/example2.opml");

	opml_urlreader reader(&cfg);

	REQUIRE_NOTHROW(reader.reload());

	using URL = std::string;
	using Tag = std::string;
	using Tags = std::vector<Tag>;
	const std::map<URL, Tags> expected {
		{"https://example.com/feed.xml", {}},
		{"https://example.com/mirrors/distrowatch.rss", {}},
		{"https://example.com/feed.atom", {}},
		{"https://example.com/feed.rss09", {}},
		{"https://blogs.example.com/~john/posts.rss", {"Blogs"}},
		{"https://fred.example.com/writing/index.php?type=rss", {"eloquent"}},
		{"https://blogs.example.com/~mike/.rss", {"Blogs/friends"}},
		{"https://one.example.com/file.xml", {}},
		{"https://to.example.com/elves/tidings.rss", {}},
		{"https://third.example.com/~of/internet.rss", {"Rant boards/humans"}},
        {"https://four.example.com/~john/posts.rss", {"Rant boards"}},
		{"https://example.com/five.atom", {}},
		{"https://freddie.example.com/ritin/index.pl?format=rss", {"eloquent"}},
		{"https://example.com/feed2.rss09", {}},
	};

	REQUIRE(reader.get_urls().size() == expected.size());

	for (const auto& url : reader.get_urls()) {
		INFO("url = " << url);

		const auto e = expected.find(url);
		REQUIRE(e != expected.cend());

		const auto tags = reader.get_tags(url);
		REQUIRE(tags == e->second);
	}

	std::set<Tag> expected_tags;
	for (const auto& entry : expected) {
		expected_tags.insert(
				entry.second.cbegin(),
				entry.second.cend());
	}
	std::set<Tag> tags;
	const auto alltags = reader.get_alltags();
	tags.insert(alltags.cbegin(), alltags.cend());
	REQUIRE(tags == expected_tags);
}

TEST_CASE("opml_urlreader::write_config() doesn't change the input file",
		"[opml_urlreader]")
{
	const std::string testDataPath("data/example.opml");

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

	configcontainer cfg;
	cfg.set_configvalue("opml-url", "file://" + urlsFile.getPath());

	opml_urlreader u(&cfg);
	REQUIRE_NOTHROW(u.reload());

	const std::string sentry("wasn't touched by opml_urlreader at all");
	urlsFileStream.open(urlsFile.getPath());
	REQUIRE(urlsFileStream.is_open());
	urlsFileStream << sentry;
	urlsFileStream.close();

	REQUIRE_NOTHROW(u.write_config());

	std::ifstream urlsFileReadStream;
	urlsFileReadStream.open(urlsFile.getPath());
	REQUIRE(urlsFileReadStream.is_open());
	std::string line;
	std::getline(urlsFileReadStream, line);
	REQUIRE(line == sentry);
}

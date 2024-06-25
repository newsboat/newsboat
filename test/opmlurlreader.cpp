#define ENABLE_IMPLICIT_FILEPATH_CONVERSIONS

#include "opmlurlreader.h"

#include <map>
#include <unistd.h>

#include "3rd-party/catch.hpp"
#include "utils.h"

using namespace newsboat;

TEST_CASE("OPML URL reader gets the path to input file from \"opml-url\" "
	"setting", "[OpmlUrlReader]")
{
	ConfigContainer cfg;
	OpmlUrlReader reader(cfg, "");

	const std::string setting("opml-url");
	const std::string url1("https://example.com/feeds.opml");

	cfg.set_configvalue(setting, url1);
	REQUIRE(reader.get_source() == url1);

	const std::string url2("http://www.example.com/~henry/subscriptions.xml");
	cfg.set_configvalue(setting, url2);
	REQUIRE(reader.get_source() == url2);
}

TEST_CASE("OpmlUrlReader::reload() reads URLs and tags from an OPML file",
	"[OpmlUrlReader]")
{
	ConfigContainer cfg;
	cfg.set_configvalue(
		"opml-url",
		"file://" + utils::getcwd().to_locale_string() + "/data/example.opml");

	OpmlUrlReader reader(cfg, "");

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

TEST_CASE("OpmlUrlReader::reload() loads URLs from multiple sources",
	"[OpmlUrlReader]")
{
	const std::string cwd = utils::getcwd();

	ConfigContainer cfg;
	cfg.set_configvalue("opml-url",
		"file://" + cwd + "/data/example.opml"
		+ " "
		+ "file://" + cwd + "/data/example2.opml");

	OpmlUrlReader reader(cfg, "");

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

TEST_CASE("OpmlUrlReader::reload() skips things that can't be parsed",
	"[OpmlUrlReader]")
{
	const std::string cwd = utils::getcwd();

	ConfigContainer cfg;
	cfg.set_configvalue("opml-url",
		"file://" + cwd + "/data/example.opml"
		+ " "
		+ "file:///dev/null" // empty file
		+ " "
		+ "file://" + cwd + "/data/guaranteed-not-to-exist.xml"
		+ " "
		+ "file://" + cwd + "/data/example2.opml");

	OpmlUrlReader reader(cfg, "");

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

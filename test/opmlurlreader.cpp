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
	OpmlUrlReader reader(cfg, Filepath());

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
	const auto cwd = utils::getcwd();

	ConfigContainer cfg;
	cfg.set_configvalue(
		"opml-url",
		"file://" + cwd.to_locale_string() + "/data/example.opml"
		+ " "
		+ "file://" + cwd.to_locale_string() + "/data/category.opml");

	OpmlUrlReader reader(cfg, Filepath());

	REQUIRE_NOTHROW(reader.reload());

	using URL = std::string;
	using Tag = std::string;
	using Tags = std::vector<Tag>;
	const std::map<URL, Tags> expected {
		{"https://example.com/feed.xml", {"~example.com website"}},
		{"https://example.com/mirrors/distrowatch.rss", {"~Distrowatch mirror"}},
		{"https://example.com/feed.atom", {"~example.com website (Atom feed)"}},
		{"https://example.com/feed.rss09", {"~example.com website (RSS 0.9 feed)"}},
		{"https://blogs.example.com/~john/posts.rss", {"~John's musings", "Blogs"}},
		{"https://fred.example.com/writing/index.php?type=rss", {"~Fred on everything", "eloquent"}},
		{"https://blogs.example.com/~mike/.rss", {"~Notes by Mike", "Blogs/friends"}},
		{"http://feeds.bbci.co.uk/news/business/rss.xml?edition=int", {"tag one", "tag_two", "tag/three"}},
	};

	REQUIRE(reader.get_urls().size() == expected.size());

	for (const auto& feed_url : reader.get_urls()) {
		INFO("url = " << feed_url.url);

		const auto e = expected.find(feed_url.url);
		REQUIRE(e != expected.cend());

		const auto& tags = reader.get_entry(feed_url.url)->tags;
		REQUIRE(tags == e->second);
	}

	std::set<Tag> expected_tags;
	for (const auto& entry : expected) {
		for (const auto& tag : entry.second) {
			if (tag[0] != '~') {
				expected_tags.insert(tag);
			}
		}
	}
	std::set<Tag> tags;
	const auto alltags = reader.get_alltags();
	tags.insert(alltags.cbegin(), alltags.cend());
	REQUIRE(tags == expected_tags);
}

TEST_CASE("OpmlUrlReader::reload() loads URLs from multiple sources",
	"[OpmlUrlReader]")
{
	const auto cwd = utils::getcwd();

	ConfigContainer cfg;
	cfg.set_configvalue("opml-url",
		"file://" + cwd.to_locale_string() + "/data/example.opml"
		+ " "
		+ "file://" + cwd.to_locale_string() + "/data/example2.opml");

	OpmlUrlReader reader(cfg, Filepath());

	REQUIRE_NOTHROW(reader.reload());

	using URL = std::string;
	using Tag = std::string;
	using Tags = std::vector<Tag>;
	const std::map<URL, Tags> expected {
		{"https://example.com/feed.xml", {"~example.com website"}},
		{"https://example.com/mirrors/distrowatch.rss", {"~Distrowatch mirror"}},
		{"https://example.com/feed.atom", {"~example.com website (Atom feed)"}},
		{"https://example.com/feed.rss09", {"~example.com website (RSS 0.9 feed)"}},
		{"https://blogs.example.com/~john/posts.rss", {"~John's musings", "Blogs"}},
		{"https://fred.example.com/writing/index.php?type=rss", {"~Fred on everything", "eloquent"}},
		{"https://blogs.example.com/~mike/.rss", {"~Notes by Mike", "Blogs/friends"}},
		{"https://one.example.com/file.xml", {"~To rule them all"}},
		{"https://to.example.com/elves/tidings.rss", {"~Another file"}},
		{"https://third.example.com/~of/internet.rss", {"~Really?", "Rant boards/humans"}},
		{"https://four.example.com/~john/posts.rss", {"~and another", "Rant boards"}},
		{"https://example.com/five.atom", {"~another title"}},
		{"https://freddie.example.com/ritin/index.pl?format=rss", {"~freddie rite about fings", "eloquent"}},
		{"https://example.com/feed2.rss09", {"~an outdated feed"}},
	};

	REQUIRE(reader.get_urls().size() == expected.size());

	for (const auto& feed_url : reader.get_urls()) {
		INFO("url = " << feed_url.url);

		const auto e = expected.find(feed_url.url);
		REQUIRE(e != expected.cend());

		const auto& tags = reader.get_entry(feed_url.url)->tags;
		REQUIRE(tags == e->second);
	}

	std::set<Tag> expected_tags;
	for (const auto& entry : expected) {
		for (const auto& tag : entry.second) {
			if (tag[0] != '~') {
				expected_tags.insert(tag);
			}
		}
	}
	std::set<Tag> tags;
	const auto alltags = reader.get_alltags();
	tags.insert(alltags.cbegin(), alltags.cend());
	REQUIRE(tags == expected_tags);
}

TEST_CASE("OpmlUrlReader::reload() skips things that can't be parsed",
	"[OpmlUrlReader]")
{
	const auto cwd = utils::getcwd();

	ConfigContainer cfg;
	cfg.set_configvalue("opml-url",
		"file://" + cwd.to_locale_string() + "/data/example.opml"
		+ " "
		+ "file:///dev/null" // empty file
		+ " "
		+ "file://" + cwd.to_locale_string() + "/data/guaranteed-not-to-exist.xml"
		+ " "
		+ "file://" + cwd.to_locale_string() + "/data/example2.opml");

	OpmlUrlReader reader(cfg, Filepath());

	REQUIRE_NOTHROW(reader.reload());

	using URL = std::string;
	using Tag = std::string;
	using Tags = std::vector<Tag>;
	const std::map<URL, Tags> expected {
		{"https://example.com/feed.xml", {"~example.com website"}},
		{"https://example.com/mirrors/distrowatch.rss", {"~Distrowatch mirror"}},
		{"https://example.com/feed.atom", {"~example.com website (Atom feed)"}},
		{"https://example.com/feed.rss09", {"~example.com website (RSS 0.9 feed)"}},
		{"https://blogs.example.com/~john/posts.rss", {"~John's musings", "Blogs"}},
		{"https://fred.example.com/writing/index.php?type=rss", {"~Fred on everything", "eloquent"}},
		{"https://blogs.example.com/~mike/.rss", {"~Notes by Mike", "Blogs/friends"}},
		{"https://one.example.com/file.xml", {"~To rule them all"}},
		{"https://to.example.com/elves/tidings.rss", {"~Another file"}},
		{"https://third.example.com/~of/internet.rss", {"~Really?", "Rant boards/humans"}},
		{"https://four.example.com/~john/posts.rss", {"~and another", "Rant boards"}},
		{"https://example.com/five.atom", {"~another title"}},
		{"https://freddie.example.com/ritin/index.pl?format=rss", {"~freddie rite about fings", "eloquent"}},
		{"https://example.com/feed2.rss09", {"~an outdated feed"}},
	};

	REQUIRE(reader.get_urls().size() == expected.size());

	for (const auto& feed_url : reader.get_urls()) {
		INFO("url = " << feed_url.url);

		const auto e = expected.find(feed_url.url);
		REQUIRE(e != expected.cend());

		const auto& tags = reader.get_entry(feed_url.url)->tags;
		REQUIRE(tags == e->second);
	}

	std::set<Tag> expected_tags;
	for (const auto& entry : expected) {
		for (const auto& tag : entry.second) {
			if (tag[0] != '~') {
				expected_tags.insert(tag);
			}
		}
	}
	std::set<Tag> tags;
	const auto alltags = reader.get_alltags();
	tags.insert(alltags.cbegin(), alltags.cend());
	REQUIRE(tags == expected_tags);
}

TEST_CASE("reload() skips URLs that start with a pipe symbol (\"|\") instead of "
	"turning them into `exec:` URLs",
	"[OpmlUrlReader]")
{
	const auto cwd = utils::getcwd();

	ConfigContainer cfg;
	cfg.set_configvalue("opml-url", "file://" + cwd.to_locale_string() + "/data/piped.opml");

	OpmlUrlReader reader(cfg, Filepath());

	REQUIRE_NOTHROW(reader.reload());

	const auto actual_urls = reader.get_urls();
	REQUIRE(actual_urls.size() == 1);
	REQUIRE(actual_urls[0].url == "https://example.com/feed.atom");
	const std::set<std::string> expected_tags {
		"~example.com website (Atom feed)", "tagged"
	};
	const auto actual_tags = actual_urls[0].tags;
	REQUIRE(actual_tags.size() == expected_tags.size());
	for (const auto& tag : actual_tags) {
		REQUIRE(expected_tags.find(tag) != std::end(expected_tags));
	}

	const auto alltags = reader.get_alltags();
	REQUIRE(alltags == std::vector<std::string>({"tagged"}));
}

TEST_CASE("reload() ignores `filtercmd` attribute and adds the URL as a regular feed",
	"[OpmlUrlReader]")
{
	const auto cwd = utils::getcwd();

	ConfigContainer cfg;
	cfg.set_configvalue("opml-url",
		"file://" + cwd.to_locale_string() + "/data/filtered.opml");

	OpmlUrlReader reader(cfg, Filepath());

	REQUIRE_NOTHROW(reader.reload());

	using URL = std::string;
	using Tag = std::string;
	using Tags = std::vector<Tag>;
	const std::map<URL, Tags> expected {
		{"https://example.com/another_feed.atom", {"~example.com website (Atom feed)", "misc"}},
		{"https://example.com/firehose", {"~Firehose"}},
	};

	REQUIRE(reader.get_urls().size() == expected.size());

	for (const auto& feed : reader.get_urls()) {
		INFO("url = " << feed.url);

		const auto e = expected.find(feed.url);
		REQUIRE(e != expected.cend());

		REQUIRE(feed.tags == e->second);
	}

	std::set<Tag> expected_tags;
	for (const auto& entry : expected) {
		for (const auto& tag : entry.second) {
			if (tag[0] != '~') {
				expected_tags.insert(tag);
			}
		}
	}
	std::set<Tag> tags;
	const auto alltags = reader.get_alltags();
	tags.insert(alltags.cbegin(), alltags.cend());
	REQUIRE(tags == expected_tags);
}

TEST_CASE("reload() ignores exec: URLs", "[OpmlUrlReader]")
{
	const auto cwd = utils::getcwd();

	ConfigContainer cfg;
	cfg.set_configvalue("opml-url",
		"file://" + cwd.to_locale_string() + "/data/with_verbatim_exec_url.opml");

	OpmlUrlReader reader(cfg, Filepath());

	REQUIRE_NOTHROW(reader.reload());

	const auto urls = reader.get_urls();
	REQUIRE(urls.size() == 1);
	REQUIRE(urls[0].url == "https://example.com/feed.atom");
	const std::vector<std::string> expected_feed_tags({ "~Normal feed", "tagged" });
	REQUIRE(urls[0].tags == expected_feed_tags);
	const std::vector<std::string> expected_alltags({ "tagged" });
	REQUIRE(reader.get_alltags() == expected_alltags);
}

TEST_CASE("reload() ignores filter: URLs", "[OpmlUrlReader]")
{
	const auto cwd = utils::getcwd();

	ConfigContainer cfg;
	cfg.set_configvalue("opml-url",
		"file://" + cwd.to_locale_string() + "/data/with_verbatim_filter_url.opml");

	OpmlUrlReader reader(cfg, Filepath());

	REQUIRE_NOTHROW(reader.reload());

	const auto urls = reader.get_urls();
	REQUIRE(urls.size() == 1);
	REQUIRE(urls[0].url == "https://example.com/feed.atom");
	const std::vector<std::string> expected_feed_tags({ "~Normal feed", "tagged" });
	REQUIRE(urls[0].tags == expected_feed_tags);
	const std::vector<std::string> expected_alltags({ "tagged" });
	REQUIRE(reader.get_alltags() == expected_alltags);
}

TEST_CASE("reload() ignores query: URLs", "[OpmlUrlReader]")
{
	const auto cwd = utils::getcwd();

	ConfigContainer cfg;
	cfg.set_configvalue("opml-url",
		"file://" + cwd.to_locale_string() + "/data/with_verbatim_query_url.opml");

	OpmlUrlReader reader(cfg, Filepath());

	REQUIRE_NOTHROW(reader.reload());

	const auto urls = reader.get_urls();
	REQUIRE(urls.size() == 1);
	REQUIRE(urls[0].url == "https://example.com/feed.atom");
	const std::vector<std::string> expected_feed_tags({ "~Normal feed", "tagged" });
	REQUIRE(urls[0].tags == expected_feed_tags);
	const std::vector<std::string> expected_alltags({ "tagged" });
	REQUIRE(reader.get_alltags() == expected_alltags);
}

TEST_CASE("reload() ignores URLs with unsupported schemas", "[OpmlUrlReader]")
{
	const auto cwd = utils::getcwd();

	ConfigContainer cfg;
	cfg.set_configvalue("opml-url",
		"file://" + cwd.to_locale_string() + "/data/with_unsupported_schemas.opml");

	OpmlUrlReader reader(cfg, Filepath());

	REQUIRE_NOTHROW(reader.reload());

	const auto urls = reader.get_urls();
	REQUIRE(urls.size() == 1);
	REQUIRE(urls[0].url == "https://example.com/feed.atom");
	std::vector<std::string> expected_tags({ "~Normal feed" });
	REQUIRE(urls[0].tags == expected_tags);
	REQUIRE(reader.get_alltags().empty());
}

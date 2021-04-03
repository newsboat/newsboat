#include "opml.h"

#include <libxml/xmlsave.h>

#include "3rd-party/catch.hpp"
#include "cache.h"
#include "fileurlreader.h"
#include "rssfeed.h"
#include "test-helpers/misc.h"
#include "test-helpers/tempfile.h"

using namespace newsboat;

TEST_CASE("opml::generate creates an XML document with feed URLs in OPML format",
	"[Opml]")
{
	const auto check =
		[]
		(const FeedContainer& feedcontainer)
	-> std::string {
		xmlDocPtr opml = opml::generate(feedcontainer);

		xmlBufferPtr buffer = xmlBufferCreate();

		xmlSaveCtxtPtr context = xmlSaveToBuffer(buffer, nullptr, 0);
		xmlSaveDoc(context, opml);
		xmlSaveClose(context);

		xmlFreeDoc(opml);

		const std::string opmlText(
			reinterpret_cast<char*>(buffer->content),
			buffer->use);

		xmlBufferFree(buffer);
		buffer = nullptr;

		return opmlText;
	};

	SECTION("No feeds") {
		FeedContainer feeds;

		const std::string expectedOpmlText(
			"<?xml version=\"1.0\"?>\n"
			"<opml version=\"1.0\">"
			"<head><title>Newsboat - Exported Feeds</title></head>"
			"<body/></opml>\n");

		REQUIRE(check(feeds) == expectedOpmlText);
	}

	SECTION("A few feeds") {
		ConfigContainer cfg;
		Cache rsscache(":memory:", &cfg);
		FeedContainer feeds;

		std::shared_ptr<RssFeed> feed =
			std::make_shared<RssFeed>(&rsscache, "https://example.com/feed1.xml");
		feed->set_title("Feed 1");
		feed->set_link("https://example.com/feed1/");
		feeds.add_feed(std::move(feed));

		feed = std::make_shared<RssFeed>(&rsscache, "https://example.com/feed2.xml");
		feed->set_title("Feed 2");
		feed->set_link("https://example.com/feed2/");
		feeds.add_feed(std::move(feed));

		const std::string expectedOpmlText(
			"<?xml version=\"1.0\"?>\n"
			"<opml version=\"1.0\">"
			"<head><title>Newsboat - Exported Feeds</title></head>"
			"<body>"
			"<outline type=\"rss\" "
			"xmlUrl=\"https://example.com/feed1.xml\" "
			"htmlUrl=\"https://example.com/feed1/\" "
			"title=\"Feed 1\"/>"
			"<outline type=\"rss\" "
			"xmlUrl=\"https://example.com/feed2.xml\" "
			"htmlUrl=\"https://example.com/feed2/\" "
			"title=\"Feed 2\"/>"
			"</body>"
			"</opml>\n");

		REQUIRE(check(feeds) == expectedOpmlText);
	}
}

TEST_CASE("import() populates UrlReader with URLs from the OPML file", "[Opml]")
{
	TestHelpers::TempFile urlsFile;

	TestHelpers::copy_file("data/test-urls.txt", urlsFile.get_path());

	using URL = std::string;
	using Tag = std::string;
	using Tags = std::vector<Tag>;
	const std::map<URL, Tags> testUrls {
		{"http://test1.url.cc/feed.xml", {"~title", "tag1", "tag2"}},
		{"http://anotherfeed.com/", {}},
		{"http://onemorefeed.at/feed/", {"tag1", "~another title", "tag3"}}
	};

	FileUrlReader urlcfg(urlsFile.get_path());
	urlcfg.reload();

	REQUIRE(urlcfg.get_urls().size() == testUrls.size());

	for (const auto& url : urlcfg.get_urls()) {
		INFO("url = " << url);

		const auto entry = testUrls.find(url);
		REQUIRE(entry != testUrls.cend());

		const auto tags = urlcfg.get_tags(url);
		REQUIRE(tags == entry->second);
	}

	REQUIRE_NOTHROW(
		opml::import(
			"file://" + utils::getcwd() + "/data/example.opml",
			urlcfg));

	const std::map<URL, Tags> opmlUrls {
		{"https://example.com/feed.xml", {}},
		{"https://example.com/mirrors/distrowatch.rss", {}},
		{"https://example.com/feed.atom", {}},
		{"https://example.com/feed.rss09", {}},
		{"https://blogs.example.com/~john/posts.rss", {"Blogs"}},
		{"https://blogs.example.com/~mike/.rss", {"Blogs/friends"}},
		{"https://fred.example.com/writing/index.php?type=rss", {"eloquent"}},
	};

	std::map<URL, Tags> combinedUrls;
	combinedUrls.insert(testUrls.cbegin(), testUrls.cend());
	combinedUrls.insert(opmlUrls.cbegin(), opmlUrls.cend());

	REQUIRE(urlcfg.get_urls().size() == combinedUrls.size());

	for (const auto& url : urlcfg.get_urls()) {
		INFO("url = " << url);

		const auto entry = combinedUrls.find(url);
		REQUIRE(entry != testUrls.cend());

		const auto tags = urlcfg.get_tags(url);
		REQUIRE(tags == entry->second);
	}
}

TEST_CASE("import() turns URLs that start with a pipe symbol (\"|\") "
	"into `exec:` URLs (Liferea convention)", "[Opml]")
{
	TestHelpers::TempFile urlsFile;

	FileUrlReader urlcfg(urlsFile.get_path());
	urlcfg.reload();

	REQUIRE_NOTHROW(
		opml::import(
			"file://" + utils::getcwd() + "/data/piped.opml",
			urlcfg));

	using URL = std::string;
	using Tag = std::string;
	using Tags = std::vector<Tag>;
	const std::map<URL, Tags> opmlUrls {
		{"exec:~/fetch_tweets.py", {}},
		{"https://example.com/feed.atom", {"tagged"}},
	};

	REQUIRE(urlcfg.get_urls().size() == opmlUrls.size());

	for (const auto& url : urlcfg.get_urls()) {
		INFO("url = " << url);

		const auto entry = opmlUrls.find(url);
		REQUIRE(entry != opmlUrls.cend());

		const auto tags = urlcfg.get_tags(url);
		REQUIRE(tags == entry->second);
	}
}

TEST_CASE("import() turns \"filtercmd\" attribute into a `filter:` URL "
	"(appears to be Liferea convention)",
	"[Opml]")
{
	TestHelpers::TempFile urlsFile;

	FileUrlReader urlcfg(urlsFile.get_path());
	urlcfg.reload();

	REQUIRE_NOTHROW(
		opml::import(
			"file://" + utils::getcwd() + "/data/filtered.opml",
			urlcfg));

	using URL = std::string;
	using Tag = std::string;
	using Tags = std::vector<Tag>;
	const std::map<URL, Tags> opmlUrls {
		{"https://example.com/another_feed.atom", {"misc"}},
		{"filter:~/.bin/keep_interesting.pl:https://example.com/firehose", {}},
	};

	REQUIRE(urlcfg.get_urls().size() == opmlUrls.size());

	for (const auto& url : urlcfg.get_urls()) {
		INFO("url = " << url);

		const auto entry = opmlUrls.find(url);
		REQUIRE(entry != opmlUrls.cend());

		const auto tags = urlcfg.get_tags(url);
		REQUIRE(tags == entry->second);
	}
}

TEST_CASE("import() skips URLs that are already present in UrlReader",
	"[Opml]")
{
	TestHelpers::TempFile urlsFile;

	TestHelpers::copy_file("data/test-urls.txt", urlsFile.get_path());

	using URL = std::string;
	using Tag = std::string;
	using Tags = std::vector<Tag>;
	const std::map<URL, Tags> testUrls {
		{"http://test1.url.cc/feed.xml", {"~title", "tag1", "tag2"}},
		{"http://anotherfeed.com/", {}},
		{"http://onemorefeed.at/feed/", {"tag1", "~another title", "tag3"}}
	};

	FileUrlReader urlcfg(urlsFile.get_path());
	urlcfg.reload();

	REQUIRE(urlcfg.get_urls().size() == testUrls.size());

	for (const auto& url : urlcfg.get_urls()) {
		INFO("url = " << url);

		const auto entry = testUrls.find(url);
		REQUIRE(entry != testUrls.cend());

		const auto tags = urlcfg.get_tags(url);
		REQUIRE(tags == entry->second);
	}

	REQUIRE_NOTHROW(
		opml::import(
			"file://" + utils::getcwd() + "/data/test-urls+.opml",
			urlcfg));

	const std::map<URL, Tags> opmlUrls {
		{"https://example.com/another_feed.atom", {}},
	};

	std::map<URL, Tags> combinedUrls;
	combinedUrls.insert(testUrls.cbegin(), testUrls.cend());
	combinedUrls.insert(opmlUrls.cbegin(), opmlUrls.cend());

	REQUIRE(urlcfg.get_urls().size() == combinedUrls.size());

	for (const auto& url : urlcfg.get_urls()) {
		INFO("url = " << url);

		const auto entry = combinedUrls.find(url);
		REQUIRE(entry != testUrls.cend());

		const auto tags = urlcfg.get_tags(url);
		REQUIRE(tags == entry->second);
	}
}

// falls back to "url" if "xmlUrl" is absent

// skips an entry if xmlUrl/url is absent

// in tags, falls back to "title" if "text" is not available

// can exec and filter be nested? OPML:
// <outline
//		type="rss"
//		filtercmd="~/.bin/keep_interesting.pl"
//		xmlUrl="|~/.bin/fetch_tweets.py" />
// Urls file:
// filter:~/.bin/keep_interesing.pl:exec:~/.bin/fetch_tweets.py

// ignores <outline> without title/text

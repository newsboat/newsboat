#include "opml.h"

#include <libxml/xmlsave.h>
#include "cache.h"
#include "fileurlreader.h"

#include "3rd-party/catch.hpp"
#include "test-helpers.h"

using namespace newsboat;

TEST_CASE("generate_opml creates an XML document with feed URLs in OPML format",
		"[opml]")
{
	const auto check =
		[]
		(const FeedContainer& feedcontainer)
		-> std::string
		{
			xmlDocPtr opml = OPML::generate_opml(feedcontainer);

			xmlBufferPtr buffer = xmlBufferCreate();

			xmlSaveCtxtPtr context = xmlSaveToBuffer(buffer, nullptr, 0);
			xmlSaveDoc(context, opml);
			xmlSaveClose(context);

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
			"<head><title>newsboat - Exported Feeds</title></head>"
			"<body/></opml>\n");

		REQUIRE(check(feeds) == expectedOpmlText);
	}

	SECTION("A few feeds") {
		ConfigContainer cfg;
		Cache rsscache(":memory:", &cfg);
		FeedContainer feeds;

		std::shared_ptr<RssFeed> feed =
			std::make_shared<RssFeed>(&rsscache);
		feed->set_title("Feed 1");
		feed->set_link("https://example.com/feed1/");
		feed->set_rssurl("https://example.com/feed1.xml");
		feeds.add_feed(std::move(feed));

		feed = std::make_shared<RssFeed>(&rsscache);
		feed->set_title("Feed 2");
		feed->set_link("https://example.com/feed2/");
		feed->set_rssurl("https://example.com/feed2.xml");
		feeds.add_feed(std::move(feed));

		const std::string expectedOpmlText(
			"<?xml version=\"1.0\"?>\n"
			"<opml version=\"1.0\">"
			"<head><title>newsboat - Exported Feeds</title></head>"
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

TEST_CASE("import() populates urlreader with URLs from the OPML file", "[opml]")
{
	TestHelpers::TempFile urlsFile;

	std::ifstream urlsExampleFile;
	urlsExampleFile.open("data/test-urls.txt");
	REQUIRE(urlsExampleFile.is_open());

	std::ofstream urlsFileStream;
	urlsFileStream.open(urlsFile.getPath());
	REQUIRE(urlsFileStream.is_open());

	for (std::string line; std::getline(urlsExampleFile, line); ) {
		urlsFileStream << line << '\n';
	}

	urlsExampleFile.close();
	urlsFileStream.close();

	using URL = std::string;
	using Tag = std::string;
	using Tags = std::vector<Tag>;
	const std::map<URL, Tags> testUrls {
		{"http://test1.url.cc/feed.xml", {"tag1", "tag2"}},
		{"http://anotherfeed.com/", {}},
		{"http://onemorefeed.at/feed/", {"tag1", "tag3"}}
	};

	FileUrlReader urlcfg(urlsFile.getPath());
	urlcfg.reload();

	REQUIRE(urlcfg.get_urls().size() == testUrls.size());

	for (const auto& url : urlcfg.get_urls()) {
		INFO("url = " << url);

		const auto entry = testUrls.find(url);
		REQUIRE(entry != testUrls.cend());

		const auto tags = urlcfg.get_tags(url);
		REQUIRE(tags == entry->second);
	}

	const std::string cwd(::getcwd(nullptr, 0));
	REQUIRE_NOTHROW(
			OPML::import("file://" + cwd + "/data/example.opml", &urlcfg));

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
		"into `exec:` URLs (Liferea convention)", "[opml]")
{
	TestHelpers::TempFile urlsFile;

	FileUrlReader urlcfg(urlsFile.getPath());
	urlcfg.reload();

	const std::string cwd(::getcwd(nullptr, 0));
	REQUIRE_NOTHROW(
			OPML::import("file://" + cwd + "/data/piped.opml", &urlcfg));

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
		"[opml]")
{
	TestHelpers::TempFile urlsFile;

	FileUrlReader urlcfg(urlsFile.getPath());
	urlcfg.reload();

	const std::string cwd(::getcwd(nullptr, 0));
	REQUIRE_NOTHROW(
			OPML::import("file://" + cwd + "/data/filtered.opml", &urlcfg));

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

TEST_CASE("import() skips URLs that are already present in urlreader",
		"[opml]")
{
	TestHelpers::TempFile urlsFile;

	std::ifstream urlsExampleFile;
	urlsExampleFile.open("data/test-urls.txt");
	REQUIRE(urlsExampleFile.is_open());

	std::ofstream urlsFileStream;
	urlsFileStream.open(urlsFile.getPath());
	REQUIRE(urlsFileStream.is_open());

	for (std::string line; std::getline(urlsExampleFile, line); ) {
		urlsFileStream << line << '\n';
	}

	urlsExampleFile.close();
	urlsFileStream.close();

	using URL = std::string;
	using Tag = std::string;
	using Tags = std::vector<Tag>;
	const std::map<URL, Tags> testUrls {
		{"http://test1.url.cc/feed.xml", {"tag1", "tag2"}},
		{"http://anotherfeed.com/", {}},
		{"http://onemorefeed.at/feed/", {"tag1", "tag3"}}
	};

	FileUrlReader urlcfg(urlsFile.getPath());
	urlcfg.reload();

	REQUIRE(urlcfg.get_urls().size() == testUrls.size());

	for (const auto& url : urlcfg.get_urls()) {
		INFO("url = " << url);

		const auto entry = testUrls.find(url);
		REQUIRE(entry != testUrls.cend());

		const auto tags = urlcfg.get_tags(url);
		REQUIRE(tags == entry->second);
	}

	const std::string cwd(::getcwd(nullptr, 0));
	REQUIRE_NOTHROW(
			OPML::import("file://" + cwd + "/data/test-urls+.opml", &urlcfg));

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

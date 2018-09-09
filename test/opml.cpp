#include "opml.h"

#include <libxml/xmlsave.h>
#include "cache.h"

#include "3rd-party/catch.hpp"

using namespace newsboat;

TEST_CASE("prepare_opml creates an XML document with feed URLs in OPML format",
		"[opml]")
{
	const auto check =
		[]
		(const FeedContainer& feedcontainer)
		-> std::string
		{
			xmlDocPtr opml = OPML::prepare_opml(feedcontainer);

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
		configcontainer cfg;
		cache rsscache(":memory:", &cfg);
		FeedContainer feeds;

		std::shared_ptr<rss_feed> feed
			= std::make_shared<rss_feed>(&rsscache);
		feed->set_title("Feed 1");
		feed->set_link("https://example.com/feed1/");
		feed->set_rssurl("https://example.com/feed1.xml");
		feeds.add_feed(std::move(feed));

		feed = std::make_shared<rss_feed>(&rsscache);
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

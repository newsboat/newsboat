#include "cacheurlreader.h"

#include "3rd-party/catch.hpp"
#include "configcontainer.h"
#include "curlhandle.h"
#include "feedretriever.h"
#include "rssfeed.h"
#include "rssparser.h"
#include "fileurlreader.h"
#include "cacheapi.h"
#include "test_helpers/tempfile.h"

using namespace newsboat;

TEST_CASE("reload gets urls in both cache and file", "[CacheUrlReader]")
{
	auto cfg = std::make_unique<ConfigContainer>();
	auto rsscache = Cache::in_memory(*cfg);
	CurlHandle easyHandle;
	FeedRetriever feed_retriever(*cfg, *rsscache, easyHandle);
	test_helpers::TempFile dbfile;

	auto feedurl = "file://data/rss.xml";
	RssParser parser1(feedurl, *rsscache, *cfg, nullptr);
	auto feed = parser1.parse(feed_retriever.retrieve(feedurl));
	rsscache->externalize_rssfeed(*feed, false);

	CacheApi api = CacheApi(*cfg, rsscache.get());

	const auto url = "data/test-urls.txt"_path;
	FileUrlReader file_reader(url);

	CacheUrlReader url_reader(file_reader, &api);
	url_reader.reload();

	auto feeds = url_reader.get_urls();

	REQUIRE(feeds.size() == 4);

	REQUIRE(feeds[0].first == "http://test1.url.cc/feed.xml");
	REQUIRE(feeds[1].first == "http://anotherfeed.com/");
	REQUIRE(feeds[2].first == "http://onemorefeed.at/feed/");
	REQUIRE(feeds[3].first == "file://data/rss.xml");
}
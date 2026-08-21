#include "cacheapi.h"

#include "3rd-party/catch.hpp"
#include "configcontainer.h"
#include "curlhandle.h"
#include "feedretriever.h"
#include "rssfeed.h"
#include "rssparser.h"
#include "test_helpers/tempfile.h"

using namespace newsboat;

TEST_CASE("get_subscribed_urls gets all cached feeds", "[CacheApi]")
{
	auto cfg = std::make_unique<ConfigContainer>();
	auto rsscache = Cache::in_memory(*cfg);
	CurlHandle easyHandle;
	FeedRetriever feed_retriever(*cfg, *rsscache, easyHandle);
	test_helpers::TempFile dbfile;

	auto feedurl = "file://data/rss.xml";
	RssParser parser(feedurl, *rsscache, *cfg, nullptr);
	auto feed = parser.parse(feed_retriever.retrieve(feedurl));
	rsscache->externalize_rssfeed(*feed, false);

	CacheApi api(*cfg, rsscache.get());
	auto feeds = api.get_subscribed_urls();

	REQUIRE(feeds.size() == 1);
	REQUIRE(feeds[0].first == feedurl);
}
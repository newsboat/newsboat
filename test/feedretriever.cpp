#include "feedretriever.h"

#include "3rd-party/catch.hpp"
#include "cache.h"
#include "configcontainer.h"
#include "curlhandle.h"
#include "strprintf.h"
#include "test/test_helpers/httptestserver.h"
#include "test_helpers/misc.h"

using namespace newsboat;

TEST_CASE("Feed retriever retrieves feed successfully", "[FeedRetriever]")
{
	auto feed_xml = test_helpers::read_binary_file("data/atom10_1.xml");

	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	CurlHandle easyHandle;
	FeedRetriever feedRetriever(cfg, rsscache, easyHandle);

	auto& testServer = test_helpers::HttpTestServer::get_instance();
	auto endpointLifetime = testServer.add_endpoint("/feed", {}, 200, {
		{"content-type", "text/xml"},
	}, feed_xml);

	const auto address = testServer.get_address();
	const auto url = strprintf::fmt("http://%s/feed", address);

	const auto feed = feedRetriever.retrieve(url);
	REQUIRE(feed.items.size() == 3);
}

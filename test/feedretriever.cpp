#include "feedretriever.h"

#include "3rd-party/catch.hpp"
#include "cache.h"
#include "configcontainer.h"
#include "curlhandle.h"
#include "rssfeed.h"
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
	auto mockRegistration = testServer.add_endpoint("/feed", {}, 200, {
		{"content-type", "text/xml"},
	}, feed_xml);

	const auto address = testServer.get_address();
	const auto url = strprintf::fmt("http://%s/feed", address);

	const auto feed = feedRetriever.retrieve(url);
	REQUIRE(testServer.num_hits(mockRegistration) == 1);
	REQUIRE(feed.items.size() == 3);
}

TEST_CASE("Feed retriever adds header with etag info if available", "[FeedRetriever]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	CurlHandle easyHandle;
	FeedRetriever feedRetriever(cfg, rsscache, easyHandle);

	auto& testServer = test_helpers::HttpTestServer::get_instance();
	const auto address = testServer.get_address();
	const auto url = strprintf::fmt("http://%s/feed", address);

	auto feed_with_etag = std::make_shared<RssFeed>(&rsscache, url);
	rsscache.externalize_rssfeed(feed_with_etag, false);
	rsscache.update_lastmodified(url, {}, "some-random-etag");

	auto mockRegistration = testServer.add_endpoint("/feed", {
		{"If-None-Match", "some-random-etag"},
	}, 304, {
		{"content-type", "text/xml"},
	}, {});

	const auto feed = feedRetriever.retrieve(url);
	REQUIRE(testServer.num_hits(mockRegistration) == 1);
	REQUIRE(feed.items.size() == 0);
}

TEST_CASE("Feed retriever does not retry download on HTTP 304 (Not Modified)",
	"[FeedRetriever]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	CurlHandle easyHandle;
	FeedRetriever feedRetriever(cfg, rsscache, easyHandle);

	auto& testServer = test_helpers::HttpTestServer::get_instance();
	const auto address = testServer.get_address();
	const auto url = strprintf::fmt("http://%s/feed", address);

	auto feed_with_etag = std::make_shared<RssFeed>(&rsscache, url);
	rsscache.externalize_rssfeed(feed_with_etag, false);
	rsscache.update_lastmodified(url, {}, "some-random-etag");

	auto mockRegistration = testServer.add_endpoint("/feed", {
		{"If-None-Match", "some-random-etag"},
	}, 304, {
		{"content-type", "text/xml"},
	}, {});

	cfg.set_configvalue("download-retries", "5");

	const auto feed = feedRetriever.retrieve(url);
	// TODO: Fix behavior and update this test.
	// There should only be 1 request.
	// Added an assertion for the wrong amount to make sure we update this test when fixing the behavior.
	// See https://github.com/newsboat/newsboat/issues/2732
	REQUIRE(testServer.num_hits(mockRegistration) == 5);
	//REQUIRE(testServer.num_Hits(mockRegistration) == 1);
}

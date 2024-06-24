#include "feedretriever.h"

#include "3rd-party/catch.hpp"
#include "cache.h"
#include "configcontainer.h"
#include "curlhandle.h"
#include "rss/exception.h"
#include "rssfeed.h"
#include "strprintf.h"
#include "test/test_helpers/httptestserver.h"
#include "test_helpers/misc.h"
#include <cstdint>
#include <vector>

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

	REQUIRE_THROWS_AS(feedRetriever.retrieve(url), rsspp::NotModifiedException);
	REQUIRE(testServer.num_hits(mockRegistration) == 1);
}

TEST_CASE("Feed retriever retries download if no data is received",
	"[FeedRetriever]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	CurlHandle easyHandle;
	FeedRetriever feedRetriever(cfg, rsscache, easyHandle);

	auto& testServer = test_helpers::HttpTestServer::get_instance();
	const auto address = testServer.get_address();
	const auto url = strprintf::fmt("http://%s/feed", address);

	auto mockRegistration = testServer.add_endpoint("/feed", {}, 200, {
		{"content-type", "text/xml"},
	}, {});

	cfg.set_configvalue("download-retries", "3");

	feedRetriever.retrieve(url);
	REQUIRE(testServer.num_hits(mockRegistration) == 3);
}

TEST_CASE("Feed retriever does not retry download on HTTP 304 (Not Modified), 429 (Too Many Requests)",
	"[FeedRetriever]")
{
	auto http_status = GENERATE(as<std::uint16_t> {}, 304, 429);
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
	}, http_status, {
		{"content-type", "text/xml"},
	}, {});

	cfg.set_configvalue("download-retries", "5");

	if (http_status == 304) {
		REQUIRE_THROWS_AS(feedRetriever.retrieve(url), rsspp::NotModifiedException);
	} else if (http_status == 429) {
		REQUIRE_THROWS_AS(feedRetriever.retrieve(url), rsspp::Exception);
	}
	REQUIRE(testServer.num_hits(mockRegistration) == 1);
}

TEST_CASE("Feed retriever throws on HTTP error status codes", "[FeedRetriever]")
{
	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	CurlHandle easyHandle;
	FeedRetriever feedRetriever(cfg, rsscache, easyHandle);

	auto& testServer = test_helpers::HttpTestServer::get_instance();
	const auto address = testServer.get_address();
	const auto url = strprintf::fmt("http://%s/feed", address);

	SECTION("Client errors: HTTP status code 4xx") {
		auto mockRegistration = testServer.add_endpoint("/feed", {}, 404, {}, {});

		REQUIRE_THROWS_AS(feedRetriever.retrieve(url), rsspp::Exception);
		REQUIRE(testServer.num_hits(mockRegistration) == 1);
	}

	SECTION("Server errors: HTTP status code 5xx") {
		auto mockRegistration = testServer.add_endpoint("/feed", {}, 500, {}, {});

		REQUIRE_THROWS_AS(feedRetriever.retrieve(url), rsspp::Exception);
		REQUIRE(testServer.num_hits(mockRegistration) == 1);
	}
}

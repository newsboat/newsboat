#include "feedretriever.h"

#include <memory>
#include <string>
#include <vector>

#include "3rd-party/catch.hpp"
#include "cache.h"
#include "configcontainer.h"
#include "libnewsboat-ffi/src/http_test_server.rs.h"
#include "rssfeed.h"
#include "strprintf.h"
#include "test_helpers/misc.h"

using namespace newsboat;

namespace {
const std::vector<http_test_server::bridged::Header> default_response_headers = {
	http_test_server::bridged::Header {
		rust::String("content-type"),
		rust::String("text/xml"),
	},
};
}

TEST_CASE("Feed retriever retrieves feed successfully", "[FeedRetriever]")
{
	auto feed_xml = test_helpers::read_binary_file("data/atom10_1.xml");

	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	FeedRetriever feedRetriever(cfg, rsscache);

	auto server = http_test_server::bridged::create();
	const auto address = std::string(http_test_server::bridged::get_address(*server));
	const auto url = strprintf::fmt("http://%s/feed", address);

	rust::Slice<const uint8_t> body(feed_xml.data(), feed_xml.size());
	const auto mock_id = http_test_server::bridged::add_endpoint(
			*server,
			"/feed",
			{},
			200,
			rust::Slice<http_test_server::bridged::Header const>(default_response_headers.data(),
				default_response_headers.size()),
			body);

	const auto feed = feedRetriever.retrieve(url);
	http_test_server::bridged::assert_hits(*server, mock_id, 1);
	REQUIRE(feed.items.size() == 3);
}

TEST_CASE("Feed retriever adds header with etag info if available", "[FeedRetriever]")
{
	auto feed_xml = test_helpers::read_binary_file("data/atom10_1.xml");

	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	FeedRetriever feedRetriever(cfg, rsscache);

	auto server = http_test_server::bridged::create();
	const auto address = std::string(http_test_server::bridged::get_address(*server));
	const auto url = strprintf::fmt("http://%s/feed", address);

	auto feed_with_etag = std::make_shared<RssFeed>(&rsscache, url);
	rsscache.externalize_rssfeed(feed_with_etag, false);
	rsscache.update_lastmodified(url, {}, "some-random-etag");

	rust::Slice<const uint8_t> body(feed_xml.data(), feed_xml.size());
	std::vector<http_test_server::bridged::Header> expected_headers {
		http_test_server::bridged::Header { rust::String("If-None-Match"), rust::String("some-random-etag")},
	};
	const auto mock_id = http_test_server::bridged::add_endpoint(
			*server,
			"/feed",
			rust::Slice<http_test_server::bridged::Header const>(expected_headers.data(),
				expected_headers.size()),
			304,
			rust::Slice<http_test_server::bridged::Header const>(default_response_headers.data(),
				default_response_headers.size()),
			body);

	const auto feed = feedRetriever.retrieve(url);
	http_test_server::bridged::assert_hits(*server, mock_id, 1);
	REQUIRE(feed.items.size() == 0);
}

TEST_CASE("Feed retriever does not retry download on HTTP 304 (Not Modified)",
	"[FeedRetriever]")
{
	auto feed_xml = test_helpers::read_binary_file("data/atom10_1.xml");

	ConfigContainer cfg;
	Cache rsscache(":memory:", &cfg);
	FeedRetriever feedRetriever(cfg, rsscache);

	auto server = http_test_server::bridged::create();
	const auto address = std::string(http_test_server::bridged::get_address(*server));
	const auto url = strprintf::fmt("http://%s/feed", address);

	auto feed_with_etag = std::make_shared<RssFeed>(&rsscache, url);
	rsscache.externalize_rssfeed(feed_with_etag, false);
	rsscache.update_lastmodified(url, {}, "some-random-etag");

	rust::Slice<const uint8_t> body(feed_xml.data(), feed_xml.size());
	std::vector<http_test_server::bridged::Header> expected_headers {
		http_test_server::bridged::Header { rust::String("If-None-Match"), rust::String("some-random-etag")},
	};
	const auto mock_id = http_test_server::bridged::add_endpoint(
			*server,
			"/feed",
			rust::Slice<http_test_server::bridged::Header const>(expected_headers.data(),
				expected_headers.size()),
			304,
			rust::Slice<http_test_server::bridged::Header const>(default_response_headers.data(),
				default_response_headers.size()),
			body);

	cfg.set_configvalue("download-retries", "5");

	const auto feed = feedRetriever.retrieve(url);
	// TODO: Fix behavior and update this test.
	// There should only be 1 request.
	// Added an assertion for the wrong amount to make sure we update this test when fixing the behavior.
	// See https://github.com/newsboat/newsboat/issues/2732
	http_test_server::bridged::assert_hits(*server, mock_id, 5);
	//http_test_server::bridged::assert_hits(*server, mock_id, 1);
}

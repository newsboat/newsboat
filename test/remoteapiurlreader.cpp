#include "remoteapiurlreader.h"

#include "3rd-party/catch.hpp"
#include "configcontainer.h"
#include "remoteapi.h"
#include "test_helpers/tempfile.h"
#include <fstream>

using namespace newsboat;

class DummyRemoteApi : public RemoteApi {
public:
	DummyRemoteApi(ConfigContainer& cfg)
		: RemoteApi(cfg)
	{
	}

	void set_subscribed_urls(std::vector<TaggedFeedUrl> urls)
	{
		subscribed_urls = urls;
	};

	std::vector<TaggedFeedUrl> get_subscribed_urls() override
	{
		return subscribed_urls;
	};

	bool authenticate() override
	{
		return false;
	};

	void add_custom_headers(curl_slist**) override {};

	bool mark_all_read(const std::string&) override
	{
		return false;
	};

	bool mark_article_read(const std::string&, bool) override
	{
		return false;
	};

	bool update_article_flags(const std::string&, const std::string&,
		const std::string&) override
	{
		return false;
	};

private:
	std::vector<TaggedFeedUrl> subscribed_urls;
};

TEST_CASE("RemoteApiUrlReader has no urls configured by default", "[RemoteApiUrlReader]")
{
	ConfigContainer cfg;
	DummyRemoteApi remote_api(cfg);
	test_helpers::TempFile urls;
	RemoteApiUrlReader url_reader("dummy", urls.get_path(), remote_api);

	REQUIRE(url_reader.get_urls().size() == 0);
	REQUIRE(url_reader.get_alltags().size() == 0);
}

TEST_CASE("RemoteApiUrlReader reload includes query urls and urls from the RemoteApi",
	"[RemoteApiUrlReader]")
{
	ConfigContainer cfg;
	DummyRemoteApi remote_api(cfg);
	test_helpers::TempFile urls;
	RemoteApiUrlReader url_reader("dummy", urls.get_path(), remote_api);

	GIVEN("A remote API with configured URLs and a urls file with a query URL") {
		{
			std::ofstream urls_file(urls.get_path().to_locale_string());
			urls_file << "https://example.com/non-query.xml" << std::endl;
			urls_file << R"("query:Unread Articles:unread = \"yes\"" querytag ~querytitlerename)" <<
				std::endl;
		}

		const std::string feed1 = "https://example.com/remote-feed1.xml";
		const std::string feed2 = "https://example.com/remote-feed2.xml";

		remote_api.set_subscribed_urls({
			{ feed1, {"tag1", "tag2"} },
			{ feed2, {} },
		});

		WHEN("URLs are reloaded") {
			url_reader.reload();

			THEN("Both remote URLs and query URL are included including their tags") {
				const auto& loaded_urls = url_reader.get_urls();
				REQUIRE(loaded_urls.size() == 3);

				const std::string query_feed = R"(query:Unread Articles:unread = "yes")";
				REQUIRE(loaded_urls[0] == query_feed);
				REQUIRE(loaded_urls[1] == feed1);
				REQUIRE(loaded_urls[2] == feed2);

				REQUIRE(url_reader.get_tags(query_feed).size() == 2);
				REQUIRE(url_reader.get_tags(feed1).size() == 2);
				REQUIRE(url_reader.get_tags(feed2).size() == 0);
			}

			THEN("Alltags includes tags from both sources but excludes title rename tags") {
				const auto tags = url_reader.get_alltags();
				REQUIRE(tags.size() == 3);
				REQUIRE(std::set<std::string>(tags.begin(), tags.end()) == std::set<std::string> {"querytag", "tag1", "tag2"});
			}
		}
	}
}

#include "rssignores.h"

#include <set>

#include "3rd-party/catch.hpp"

#include "cache.h"
#include "confighandlerexception.h"
#include "rssitem.h"

using namespace newsboat;

TEST_CASE(
	"RssIgnores::matches_lastmodified() returns true if given url "
	"has to always be downloaded",
	"[RssIgnores]")
{
	RssIgnores ignores;
	ignores.handle_action("always-download", {
		"http://newsboat.org",
		"www.cool-website.com",
		"www.example.com"
	});

	REQUIRE(ignores.matches_lastmodified("www.example.com"));
	REQUIRE(ignores.matches_lastmodified("http://newsboat.org"));
	REQUIRE(ignores.matches_lastmodified("www.cool-website.com"));
	REQUIRE_FALSE(ignores.matches_lastmodified("www.smth.com"));
}

TEST_CASE(
	"RssIgnores::matches_resetunread() returns true if given url "
	"will always have its unread flag reset on update",
	"[RssIgnores]")
{
	RssIgnores ignores;
	ignores.handle_action("reset-unread-on-update", {
		"http://newsboat.org",
		"www.cool-website.com",
		"www.example.com"
	});

	REQUIRE(ignores.matches_resetunread("www.example.com"));
	REQUIRE(ignores.matches_resetunread("http://newsboat.org"));
	REQUIRE(ignores.matches_resetunread("www.cool-website.com"));
	REQUIRE_FALSE(ignores.matches_resetunread("www.smth.com"));
}

TEST_CASE("RssIgnores::handle_action() handles `ignore-article`",
	"[RssIgnores]")
{
	RssIgnores ignores;

	const std::string action = "ignore-article";

	SECTION("Throws ConfigHandlerException if less than 2 parameters") {
		REQUIRE_THROWS_AS(ignores.handle_action(action, {}), ConfigHandlerException);
		REQUIRE_THROWS_AS(ignores.handle_action(action, {"param"}),
			ConfigHandlerException);
	}

	SECTION("Throws ConfigHandlerException if second param can't be parsed as filter expression") {
		REQUIRE_THROWS_AS(ignores.handle_action(action, {"url", "!?"}),
			ConfigHandlerException);
	}

	SECTION("Doesn't throw on two or more params") {
		REQUIRE_NOTHROW(ignores.handle_action(action, {"url", "expression = 1", "more"}));
		REQUIRE_NOTHROW(ignores.handle_action(action, {"url", "expression = 1", "way", "more different", "parameters"}));
	}

	SECTION("Throws ConfigHandlerException if second param can't be parsed as a filter expression") {
		REQUIRE_THROWS_AS(ignores.handle_action("ignore-article", {"regex:a{1z", "author = \"John Doe\""}),
			ConfigHandlerException);
	}
}

TEST_CASE("RssIgnores::handle_action() handles `always-download`",
	"[RssIgnores]")
{
	RssIgnores ignores;

	const std::string action = "always-download";

	SECTION("Throws ConfigHandlerException if given zero parameters") {
		REQUIRE_THROWS_AS(ignores.handle_action(action, {}), ConfigHandlerException);
	}

	SECTION("Doesn't trow if given one or more parameters") {
		REQUIRE_NOTHROW(ignores.handle_action(action, {"url1"}));
		REQUIRE_NOTHROW(ignores.handle_action(action, {"url1", "url2"}));
		REQUIRE_NOTHROW(ignores.handle_action(action, {"url1", "url2", "url3", "url4", "url5"}));
	}
}

TEST_CASE("RssIgnores::handle_action() handles `reset-unread-on-update`",
	"[RssIgnores]")
{
	RssIgnores ignores;

	const std::string action = "reset-unread-on-update";

	SECTION("Throws ConfigHandlerException if given zero parameters") {
		REQUIRE_THROWS_AS(ignores.handle_action(action, {}), ConfigHandlerException);
	}

	SECTION("Doesn't throw if given one or more parameters") {
		REQUIRE_NOTHROW(ignores.handle_action(action, {"url1"}));
		REQUIRE_NOTHROW(ignores.handle_action(action, {"url1", "url2"}));
		REQUIRE_NOTHROW(ignores.handle_action(action, {"url1", "url2", "url3", "url4", "url5"}));
	}
}

TEST_CASE("RssIgnores::handle_action() throws ConfigHandlerException "
	"on unknown command",
	"[RssIgnores]")
{
	RssIgnores ignores;

	REQUIRE_THROWS_AS(ignores.handle_action("bind-key", {"ESC", "quit"}),
		ConfigHandlerException);
	REQUIRE_THROWS_AS(ignores.handle_action("auto-reload", {"yes"}),
		ConfigHandlerException);
	REQUIRE_THROWS_AS(ignores.handle_action("color", {"listnormal", "blue", "black"}),
		ConfigHandlerException);
}

TEST_CASE("RssIgnores::dump_config() writes out all configured settings "
	"to the provided vector",
	"[RssIgnores]")
{
	RssIgnores ignores;

	SECTION("`ignore-article`") {
		const std::string action = "ignore-article";

		ignores.handle_action(action, {"https://example.com/feed.xml", "author =~ \"Joe\""});
		ignores.handle_action(action, {"*", "title # \"interesting\""});
		ignores.handle_action(action, {"https://blog.example.com/joe/posts.xml", "guid # 123"});
		ignores.handle_action(action, {"regex:^https://.*", "author = \"John Doe\""});

		std::vector<std::string> config;
		const auto comment =
			"# Comment to check that RssIgnores::dump_config() doesn't clear the vector";
		config.push_back(comment);

		ignores.dump_config(config);

		std::set<std::string> config_set(config.begin(), config.end());

		REQUIRE(config.size() == 5); // four actions plus one comment
		REQUIRE(config_set.count(comment) == 1);
		REQUIRE(config_set.count(
				R"#(ignore-article "https://example.com/feed.xml" "author =~ \"Joe\"")#") == 1);
		REQUIRE(config_set.count(
				R"#(ignore-article * "title # \"interesting\"")#") == 1);
		REQUIRE(config_set.count(
				R"#(ignore-article "https://blog.example.com/joe/posts.xml" "guid # 123")#") == 1);
		REQUIRE(config_set.count(
				R"#(ignore-article "regex:^https://.*" "author = \"John Doe\"")#") == 1);
	}

	SECTION("`always-download`") {
		const std::string action = "always-download";

		ignores.handle_action(action, {"url1"});
		ignores.handle_action(action, {"url2", "url3", "url4"});

		std::vector<std::string> config;
		const auto comment =
			"# Comment to check that RssIgnores::dump_config() doesn't clear the vector";
		config.push_back(comment);

		ignores.dump_config(config);

		REQUIRE(config.size() == 5); // four URLs plus one comment
		REQUIRE(config[0] == comment);
		REQUIRE(config[1] == R"#(always-download "url1")#");
		REQUIRE(config[2] == R"#(always-download "url2")#");
		REQUIRE(config[3] == R"#(always-download "url3")#");
		REQUIRE(config[4] == R"#(always-download "url4")#");
	}

	SECTION("`reset-unread-on-update`") {
		const std::string action = "reset-unread-on-update";

		ignores.handle_action(action, {"url1"});
		ignores.handle_action(action, {"url2", "url3", "url4"});

		std::vector<std::string> config;
		const auto comment =
			"# Comment to check that RssIgnores::dump_config() doesn't clear the vector";
		config.push_back(comment);

		ignores.dump_config(config);

		REQUIRE(config.size() == 5); // four URLs plus one comment
		REQUIRE(config[0] == comment);
		REQUIRE(config[1] == R"#(reset-unread-on-update "url1")#");
		REQUIRE(config[2] == R"#(reset-unread-on-update "url2")#");
		REQUIRE(config[3] == R"#(reset-unread-on-update "url3")#");
		REQUIRE(config[4] == R"#(reset-unread-on-update "url4")#");
	}

	SECTION("Mix of all supported commands") {
		ignores.handle_action("reset-unread-on-update", {"url1"});
		ignores.handle_action("ignore-article", {"*", "title # \"interesting\""});
		ignores.handle_action("always-download", {"url1"});
		ignores.handle_action("ignore-article", {"https://blog.example.com/joe/posts.xml", "guid # 123"});
		ignores.handle_action("reset-unread-on-update", {"url2", "url3"});
		ignores.handle_action("always-download", {"url2", "url3", "url4"});

		std::vector<std::string> config;
		const auto comment =
			"# Comment to check that RssIgnores::dump_config() doesn't clear the vector";
		config.push_back(comment);

		ignores.dump_config(config);

		std::set<std::string> config_set(config.begin(), config.end());

		REQUIRE(config.size() == 10);
		REQUIRE(config[0] == comment);
		REQUIRE(config_set.count(
				R"#(ignore-article * "title # \"interesting\"")#") == 1);
		REQUIRE(config_set.count(
				R"#(ignore-article "https://blog.example.com/joe/posts.xml" "guid # 123")#") == 1);
		REQUIRE(config[3] == R"#(always-download "url1")#");
		REQUIRE(config[4] == R"#(always-download "url2")#");
		REQUIRE(config[5] == R"#(always-download "url3")#");
		REQUIRE(config[6] == R"#(always-download "url4")#");
		REQUIRE(config[7] == R"#(reset-unread-on-update "url1")#");
		REQUIRE(config[8] == R"#(reset-unread-on-update "url2")#");
		REQUIRE(config[9] == R"#(reset-unread-on-update "url3")#");
	}
}

TEST_CASE("RssIgnores::matches() returns true if given RssItem matches any "
	"of ignore-article rules, otherwise false",
	"[RssIgnores]")
{
	RssIgnores ignores;

	ConfigContainer cfg;
	auto rsscache = Cache::in_memory(cfg);
	RssItem item(rsscache.get());

	const auto feedurl = "https://example.com/feed.xml";

	item.set_title("Updates");
	item.set_author("John Doe");
	item.set_feedurl(feedurl);

	SECTION("Only rules associated with item's feed URL are applied") {
		SECTION("No rules for this feed") {
			ignores.handle_action("ignore-article", {"https://example.org/anotherfeed.xml", "title =~ \".*\""});

			REQUIRE_FALSE(ignores.matches(&item));
		}

		SECTION("One rule for feed, but doesn't match item") {
			ignores.handle_action("ignore-article", {feedurl, "title = \"news\""});

			REQUIRE_FALSE(ignores.matches(&item));
		}

		SECTION("A rule matches both the feed and the item") {
			ignores.handle_action("ignore-article", {feedurl, "title = \"Updates\""});

			REQUIRE(ignores.matches(&item));
		}

		SECTION("Multiple rules that match both the feed and the item") {
			ignores.handle_action("ignore-article", {feedurl, "title = \"Updates\""});
			ignores.handle_action("ignore-article", {feedurl, "author = \"John Doe\""});

			REQUIRE(ignores.matches(&item));
		}
	}

	SECTION("Pattern matching rules prefixed with \"regex:\"") {
		SECTION("Matching feeds starting with https://") {
			ignores.handle_action("ignore-article", {"regex:^https://.*", "author = \"John Doe\""});

			REQUIRE(ignores.matches(&item));
		}

		SECTION("Matching feeds containing example.com and ending with xml") {
			ignores.handle_action("ignore-article", {"regex:.*example.com/.*\\.xml", "author = \"John Doe\""});

			REQUIRE(ignores.matches(&item));
		}
	}

	SECTION("Rules with URL of \"*\" match any feed") {
		ignores.handle_action("ignore-article", {"*", "author = \"John Doe\""});

		REQUIRE(ignores.matches(&item));
	}
}

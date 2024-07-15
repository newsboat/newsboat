#include "configcontainer.h"

#include <unordered_set>

#include "3rd-party/catch.hpp"

#include "confighandlerexception.h"
#include "configparser.h"
#include "keymap.h"

using namespace newsboat;

TEST_CASE("Parses test config without exceptions", "[ConfigContainer]")
{
	ConfigContainer cfg;
	ConfigParser cfgparser;
	cfg.register_commands(cfgparser);
	KeyMap k(KM_NEWSBOAT);
	cfgparser.register_handler("macro", k);

	REQUIRE_NOTHROW(cfgparser.parse_file(
			Filepath::from_locale_string("data/test-config.txt")));

	SECTION("bool value") {
		REQUIRE(cfg.get_configvalue("show-read-feeds") == "no");
		REQUIRE_FALSE(cfg.get_configvalue_as_bool("show-read-feeds"));
	}

	SECTION("string value") {
		REQUIRE(cfg.get_configvalue("browser") == "firefox");
	}

	SECTION("integer value") {
		REQUIRE(cfg.get_configvalue("max-items") == "100");
		REQUIRE(cfg.get_configvalue_as_int("max-items") == 100);
	}

	SECTION("Tilde got expanded into path to user's home directory") {
		char* home = ::getenv("HOME");
		REQUIRE(home != nullptr);
		std::string cachefilecomp = home;
		cachefilecomp.append("/foo");
		REQUIRE(cfg.get_configvalue("cache-file") == cachefilecomp);
	}
}

TEST_CASE(
	"Parses test config correctly, even if there's no \\n at the end line.",
	"[ConfigContainer]")
{
	ConfigContainer cfg;
	ConfigParser cfgparser;
	cfg.register_commands(cfgparser);

	REQUIRE_NOTHROW(cfgparser.parse_file(
			Filepath::from_locale_string("data/test-config-without-newline-at-the-end.txt")));

	SECTION("first line") {
		REQUIRE(cfg.get_configvalue("browser") == "firefox");
	}

	SECTION("last line") {
		REQUIRE(cfg.get_configvalue("download-path") == "whatever");
	}
}

TEST_CASE("Throws if invalid command is encountered", "[ConfigContainer]")
{
	ConfigContainer cfg;

	CHECK_THROWS_AS(cfg.handle_action("command-that-surely-does-not-exist",
	{"and", "its", "arguments"}),
	ConfigHandlerException);
}

TEST_CASE("Throws if there are no arguments", "[ConfigContainer]")
{
	ConfigContainer cfg;

	CHECK_THROWS_AS(
		cfg.handle_action("auto-reload", {}), ConfigHandlerException);
}

TEST_CASE("Throws if command argument has invalid type", "[ConfigContainer]")
{
	ConfigContainer cfg;

	SECTION("bool") {
		CHECK_THROWS_AS(cfg.handle_action("always-display-description",
		{"whatever"}),
		ConfigHandlerException);
	}

	SECTION("int") {
		CHECK_THROWS_AS(
			cfg.handle_action("download-retries", {"whatever"}),
			ConfigHandlerException);
	}

	SECTION("enum") {
		CHECK_THROWS_AS(cfg.handle_action("proxy-type", {"whatever"}),
			ConfigHandlerException);
	}
}

TEST_CASE("Throws if there are too few arguments", "[ConfigContainer]")
{
	ConfigContainer cfg;

	CHECK_THROWS_AS(cfg.handle_action("browser", {}), ConfigHandlerException);
}

TEST_CASE("Throws if there are too many arguments", "[ConfigContainer]")
{
	ConfigContainer cfg;

	CHECK_THROWS_AS(cfg.handle_action("browser", {"xdg-open", "%u"}), ConfigHandlerException);
}

TEST_CASE("reset_to_default changes setting to its default value",
	"[ConfigContainer]")
{
	ConfigContainer cfg;

	const std::string default_value = "any";
	const std::vector<std::string> tests{"any",
		"basic",
		"digest",
		"digest_ie",
		"gssnegotiate",
		"ntlm",
		"anysafe"};
	const std::string key("http-auth-method");

	REQUIRE(cfg.get_configvalue(key) == default_value);

	for (const std::string& test_value : tests) {
		cfg.set_configvalue(key, test_value);
		REQUIRE(cfg.get_configvalue(key) == test_value);
		REQUIRE_NOTHROW(cfg.reset_to_default(key));
		REQUIRE(cfg.get_configvalue(key) == default_value);
	}
}

TEST_CASE("get_configvalue() returns empty string if settings doesn't exist",
	"[ConfigContainer]")
{
	ConfigContainer cfg;
	REQUIRE(cfg.get_configvalue("nonexistent-key") == "");
}

TEST_CASE("get_configvalue_as_bool() recognizes several boolean formats",
	"[ConfigContainer]")
{
	ConfigContainer cfg;
	cfg.set_configvalue("cleanup-on-quit", "yes");
	cfg.set_configvalue("auto-reload", "true");
	cfg.set_configvalue("show-read-feeds", "no");
	cfg.set_configvalue("bookmark-interactive", "false");

	SECTION("\"yes\" and \"true\"") {
		REQUIRE(cfg.get_configvalue("cleanup-on-quit") == "yes");
		REQUIRE(cfg.get_configvalue_as_bool("cleanup-on-quit"));

		REQUIRE(cfg.get_configvalue("auto-reload") == "true");
		REQUIRE(cfg.get_configvalue_as_bool("auto-reload"));
	}

	SECTION("\"no\" and \"false\"") {
		REQUIRE(cfg.get_configvalue("show-read-feeds") == "no");
		REQUIRE_FALSE(cfg.get_configvalue_as_bool("show-read-feeds"));

		REQUIRE(cfg.get_configvalue("bookmark-interactive") == "false");
		REQUIRE_FALSE(
			cfg.get_configvalue_as_bool("bookmark-interactive"));
	}
}

TEST_CASE("get_configvalue_as_int() returns zero if setting doesn't exist",
	"[ConfigContainer]")
{
	ConfigContainer cfg;

	REQUIRE(cfg.get_configvalue_as_int("setting-name") == 0);
}

TEST_CASE("get_configvalue_as_int() returns zero if value can't be parsed as int",
	"[ConfigContainer]")
{
	const auto key = std::string("auto-reload");
	const auto value = std::string("true");

	ConfigContainer cfg;
	cfg.set_configvalue(key, value);

	REQUIRE(cfg.get_configvalue(key) == value);
	REQUIRE(cfg.get_configvalue_as_int(key) == 0);
}

TEST_CASE("toggle() inverts the value of a boolean setting",
	"[ConfigContainer]")
{
	ConfigContainer cfg;

	const std::string key("always-display-description");
	SECTION("\"true\" becomes \"false\"") {
		cfg.set_configvalue(key, "true");
		REQUIRE_NOTHROW(cfg.toggle(key));
		REQUIRE_FALSE(cfg.get_configvalue_as_bool(key));
	}

	SECTION("\"false\" becomes \"true\"") {
		cfg.set_configvalue(key, "false");
		REQUIRE_NOTHROW(cfg.toggle(key));
		REQUIRE(cfg.get_configvalue_as_bool(key));
	}
}

TEST_CASE("toggle() does nothing if setting is non-boolean",
	"[ConfigContainer]")
{
	ConfigContainer cfg;

	const std::vector<std::string> tests{"articlelist-title-format",
		"cache-file",
		"http-auth-method",
		"inoreader-passwordeval",
		"notify-program",
		"ocnews-passwordfile",
		"oldreader-min-items",
		"save-path"};

	for (const std::string& key : tests) {
		const std::string expected = cfg.get_configvalue(key);
		REQUIRE_NOTHROW(cfg.toggle(key));
		REQUIRE(cfg.get_configvalue(key) == expected);
	}
}

TEST_CASE(
	"dump_config turns current state into text and saves it "
	"into the supplyed vector",
	"[ConfigContainer]")
{
	ConfigContainer cfg;

	auto all_values_found =
		[](std::unordered_set<std::string>& expected,
	const std::vector<std::string>& result) {
		for (const auto& line : result) {
			auto it = expected.find(line);
			if (it != expected.end()) {
				expected.erase(it);
			}
		}

		return expected.empty();
	};

	std::vector<std::string> result;

	SECTION("By default, simply enumerates all settings") {
		REQUIRE_NOTHROW(cfg.dump_config(result));
		{
			INFO("Checking that all the expected values were found");
			std::unordered_set<std::string> expected{
				"always-display-description false",
				"download-timeout 30",
				"ignore-mode \"download\"",
				"newsblur-min-items 20",
				"oldreader-password \"\"",
				"proxy-type \"http\"",
				"ttrss-mode \"multi\""};
			REQUIRE(all_values_found(expected, result));
		}
	}

	SECTION("If setting was changed, dump_config() will mention "
		"its default value") {
		cfg.set_configvalue("download-timeout", "100");
		cfg.set_configvalue("http-auth-method", "digest");

		REQUIRE_NOTHROW(cfg.dump_config(result));
		{
			INFO("Checking that all the expected values were found");
			std::unordered_set<std::string> expected{
				"download-timeout 100 # default: 30",
				"http-auth-method \"digest\" # default: any",
			};
			REQUIRE(all_values_found(expected, result));
		}
	}
}

// Added for https://github.com/newsboat/newsboat/issues/3104
TEST_CASE("Resetting or toggling invalid config option does cause crash dump_config()",
	"[ConfigContainer]")
{
	ConfigContainer cfg;
	std::vector<std::string> dummy_output;

	SECTION("reset_to_default()") {
		cfg.reset_to_default("non-existent");
		cfg.dump_config(dummy_output);
	}

	SECTION("toggle()") {
		cfg.toggle("non-existent");
		cfg.dump_config(dummy_output);
	}
}

TEST_CASE(
	"get_suggestions() returns all settings whose names begin "
	"with a given string",
	"[ConfigContainer]")
{
	ConfigContainer cfg;

	const std::string key1("d");
	const std::unordered_set<std::string> expected1{
		"datetime-format",
		"delete-played-files",
		"delete-read-articles-on-quit",
		"dialogs-title-format",
		"display-article-progress",
		"dirbrowser-title-format",
		"download-full-page",
		"download-filename-format",
		"download-path",
		"download-retries",
		"download-timeout",
	};
	std::vector<std::string> results = cfg.get_suggestions(key1);
	const std::unordered_set<std::string> results_set1(
		results.begin(), results.end());
	REQUIRE(results_set1 == expected1);

	const std::string key2("feed");
	const std::unordered_set<std::string> expected2{
		"feed-sort-order",
		"feedbin-flag-star",
		"feedbin-url",
		"feedbin-login",
		"feedbin-password",
		"feedbin-passwordeval",
		"feedbin-passwordfile",
		"feedhq-flag-share",
		"feedhq-flag-star",
		"feedhq-login",
		"feedhq-min-items",
		"feedhq-password",
		"feedhq-passwordeval",
		"feedhq-passwordfile",
		"feedhq-show-special-feeds",
		"feedhq-url",
		"feedlist-format",
		"feedlist-title-format",
	};
	results = cfg.get_suggestions(key2);
	const std::unordered_set<std::string> results_set2(
		results.begin(), results.end());
	REQUIRE(results_set2 == expected2);
}

TEST_CASE("get_suggestions() returns results in alphabetical order",
	"[ConfigContainer]")
{
	ConfigContainer cfg;

	const std::vector<std::string> keys{"dow", "rel", "us", "d"};
	for (const auto& key : keys) {
		const std::vector<std::string> results =
			cfg.get_suggestions(key);
		for (auto one = results.begin(), two = one + 1;
			two != results.end();
			one = two, ++two) {
			INFO("Previous: " << *one);
			INFO("Current:  " << *two);
			REQUIRE(*one <= *two);
		}
	}
}

TEST_CASE(
	"get_feed_sort_strategy() returns correctly filled FeedSortStrategy "
	"struct",
	"[ConfigContainer]")
{
	ConfigContainer cfg;
	FeedSortStrategy sort_strategy;

	SECTION("none") {
		cfg.set_configvalue("feed-sort-order", "none");
		sort_strategy = cfg.get_feed_sort_strategy();
		REQUIRE(sort_strategy.sm == FeedSortMethod::NONE);
		REQUIRE(sort_strategy.sd == SortDirection::DESC);

		cfg.set_configvalue("feed-sort-order", "none-desc");
		sort_strategy = cfg.get_feed_sort_strategy();
		REQUIRE(sort_strategy.sm == FeedSortMethod::NONE);
		REQUIRE(sort_strategy.sd == SortDirection::DESC);

		cfg.set_configvalue("feed-sort-order", "none-asc");
		sort_strategy = cfg.get_feed_sort_strategy();
		REQUIRE(sort_strategy.sm == FeedSortMethod::NONE);
		REQUIRE(sort_strategy.sd == SortDirection::ASC);
	}

	SECTION("firsttag") {
		cfg.set_configvalue("feed-sort-order", "firsttag");
		sort_strategy = cfg.get_feed_sort_strategy();
		REQUIRE(sort_strategy.sm == FeedSortMethod::FIRST_TAG);
		REQUIRE(sort_strategy.sd == SortDirection::DESC);

		cfg.set_configvalue("feed-sort-order", "firsttag-desc");
		sort_strategy = cfg.get_feed_sort_strategy();
		REQUIRE(sort_strategy.sm == FeedSortMethod::FIRST_TAG);
		REQUIRE(sort_strategy.sd == SortDirection::DESC);

		cfg.set_configvalue("feed-sort-order", "firsttag-asc");
		sort_strategy = cfg.get_feed_sort_strategy();
		REQUIRE(sort_strategy.sm == FeedSortMethod::FIRST_TAG);
		REQUIRE(sort_strategy.sd == SortDirection::ASC);
	}

	SECTION("title") {
		cfg.set_configvalue("feed-sort-order", "title");
		sort_strategy = cfg.get_feed_sort_strategy();
		REQUIRE(sort_strategy.sm == FeedSortMethod::TITLE);
		REQUIRE(sort_strategy.sd == SortDirection::DESC);

		cfg.set_configvalue("feed-sort-order", "title-desc");
		sort_strategy = cfg.get_feed_sort_strategy();
		REQUIRE(sort_strategy.sm == FeedSortMethod::TITLE);
		REQUIRE(sort_strategy.sd == SortDirection::DESC);

		cfg.set_configvalue("feed-sort-order", "title-asc");
		sort_strategy = cfg.get_feed_sort_strategy();
		REQUIRE(sort_strategy.sm == FeedSortMethod::TITLE);
		REQUIRE(sort_strategy.sd == SortDirection::ASC);
	}

	SECTION("articlecount") {
		cfg.set_configvalue("feed-sort-order", "articlecount");
		sort_strategy = cfg.get_feed_sort_strategy();
		REQUIRE(sort_strategy.sm == FeedSortMethod::ARTICLE_COUNT);
		REQUIRE(sort_strategy.sd == SortDirection::DESC);

		cfg.set_configvalue("feed-sort-order", "articlecount-desc");
		sort_strategy = cfg.get_feed_sort_strategy();
		REQUIRE(sort_strategy.sm == FeedSortMethod::ARTICLE_COUNT);
		REQUIRE(sort_strategy.sd == SortDirection::DESC);

		cfg.set_configvalue("feed-sort-order", "articlecount-asc");
		sort_strategy = cfg.get_feed_sort_strategy();
		REQUIRE(sort_strategy.sm == FeedSortMethod::ARTICLE_COUNT);
		REQUIRE(sort_strategy.sd == SortDirection::ASC);
	}

	SECTION("unreadarticlecount") {
		cfg.set_configvalue("feed-sort-order", "unreadarticlecount");
		sort_strategy = cfg.get_feed_sort_strategy();
		REQUIRE(sort_strategy.sm ==
			FeedSortMethod::UNREAD_ARTICLE_COUNT);
		REQUIRE(sort_strategy.sd == SortDirection::DESC);

		cfg.set_configvalue(
			"feed-sort-order", "unreadarticlecount-desc");
		sort_strategy = cfg.get_feed_sort_strategy();
		REQUIRE(sort_strategy.sm ==
			FeedSortMethod::UNREAD_ARTICLE_COUNT);
		REQUIRE(sort_strategy.sd == SortDirection::DESC);

		cfg.set_configvalue(
			"feed-sort-order", "unreadarticlecount-asc");
		sort_strategy = cfg.get_feed_sort_strategy();
		REQUIRE(sort_strategy.sm ==
			FeedSortMethod::UNREAD_ARTICLE_COUNT);
		REQUIRE(sort_strategy.sd == SortDirection::ASC);
	}

	SECTION("lastupdated") {
		cfg.set_configvalue("feed-sort-order", "lastupdated");
		sort_strategy = cfg.get_feed_sort_strategy();
		REQUIRE(sort_strategy.sm == FeedSortMethod::LAST_UPDATED);
		REQUIRE(sort_strategy.sd == SortDirection::DESC);

		cfg.set_configvalue("feed-sort-order", "lastupdated-desc");
		sort_strategy = cfg.get_feed_sort_strategy();
		REQUIRE(sort_strategy.sm == FeedSortMethod::LAST_UPDATED);
		REQUIRE(sort_strategy.sd == SortDirection::DESC);

		cfg.set_configvalue("feed-sort-order", "lastupdated-asc");
		sort_strategy = cfg.get_feed_sort_strategy();
		REQUIRE(sort_strategy.sm == FeedSortMethod::LAST_UPDATED);
		REQUIRE(sort_strategy.sd == SortDirection::ASC);
	}
}

TEST_CASE("get_feed_sort_strategy() returns \"none\" method if it can't parse it",
	"[ConfigContainer]")
{
	ConfigContainer cfg;

	const auto check = [&cfg]() {
		const auto s = cfg.get_feed_sort_strategy();
		REQUIRE(s.sm == FeedSortMethod::NONE);
		REQUIRE(s.sd == SortDirection::DESC);
	};

	SECTION("empty value") {
		cfg.set_configvalue("feed-sort-order", "");
		check();
	}

	SECTION("unknown method") {
		SECTION("without a direction") {
			cfg.set_configvalue("feed-sort-order", "funniness");
			check();
		}

		SECTION("with an unknown direction") {
			cfg.set_configvalue("feed-sort-order", "funniness-increasing");
			check();
		}

		SECTION("with a valid direction") {
			cfg.set_configvalue("feed-sort-order", "funniness-asc");
			const auto s = cfg.get_feed_sort_strategy();
			REQUIRE(s.sm == FeedSortMethod::NONE);
			REQUIRE(s.sd == SortDirection::ASC);
		}
	}
}

TEST_CASE("get_feed_sort_strategy() returns descending direction "
	"if it can't parse it",
	"[ConfigContainer]")
{
	ConfigContainer cfg;

	SECTION("no direction specified") {
		cfg.set_configvalue("feed-sort-order", "title");
		REQUIRE(cfg.get_feed_sort_strategy().sd == SortDirection::DESC);
	}

	SECTION("unknown direction") {
		cfg.set_configvalue("feed-sort-order", "title-increasing");
		REQUIRE(cfg.get_feed_sort_strategy().sd == SortDirection::DESC);
	}
}

TEST_CASE(
	"get_article_sort_strategy() returns correctly filled "
	"ArticleSortStrategy struct",
	"[ConfigContainer]")
{
	ConfigContainer cfg;
	ArticleSortStrategy sort_strategy;

	SECTION("title") {
		cfg.set_configvalue("article-sort-order", "title");
		sort_strategy = cfg.get_article_sort_strategy();
		REQUIRE(sort_strategy.sm == ArtSortMethod::TITLE);
		REQUIRE(sort_strategy.sd == SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "title-asc");
		sort_strategy = cfg.get_article_sort_strategy();
		REQUIRE(sort_strategy.sm == ArtSortMethod::TITLE);
		REQUIRE(sort_strategy.sd == SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "title-desc");
		sort_strategy = cfg.get_article_sort_strategy();
		REQUIRE(sort_strategy.sm == ArtSortMethod::TITLE);
		REQUIRE(sort_strategy.sd == SortDirection::DESC);
	}

	SECTION("flags") {
		cfg.set_configvalue("article-sort-order", "flags");
		sort_strategy = cfg.get_article_sort_strategy();
		REQUIRE(sort_strategy.sm == ArtSortMethod::FLAGS);
		REQUIRE(sort_strategy.sd == SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "flags-asc");
		sort_strategy = cfg.get_article_sort_strategy();
		REQUIRE(sort_strategy.sm == ArtSortMethod::FLAGS);
		REQUIRE(sort_strategy.sd == SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "flags-desc");
		sort_strategy = cfg.get_article_sort_strategy();
		REQUIRE(sort_strategy.sm == ArtSortMethod::FLAGS);
		REQUIRE(sort_strategy.sd == SortDirection::DESC);
	}

	SECTION("author") {
		cfg.set_configvalue("article-sort-order", "author");
		sort_strategy = cfg.get_article_sort_strategy();
		REQUIRE(sort_strategy.sm == ArtSortMethod::AUTHOR);
		REQUIRE(sort_strategy.sd == SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "author-asc");
		sort_strategy = cfg.get_article_sort_strategy();
		REQUIRE(sort_strategy.sm == ArtSortMethod::AUTHOR);
		REQUIRE(sort_strategy.sd == SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "author-desc");
		sort_strategy = cfg.get_article_sort_strategy();
		REQUIRE(sort_strategy.sm == ArtSortMethod::AUTHOR);
		REQUIRE(sort_strategy.sd == SortDirection::DESC);
	}

	SECTION("link") {
		cfg.set_configvalue("article-sort-order", "link");
		sort_strategy = cfg.get_article_sort_strategy();
		REQUIRE(sort_strategy.sm == ArtSortMethod::LINK);
		REQUIRE(sort_strategy.sd == SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "link-asc");
		sort_strategy = cfg.get_article_sort_strategy();
		REQUIRE(sort_strategy.sm == ArtSortMethod::LINK);
		REQUIRE(sort_strategy.sd == SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "link-desc");
		sort_strategy = cfg.get_article_sort_strategy();
		REQUIRE(sort_strategy.sm == ArtSortMethod::LINK);
		REQUIRE(sort_strategy.sd == SortDirection::DESC);
	}

	SECTION("guid") {
		cfg.set_configvalue("article-sort-order", "guid");
		sort_strategy = cfg.get_article_sort_strategy();
		REQUIRE(sort_strategy.sm == ArtSortMethod::GUID);
		REQUIRE(sort_strategy.sd == SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "guid-asc");
		sort_strategy = cfg.get_article_sort_strategy();
		REQUIRE(sort_strategy.sm == ArtSortMethod::GUID);
		REQUIRE(sort_strategy.sd == SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "guid-desc");
		sort_strategy = cfg.get_article_sort_strategy();
		REQUIRE(sort_strategy.sm == ArtSortMethod::GUID);
		REQUIRE(sort_strategy.sd == SortDirection::DESC);
	}

	SECTION("date") {
		cfg.set_configvalue("article-sort-order", "date");
		sort_strategy = cfg.get_article_sort_strategy();
		REQUIRE(sort_strategy.sm == ArtSortMethod::DATE);
		REQUIRE(sort_strategy.sd == SortDirection::DESC);

		cfg.set_configvalue("article-sort-order", "date-asc");
		sort_strategy = cfg.get_article_sort_strategy();
		REQUIRE(sort_strategy.sm == ArtSortMethod::DATE);
		REQUIRE(sort_strategy.sd == SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "date-desc");
		sort_strategy = cfg.get_article_sort_strategy();
		REQUIRE(sort_strategy.sm == ArtSortMethod::DATE);
		REQUIRE(sort_strategy.sd == SortDirection::DESC);
	}

	SECTION("random") {
		cfg.set_configvalue("article-sort-order", "random");
		sort_strategy = cfg.get_article_sort_strategy();
		REQUIRE(sort_strategy.sm == ArtSortMethod::RANDOM);
		REQUIRE(sort_strategy.sd == SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "random-asc");
		sort_strategy = cfg.get_article_sort_strategy();
		REQUIRE(sort_strategy.sm == ArtSortMethod::RANDOM);
		REQUIRE(sort_strategy.sd == SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "random-desc");
		sort_strategy = cfg.get_article_sort_strategy();
		REQUIRE(sort_strategy.sm == ArtSortMethod::RANDOM);
		REQUIRE(sort_strategy.sd == SortDirection::DESC);
	}
}

TEST_CASE("get_article_sort_strategy() returns \"date\" method "
	"if it can't parse it",
	"[ConfigContainer]")
{
	ConfigContainer cfg;

	const auto check = [&cfg]() {
		const auto s = cfg.get_article_sort_strategy();
		REQUIRE(s.sm == ArtSortMethod::DATE);
		REQUIRE(s.sd == SortDirection::ASC);
	};

	SECTION("empty value") {
		cfg.set_configvalue("article-sort-order", "");
		check();
	}

	SECTION("unknown method") {
		SECTION("without a direction") {
			cfg.set_configvalue("article-sort-order", "funniness");
			check();
		}

		SECTION("with an unknown direction") {
			cfg.set_configvalue("article-sort-order", "funniness-increasing");
			check();
		}

		SECTION("with a valid direction") {
			cfg.set_configvalue("article-sort-order", "funniness-desc");
			const auto s = cfg.get_article_sort_strategy();
			REQUIRE(s.sm == ArtSortMethod::DATE);
			REQUIRE(s.sd == SortDirection::DESC);
		}
	}
}

TEST_CASE("get_article_sort_strategy() returns ascending direction "
	"if it can't parse it",
	"[ConfigContainer]")
{
	ConfigContainer cfg;

	SECTION("no direction specified and method is not \"date\"") {
		cfg.set_configvalue("article-sort-order", "author");
		REQUIRE(cfg.get_article_sort_strategy().sd == SortDirection::ASC);
	}

	SECTION("unknown direction") {
		cfg.set_configvalue("article-sort-order", "author-increasing");
		REQUIRE(cfg.get_article_sort_strategy().sd == SortDirection::ASC);
	}
}

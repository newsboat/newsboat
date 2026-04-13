#include "configcontainer.h"

#include <unordered_set>

#include "3rd-party/catch.hpp"

#include "confighandlerexception.h"
#include "configparser.h"
#include "keymap.h"

using namespace newsboat;

namespace {

void require_single_key(const FeedSortStrategy& strategy,
	FeedSortMethod method,
	SortDirection direction)
{
	REQUIRE(strategy.keys.size() == 1);
	REQUIRE(strategy.keys[0].sm == method);
	REQUIRE(strategy.keys[0].sd == direction);
}

void require_single_key(const ArticleSortStrategy& strategy,
	ArtSortMethod method,
	SortDirection direction)
{
	REQUIRE(strategy.keys.size() == 1);
	REQUIRE(strategy.keys[0].sm == method);
	REQUIRE(strategy.keys[0].sd == direction);
}

} // namespace

TEST_CASE("Parses test config without exceptions", "[ConfigContainer]")
{
	ConfigContainer cfg;
	ConfigParser cfgparser;
	cfg.register_commands(cfgparser);
	KeyMap k(KM_NEWSBOAT);
	cfgparser.register_handler("macro", k);

	REQUIRE_NOTHROW(cfgparser.parse_file("data/test-config.txt"_path));

	SECTION("bool value") {
		REQUIRE(cfg.get_configvalue("show-read-feeds") == "no");
		REQUIRE_FALSE(cfg.get_configvalue_as_bool("show-read-feeds"));
	}

	SECTION("string value") {
		REQUIRE(cfg.get_configvalue("article-sort-order") == "date-asc");
	}

	SECTION("integer value") {
		REQUIRE(cfg.get_configvalue("max-items") == "100");
		REQUIRE(cfg.get_configvalue_as_int("max-items") == 100);
	}

	SECTION("Tilde got expanded into path to user's home directory") {
		char* home = ::getenv("HOME");
		REQUIRE(home != nullptr);
		const Filepath expected = Filepath::from_locale_string(std::string(home)).join("foo"_path);
		REQUIRE(cfg.get_configvalue_as_filepath("cache-file") == expected);
	}
}

TEST_CASE(
	"Parses test config correctly, even if there's no \\n at the end line.",
	"[ConfigContainer]")
{
	ConfigContainer cfg;
	ConfigParser cfgparser;
	cfg.register_commands(cfgparser);

	REQUIRE_NOTHROW(
		cfgparser.parse_file("data/test-config-without-newline-at-the-end.txt"_path));

	SECTION("first line") {
		REQUIRE(cfg.get_configvalue("article-sort-order") == "date-asc");
	}

	SECTION("last line") {
		REQUIRE(cfg.get_configvalue_as_filepath("download-path") == "whatever"_path);
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

TEST_CASE("get_configvalue_as_filepath() returns null path if setting doesn't exist",
	"[ConfigContainer]")
{
	ConfigContainer cfg;

	REQUIRE(cfg.get_configvalue_as_filepath("foobar") == Filepath{});
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
	"into the supplied vector",
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
		require_single_key(sort_strategy, FeedSortMethod::NONE,
			SortDirection::DESC);

		cfg.set_configvalue("feed-sort-order", "none-desc");
		sort_strategy = cfg.get_feed_sort_strategy();
		require_single_key(sort_strategy, FeedSortMethod::NONE,
			SortDirection::DESC);

		cfg.set_configvalue("feed-sort-order", "none-asc");
		sort_strategy = cfg.get_feed_sort_strategy();
		require_single_key(sort_strategy, FeedSortMethod::NONE,
			SortDirection::ASC);
	}

	SECTION("firsttag") {
		cfg.set_configvalue("feed-sort-order", "firsttag");
		sort_strategy = cfg.get_feed_sort_strategy();
		require_single_key(sort_strategy, FeedSortMethod::FIRST_TAG,
			SortDirection::DESC);

		cfg.set_configvalue("feed-sort-order", "firsttag-desc");
		sort_strategy = cfg.get_feed_sort_strategy();
		require_single_key(sort_strategy, FeedSortMethod::FIRST_TAG,
			SortDirection::DESC);

		cfg.set_configvalue("feed-sort-order", "firsttag-asc");
		sort_strategy = cfg.get_feed_sort_strategy();
		require_single_key(sort_strategy, FeedSortMethod::FIRST_TAG,
			SortDirection::ASC);
	}

	SECTION("title") {
		cfg.set_configvalue("feed-sort-order", "title");
		sort_strategy = cfg.get_feed_sort_strategy();
		require_single_key(sort_strategy, FeedSortMethod::TITLE,
			SortDirection::DESC);

		cfg.set_configvalue("feed-sort-order", "title-desc");
		sort_strategy = cfg.get_feed_sort_strategy();
		require_single_key(sort_strategy, FeedSortMethod::TITLE,
			SortDirection::DESC);

		cfg.set_configvalue("feed-sort-order", "title-asc");
		sort_strategy = cfg.get_feed_sort_strategy();
		require_single_key(sort_strategy, FeedSortMethod::TITLE,
			SortDirection::ASC);
	}

	SECTION("articlecount") {
		cfg.set_configvalue("feed-sort-order", "articlecount");
		sort_strategy = cfg.get_feed_sort_strategy();
		require_single_key(sort_strategy, FeedSortMethod::ARTICLE_COUNT,
			SortDirection::DESC);

		cfg.set_configvalue("feed-sort-order", "articlecount-desc");
		sort_strategy = cfg.get_feed_sort_strategy();
		require_single_key(sort_strategy, FeedSortMethod::ARTICLE_COUNT,
			SortDirection::DESC);

		cfg.set_configvalue("feed-sort-order", "articlecount-asc");
		sort_strategy = cfg.get_feed_sort_strategy();
		require_single_key(sort_strategy, FeedSortMethod::ARTICLE_COUNT,
			SortDirection::ASC);
	}

	SECTION("unreadarticlecount") {
		cfg.set_configvalue("feed-sort-order", "unreadarticlecount");
		sort_strategy = cfg.get_feed_sort_strategy();
		require_single_key(sort_strategy, FeedSortMethod::UNREAD_ARTICLE_COUNT,
			SortDirection::DESC);

		cfg.set_configvalue(
			"feed-sort-order", "unreadarticlecount-desc");
		sort_strategy = cfg.get_feed_sort_strategy();
		require_single_key(sort_strategy, FeedSortMethod::UNREAD_ARTICLE_COUNT,
			SortDirection::DESC);

		cfg.set_configvalue(
			"feed-sort-order", "unreadarticlecount-asc");
		sort_strategy = cfg.get_feed_sort_strategy();
		require_single_key(sort_strategy, FeedSortMethod::UNREAD_ARTICLE_COUNT,
			SortDirection::ASC);
	}

	SECTION("lastupdated") {
		cfg.set_configvalue("feed-sort-order", "lastupdated");
		sort_strategy = cfg.get_feed_sort_strategy();
		require_single_key(sort_strategy, FeedSortMethod::LAST_UPDATED,
			SortDirection::DESC);

		cfg.set_configvalue("feed-sort-order", "lastupdated-desc");
		sort_strategy = cfg.get_feed_sort_strategy();
		require_single_key(sort_strategy, FeedSortMethod::LAST_UPDATED,
			SortDirection::DESC);

		cfg.set_configvalue("feed-sort-order", "lastupdated-asc");
		sort_strategy = cfg.get_feed_sort_strategy();
		require_single_key(sort_strategy, FeedSortMethod::LAST_UPDATED,
			SortDirection::ASC);
	}
}

TEST_CASE("get_feed_sort_strategy() parses multiple sort keys",
	"[ConfigContainer]")
{
	ConfigContainer cfg;
	cfg.set_configvalue("feed-sort-order", "title,unreadarticlecount-asc");
	const auto strategy = cfg.get_feed_sort_strategy();

	REQUIRE(strategy.keys.size() == 2);
	REQUIRE(strategy.keys[0].sm == FeedSortMethod::TITLE);
	REQUIRE(strategy.keys[0].sd == SortDirection::DESC);
	REQUIRE(strategy.keys[1].sm == FeedSortMethod::UNREAD_ARTICLE_COUNT);
	REQUIRE(strategy.keys[1].sd == SortDirection::ASC);
}

TEST_CASE("get_feed_sort_strategy() returns \"none\" method if it can't parse it",
	"[ConfigContainer]")
{
	ConfigContainer cfg;

	const auto check = [&cfg]() {
		const auto s = cfg.get_feed_sort_strategy();
		require_single_key(s, FeedSortMethod::NONE, SortDirection::DESC);
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
			require_single_key(s, FeedSortMethod::NONE, SortDirection::ASC);
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
		require_single_key(cfg.get_feed_sort_strategy(), FeedSortMethod::TITLE,
			SortDirection::DESC);
	}

	SECTION("unknown direction") {
		cfg.set_configvalue("feed-sort-order", "title-increasing");
		require_single_key(cfg.get_feed_sort_strategy(), FeedSortMethod::TITLE,
			SortDirection::DESC);
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
		require_single_key(sort_strategy, ArtSortMethod::TITLE,
			SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "title-asc");
		sort_strategy = cfg.get_article_sort_strategy();
		require_single_key(sort_strategy, ArtSortMethod::TITLE,
			SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "title-desc");
		sort_strategy = cfg.get_article_sort_strategy();
		require_single_key(sort_strategy, ArtSortMethod::TITLE,
			SortDirection::DESC);
	}

	SECTION("flags") {
		cfg.set_configvalue("article-sort-order", "flags");
		sort_strategy = cfg.get_article_sort_strategy();
		require_single_key(sort_strategy, ArtSortMethod::FLAGS,
			SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "flags-asc");
		sort_strategy = cfg.get_article_sort_strategy();
		require_single_key(sort_strategy, ArtSortMethod::FLAGS,
			SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "flags-desc");
		sort_strategy = cfg.get_article_sort_strategy();
		require_single_key(sort_strategy, ArtSortMethod::FLAGS,
			SortDirection::DESC);
	}

	SECTION("author") {
		cfg.set_configvalue("article-sort-order", "author");
		sort_strategy = cfg.get_article_sort_strategy();
		require_single_key(sort_strategy, ArtSortMethod::AUTHOR,
			SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "author-asc");
		sort_strategy = cfg.get_article_sort_strategy();
		require_single_key(sort_strategy, ArtSortMethod::AUTHOR,
			SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "author-desc");
		sort_strategy = cfg.get_article_sort_strategy();
		require_single_key(sort_strategy, ArtSortMethod::AUTHOR,
			SortDirection::DESC);
	}

	SECTION("link") {
		cfg.set_configvalue("article-sort-order", "link");
		sort_strategy = cfg.get_article_sort_strategy();
		require_single_key(sort_strategy, ArtSortMethod::LINK,
			SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "link-asc");
		sort_strategy = cfg.get_article_sort_strategy();
		require_single_key(sort_strategy, ArtSortMethod::LINK,
			SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "link-desc");
		sort_strategy = cfg.get_article_sort_strategy();
		require_single_key(sort_strategy, ArtSortMethod::LINK,
			SortDirection::DESC);
	}

	SECTION("guid") {
		cfg.set_configvalue("article-sort-order", "guid");
		sort_strategy = cfg.get_article_sort_strategy();
		require_single_key(sort_strategy, ArtSortMethod::GUID,
			SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "guid-asc");
		sort_strategy = cfg.get_article_sort_strategy();
		require_single_key(sort_strategy, ArtSortMethod::GUID,
			SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "guid-desc");
		sort_strategy = cfg.get_article_sort_strategy();
		require_single_key(sort_strategy, ArtSortMethod::GUID,
			SortDirection::DESC);
	}

	SECTION("date") {
		cfg.set_configvalue("article-sort-order", "date");
		sort_strategy = cfg.get_article_sort_strategy();
		require_single_key(sort_strategy, ArtSortMethod::DATE,
			SortDirection::DESC);

		cfg.set_configvalue("article-sort-order", "date-asc");
		sort_strategy = cfg.get_article_sort_strategy();
		require_single_key(sort_strategy, ArtSortMethod::DATE,
			SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "date-desc");
		sort_strategy = cfg.get_article_sort_strategy();
		require_single_key(sort_strategy, ArtSortMethod::DATE,
			SortDirection::DESC);
	}

	SECTION("unread") {
		cfg.set_configvalue("article-sort-order", "unread");
		sort_strategy = cfg.get_article_sort_strategy();
		require_single_key(sort_strategy, ArtSortMethod::UNREAD,
			SortDirection::DESC);

		cfg.set_configvalue("article-sort-order", "unread-asc");
		sort_strategy = cfg.get_article_sort_strategy();
		require_single_key(sort_strategy, ArtSortMethod::UNREAD,
			SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "unread-desc");
		sort_strategy = cfg.get_article_sort_strategy();
		require_single_key(sort_strategy, ArtSortMethod::UNREAD,
			SortDirection::DESC);
	}

	SECTION("random") {
		cfg.set_configvalue("article-sort-order", "random");
		sort_strategy = cfg.get_article_sort_strategy();
		require_single_key(sort_strategy, ArtSortMethod::RANDOM,
			SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "random-asc");
		sort_strategy = cfg.get_article_sort_strategy();
		require_single_key(sort_strategy, ArtSortMethod::RANDOM,
			SortDirection::ASC);

		cfg.set_configvalue("article-sort-order", "random-desc");
		sort_strategy = cfg.get_article_sort_strategy();
		require_single_key(sort_strategy, ArtSortMethod::RANDOM,
			SortDirection::DESC);
	}
}

TEST_CASE("get_article_sort_strategy() parses multiple sort keys",
	"[ConfigContainer]")
{
	ConfigContainer cfg;
	cfg.set_configvalue("article-sort-order", "date-asc,unread");
	const auto strategy = cfg.get_article_sort_strategy();

	REQUIRE(strategy.keys.size() == 2);
	REQUIRE(strategy.keys[0].sm == ArtSortMethod::DATE);
	REQUIRE(strategy.keys[0].sd == SortDirection::ASC);
	REQUIRE(strategy.keys[1].sm == ArtSortMethod::UNREAD);
	REQUIRE(strategy.keys[1].sd == SortDirection::DESC);
}

TEST_CASE("get_article_sort_strategy() returns \"date\" method "
	"if it can't parse it",
	"[ConfigContainer]")
{
	ConfigContainer cfg;

	const auto check = [&cfg]() {
		const auto s = cfg.get_article_sort_strategy();
		require_single_key(s, ArtSortMethod::DATE, SortDirection::ASC);
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
			require_single_key(s, ArtSortMethod::DATE, SortDirection::DESC);
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
		require_single_key(cfg.get_article_sort_strategy(),
			ArtSortMethod::AUTHOR, SortDirection::ASC);
	}

	SECTION("unknown direction") {
		cfg.set_configvalue("article-sort-order", "author-increasing");
		require_single_key(cfg.get_article_sort_strategy(),
			ArtSortMethod::AUTHOR, SortDirection::ASC);
	}
}

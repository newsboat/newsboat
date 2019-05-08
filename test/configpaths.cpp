#include "configpaths.h"

#include "3rd-party/catch.hpp"
#include "test-helpers.h"
#include "utils.h"

using namespace newsboat;

TEST_CASE("ConfigPaths returns paths to Newsboat dotdir if no Newsboat dirs "
		"exist",
		"[ConfigPaths]")
{
	const auto test_dir = std::string("data/configpaths--no-dirs");
	const auto newsboat_dir = test_dir + "/.newsboat";

	TestHelpers::EnvVar home("HOME");
	home.set(test_dir);

	ConfigPaths paths;
	REQUIRE(paths.initialized());
	REQUIRE(paths.url_file() == newsboat_dir + "/urls");
	REQUIRE(paths.cache_file() == newsboat_dir + "/cache.db");
	REQUIRE(paths.lock_file() == newsboat_dir + "/cache.db.lock");
	REQUIRE(paths.config_file() == newsboat_dir + "/config");
	REQUIRE(paths.queue_file() == newsboat_dir + "/queue");
	REQUIRE(paths.search_file() == newsboat_dir + "/history.search");
	REQUIRE(paths.cmdline_file() == newsboat_dir + "/history.cmdline");
}

TEST_CASE("ConfigPaths returns paths to Newsboat XDG dirs if they exist and "
		"the dotdir doesn't",
		"[ConfigPaths]")
{
	const auto test_dir = std::string("data/configpaths--only-xdg");
	const auto config_dir = test_dir + "/.config/newsboat";
	const auto data_dir = test_dir + "/.local/share/newsboat";

	TestHelpers::EnvVar home("HOME");
	home.set(test_dir);

	const auto check = [&]() {
		ConfigPaths paths;
		REQUIRE(paths.initialized());
		REQUIRE(paths.config_file() == config_dir + "/config");
		REQUIRE(paths.url_file() == config_dir + "/urls");
		REQUIRE(paths.cache_file() == data_dir + "/cache.db");
		REQUIRE(paths.lock_file() == data_dir + "/cache.db.lock");
		REQUIRE(paths.queue_file() == data_dir + "/queue");
		REQUIRE(paths.search_file() == data_dir + "/history.search");
		REQUIRE(paths.cmdline_file() == data_dir + "/history.cmdline");
	};

	SECTION("XDG_CONFIG_HOME is set") {
		TestHelpers::EnvVar xdg_config("XDG_CONFIG_HOME");
		xdg_config.set(test_dir + "/.config");

		SECTION("XDG_DATA_HOME is set") {
			TestHelpers::EnvVar xdg_data("XDG_DATA_HOME");
			xdg_data.set(test_dir + "/.local/share");
			check();
		}

		SECTION("XDG_DATA_HOME is not set") {
			TestHelpers::EnvVar xdg_data("XDG_DATA_HOME");
			xdg_data.unset();
			check();
		}
	}

	SECTION("XDG_CONFIG_HOME is not set") {
		TestHelpers::EnvVar xdg_config("XDG_CONFIG_HOME");
		xdg_config.unset();

		SECTION("XDG_DATA_HOME is set") {
			TestHelpers::EnvVar xdg_data("XDG_DATA_HOME");
			xdg_data.set(test_dir + "/.local/share");
			check();
		}

		SECTION("XDG_DATA_HOME is not set") {
			TestHelpers::EnvVar xdg_data("XDG_DATA_HOME");
			xdg_data.unset();
			check();
		}
	}
}

TEST_CASE("ConfigPaths::process_args replaces paths with the ones supplied by "
		"CliArgsParser",
		"[ConfigPaths]")
{
	const auto url_file = std::string("my urls file");
	const auto cache_file = std::string("/path/to/cache file.db");
	const auto lock_file = cache_file + ".lock";
	const auto config_file = std::string("this is a/config");
	TestHelpers::Opts opts({
			"newsboat",
			"-u", url_file,
			"-c", cache_file,
			"-C", config_file,
			"-q"});
	CliArgsParser parser(opts.argc(), opts.argv());
	ConfigPaths paths;
	REQUIRE(paths.initialized());
	paths.process_args(parser);
	REQUIRE(paths.url_file() == url_file);
	REQUIRE(paths.cache_file() == cache_file);
	REQUIRE(paths.lock_file() == lock_file);
	REQUIRE(paths.config_file() == config_file);
}

TEST_CASE("ConfigPaths::set_cache_file changes paths to cache and lock files",
		"[ConfigPaths]")
{
	const auto test_dir = std::string("some/dir/we/use/as/home");
	const auto newsboat_dir = test_dir + "/.newsboat/";

	TestHelpers::EnvVar home("HOME");
	home.set(test_dir);

	ConfigPaths paths;
	REQUIRE(paths.initialized());

	REQUIRE(paths.cache_file() == newsboat_dir + "cache.db");
	REQUIRE(paths.lock_file() == newsboat_dir + "cache.db.lock");

	const auto new_cache = std::string("something/entirely different.sqlite3");
	paths.set_cache_file(new_cache);
	REQUIRE(paths.cache_file() == new_cache);
	REQUIRE(paths.lock_file() == new_cache + ".lock");
}

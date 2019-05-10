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

TEST_CASE("ConfigPaths::create_dirs() returns true if both config and data dirs "
		"were successfully created or already existed", "[ConfigPaths]")
{
	TestHelpers::TempDir tmp;

	TestHelpers::EnvVar home("HOME");
	home.set(tmp.getPath());
	INFO("Temporary directory (used as HOME): " << tmp.getPath());

	const auto require_exists = [&tmp](std::vector<std::string> dirs) {
		ConfigPaths paths;
		REQUIRE(paths.initialized());

		const auto starts_with =
		[]
		(const std::string& input, const std::string& prefix) -> bool {
			return input.substr(0, prefix.size()) == prefix;
		};
		REQUIRE(starts_with(paths.url_file(), tmp.getPath()));
		REQUIRE(starts_with(paths.config_file(), tmp.getPath()));
		REQUIRE(starts_with(paths.cache_file(), tmp.getPath()));
		REQUIRE(starts_with(paths.lock_file(), tmp.getPath()));
		REQUIRE(starts_with(paths.queue_file(), tmp.getPath()));
		REQUIRE(starts_with(paths.search_file(), tmp.getPath()));
		REQUIRE(starts_with(paths.cmdline_file(), tmp.getPath()));

		REQUIRE(paths.create_dirs());

		for (const auto& dir : dirs) {
			INFO("Checking directory " << dir);
			const auto dir_exists = 0 == ::access(dir.c_str(), R_OK | X_OK);
			REQUIRE(dir_exists);
		}
	};

	const auto dotdir = tmp.getPath() + ".newsboat/";
	const auto default_config_dir = tmp.getPath() + ".config/newsboat/";
	const auto default_data_dir = tmp.getPath() + ".local/share/newsboat/";

	SECTION("Using dotdir") {
		SECTION("Dotdir didn't exist") {
			require_exists({dotdir});
		}

		SECTION("Dotdir already existed") {
			INFO("Creating " << dotdir);
			REQUIRE(utils::mkdir_parents(dotdir, 0700) == 0);
			require_exists({dotdir});
		}
	}

	SECTION("Using XDG dirs") {
		SECTION("No XDG environment variables") {
			TestHelpers::EnvVar xdg_config_home("XDG_CONFIG_HOME");
			xdg_config_home.unset();
			TestHelpers::EnvVar xdg_data_home("XDG_DATA_HOME");
			xdg_data_home.unset();

			SECTION("No dirs existed => dotdir created") {
				require_exists({dotdir});
			}

			SECTION("Config dir existed, data dir didn't => data dir created") {
				INFO("Creating " << default_config_dir);
				REQUIRE(utils::mkdir_parents(default_config_dir, 0700) == 0);
				require_exists({default_config_dir, default_data_dir});
			}

			SECTION("Data dir existed, config dir didn't => dotdir created") {
				INFO("Creating " << default_data_dir);
				REQUIRE(utils::mkdir_parents(default_data_dir, 0700) == 0);
				require_exists({dotdir});
			}

			SECTION("Both config and data dir existed => just returns true") {
				INFO("Creating " << default_config_dir);
				REQUIRE(utils::mkdir_parents(default_config_dir, 0700) == 0);
				INFO("Creating " << default_data_dir);
				REQUIRE(utils::mkdir_parents(default_data_dir, 0700) == 0);
				require_exists({default_config_dir, default_data_dir});
			}
		}

		SECTION("XDG_CONFIG_HOME redefined") {
			const auto config_home =
				tmp.getPath() + "config" + std::to_string(rand()) + "/";
			INFO("Config home is " << config_home);
			const auto config_dir = config_home + "/newsboat";

			TestHelpers::EnvVar xdg_config_home("XDG_CONFIG_HOME");
			xdg_config_home.set(config_home);

			TestHelpers::EnvVar xdg_data_home("XDG_DATA_HOME");
			xdg_data_home.unset();

			SECTION("No dirs existed => dotdir created") {
				require_exists({dotdir});
			}

			SECTION("Config dir existed, data dir didn't => data dir created") {
				INFO("Creating " << config_dir);
				REQUIRE(utils::mkdir_parents(config_dir, 0700) == 0);
				require_exists({config_dir, default_data_dir});
			}

			SECTION("Data dir existed, config dir didn't => dotdir created") {
				INFO("Creating " << default_data_dir);
				REQUIRE(utils::mkdir_parents(default_data_dir, 0700) == 0);
				require_exists({dotdir});
			}

			SECTION("Both config and data dir existed => just returns true") {
				INFO("Creating " << config_dir);
				REQUIRE(utils::mkdir_parents(config_dir, 0700) == 0);
				INFO("Creating " << default_data_dir);
				REQUIRE(utils::mkdir_parents(default_data_dir, 0700) == 0);
				require_exists({config_dir, default_data_dir});
			}
		}

		SECTION("XDG_DATA_HOME redefined") {
			TestHelpers::EnvVar xdg_config_home("XDG_CONFIG_HOME");
			xdg_config_home.unset();

			const auto data_home =
				tmp.getPath() + "data" + std::to_string(rand()) + "/";
			INFO("Data home is " << data_home);
			const auto data_dir = data_home + "/newsboat";

			TestHelpers::EnvVar xdg_data_home("XDG_DATA_HOME");
			xdg_data_home.set(data_home);

			SECTION("No dirs existed => dotdir created") {
				require_exists({dotdir});
			}

			SECTION("Config dir existed, data dir didn't => data dir created") {
				INFO("Creating " << default_config_dir);
				REQUIRE(utils::mkdir_parents(default_config_dir, 0700) == 0);
				require_exists({default_config_dir, data_dir});
			}

			SECTION("Data dir existed, config dir didn't => dotdir created") {
				INFO("Creating " << data_dir);
				REQUIRE(utils::mkdir_parents(data_dir, 0700) == 0);
				require_exists({dotdir});
			}

			SECTION("Both config and data dir existed => just returns true") {
				INFO("Creating " << default_config_dir);
				REQUIRE(utils::mkdir_parents(default_config_dir, 0700) == 0);
				INFO("Creating " << data_dir);
				REQUIRE(utils::mkdir_parents(data_dir, 0700) == 0);
				require_exists({default_config_dir, data_dir});
			}
		}

		SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
			const auto config_home =
				tmp.getPath() + "config" + std::to_string(rand()) + "/";
			INFO("Config home is " << config_home);
			const auto config_dir = config_home + "/newsboat";

			const auto data_home =
				tmp.getPath() + "data" + std::to_string(rand()) + "/";
			INFO("Data home is " << data_home);
			const auto data_dir = data_home + "/newsboat";

			TestHelpers::EnvVar xdg_config_home("XDG_CONFIG_HOME");
			xdg_config_home.set(config_home);

			TestHelpers::EnvVar xdg_data_home("XDG_DATA_HOME");
			xdg_data_home.set(data_home);

			SECTION("No dirs existed => dotdir created") {
				require_exists({dotdir});
			}

			SECTION("Config dir existed, data dir didn't => data dir created") {
				INFO("Creating " << config_dir);
				REQUIRE(utils::mkdir_parents(config_dir, 0700) == 0);
				require_exists({config_dir, data_dir});
			}

			SECTION("Data dir existed, config dir didn't => dotdir created") {
				INFO("Creating " << data_dir);
				REQUIRE(utils::mkdir_parents(data_dir, 0700) == 0);
				require_exists({dotdir});
			}

			SECTION("Both config and data dir existed => just returns true") {
				INFO("Creating " << config_dir);
				REQUIRE(utils::mkdir_parents(config_dir, 0700) == 0);
				INFO("Creating " << data_dir);
				REQUIRE(utils::mkdir_parents(data_dir, 0700) == 0);
				require_exists({config_dir, data_dir});
			}
		}
	}
}

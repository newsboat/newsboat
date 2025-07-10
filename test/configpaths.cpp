#include "configpaths.h"

#include <fstream>
#include <unistd.h>

#include "3rd-party/catch.hpp"
#include "test_helpers/chmod.h"
#include "test_helpers/envvar.h"
#include "test_helpers/misc.h"
#include "test_helpers/opts.h"
#include "test_helpers/tempdir.h"
#include "utils.h"

using namespace Newsboat;

TEST_CASE("ConfigPaths returns paths to Newsboat dotdir if no Newsboat dirs "
	"exist",
	"[ConfigPaths]")
{
	test_helpers::TempDir tmp;
	const auto Newsboat_dir = tmp.get_path() + ".Newsboat";
	REQUIRE(0 == utils::mkdir_parents(Newsboat_dir, 0700));

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	ConfigPaths paths;
	REQUIRE(paths.initialized());
	REQUIRE(paths.url_file() == Newsboat_dir + "/urls");
	REQUIRE(paths.cache_file() == Newsboat_dir + "/cache.db");
	REQUIRE(paths.lock_file() == Newsboat_dir + "/cache.db.lock");
	REQUIRE(paths.config_file() == Newsboat_dir + "/config");
	REQUIRE(paths.queue_file() == Newsboat_dir + "/queue");
	REQUIRE(paths.search_history_file() == Newsboat_dir + "/history.search");
	REQUIRE(paths.cmdline_history_file() == Newsboat_dir + "/history.cmdline");
}

TEST_CASE("ConfigPaths returns paths to Newsboat XDG dirs if they exist and "
	"the dotdir doesn't",
	"[ConfigPaths]")
{
	test_helpers::TempDir tmp;
	const auto config_dir = tmp.get_path() + ".config/Newsboat";
	REQUIRE(0 == utils::mkdir_parents(config_dir, 0700));
	const auto data_dir = tmp.get_path() + ".local/share/Newsboat";
	REQUIRE(0 == utils::mkdir_parents(data_dir, 0700));

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	const auto check = [&]() {
		ConfigPaths paths;
		REQUIRE(paths.initialized());
		REQUIRE(paths.config_file() == config_dir + "/config");
		REQUIRE(paths.url_file() == config_dir + "/urls");
		REQUIRE(paths.cache_file() == data_dir + "/cache.db");
		REQUIRE(paths.lock_file() == data_dir + "/cache.db.lock");
		REQUIRE(paths.queue_file() == data_dir + "/queue");
		REQUIRE(paths.search_history_file() == data_dir + "/history.search");
		REQUIRE(paths.cmdline_history_file() == data_dir + "/history.cmdline");
	};

	SECTION("XDG_CONFIG_HOME is set") {
		test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
		xdg_config.set(tmp.get_path() + ".config");

		SECTION("XDG_DATA_HOME is set") {
			test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
			xdg_data.set(tmp.get_path() + ".local/share");
			check();
		}

		SECTION("XDG_DATA_HOME is not set") {
			test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
			xdg_data.unset();
			check();
		}
	}

	SECTION("XDG_CONFIG_HOME is not set") {
		test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
		xdg_config.unset();

		SECTION("XDG_DATA_HOME is set") {
			test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
			xdg_data.set(tmp.get_path() + ".local/share");
			check();
		}

		SECTION("XDG_DATA_HOME is not set") {
			test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
			xdg_data.unset();
			check();
		}
	}
}

TEST_CASE("ConfigPaths::process_args replaces paths with the ones supplied by "
	"CliArgsParser",
	"[ConfigPaths]")
{
	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar home("HOME");
	home.unset();
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	const auto url_file = std::string("my urls file");
	const auto cache_file = std::string("/path/to/cache file.db");
	const auto lock_file = cache_file + ".lock";
	const auto config_file = std::string("this is a/config");
	test_helpers::Opts opts({
		"Newsboat",
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
	const auto Newsboat_dir = test_dir + "/.Newsboat/";

	test_helpers::EnvVar home("HOME");
	home.set(test_dir);

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	ConfigPaths paths;
	REQUIRE(paths.initialized());

	REQUIRE(paths.cache_file() == Newsboat_dir + "cache.db");
	REQUIRE(paths.lock_file() == Newsboat_dir + "cache.db.lock");

	const auto new_cache = std::string("something/entirely different.sqlite3");
	paths.set_cache_file(new_cache);
	REQUIRE(paths.cache_file() == new_cache);
	REQUIRE(paths.lock_file() == new_cache + ".lock");
}

TEST_CASE("ConfigPaths::create_dirs() returns true if both config and data dirs "
	"were successfully created or already existed", "[ConfigPaths]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());
	INFO("Temporary directory (used as HOME): " << tmp.get_path());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	const auto require_exists = [&tmp](std::vector<std::string> dirs) {
		ConfigPaths paths;
		REQUIRE(paths.initialized());

		REQUIRE(test_helpers::starts_with(tmp.get_path(), paths.url_file()));
		REQUIRE(test_helpers::starts_with(tmp.get_path(), paths.config_file()));
		REQUIRE(test_helpers::starts_with(tmp.get_path(), paths.cache_file()));
		REQUIRE(test_helpers::starts_with(tmp.get_path(), paths.lock_file()));
		REQUIRE(test_helpers::starts_with(tmp.get_path(), paths.queue_file()));
		REQUIRE(test_helpers::starts_with(tmp.get_path(), paths.search_history_file()));
		REQUIRE(test_helpers::starts_with(tmp.get_path(), paths.cmdline_history_file()));

		REQUIRE(paths.create_dirs());

		for (const auto& dir : dirs) {
			INFO("Checking directory " << dir);
			const auto dir_exists = 0 == ::access(dir.c_str(), R_OK | X_OK);
			REQUIRE(dir_exists);
		}
	};

	const auto dotdir = tmp.get_path() + ".Newsboat/";
	const auto default_config_dir = tmp.get_path() + ".config/Newsboat/";
	const auto default_data_dir = tmp.get_path() + ".local/share/Newsboat/";

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
			test_helpers::EnvVar xdg_config_home("XDG_CONFIG_HOME");
			xdg_config_home.unset();
			test_helpers::EnvVar xdg_data_home("XDG_DATA_HOME");
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
				tmp.get_path() + "config" + std::to_string(rand()) + "/";
			INFO("Config home is " << config_home);
			const auto config_dir = config_home + "/Newsboat";

			test_helpers::EnvVar xdg_config_home("XDG_CONFIG_HOME");
			xdg_config_home.set(config_home);

			test_helpers::EnvVar xdg_data_home("XDG_DATA_HOME");
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
			test_helpers::EnvVar xdg_config_home("XDG_CONFIG_HOME");
			xdg_config_home.unset();

			const auto data_home =
				tmp.get_path() + "data" + std::to_string(rand()) + "/";
			INFO("Data home is " << data_home);
			const auto data_dir = data_home + "/Newsboat";

			test_helpers::EnvVar xdg_data_home("XDG_DATA_HOME");
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
				tmp.get_path() + "config" + std::to_string(rand()) + "/";
			INFO("Config home is " << config_home);
			const auto config_dir = config_home + "/Newsboat";

			const auto data_home =
				tmp.get_path() + "data" + std::to_string(rand()) + "/";
			INFO("Data home is " << data_home);
			const auto data_dir = data_home + "/Newsboat";

			test_helpers::EnvVar xdg_config_home("XDG_CONFIG_HOME");
			xdg_config_home.set(config_home);

			test_helpers::EnvVar xdg_data_home("XDG_DATA_HOME");
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

/// Creates a file at given `filepath` and writes `content` into it.
///
/// Returns `true` if successful, `false` otherwise.
bool create_file(const std::string& filepath, const std::string& contents)
{
	std::ofstream out(filepath, std::ios::trunc);
	if (!out.is_open()) {
		return false;
	}
	out << contents;
	out.close();
	return !out.fail();
}

/// Strings that are placed in files before running commands, to see if
/// commands modify those files.
struct FileSentries {
	std::string config = std::to_string(rand()) + "config";
	std::string urls = std::to_string(rand()) + "urls";
	std::string cache = std::to_string(rand()) + "cache";
	std::string queue = std::to_string(rand()) + "queue";
	std::string search = std::to_string(rand()) + "search";
	std::string cmdline = std::to_string(rand()) + "cmdline";
};

void mock_newsbeuter_dotdir(
	const test_helpers::TempDir& tmp,
	const FileSentries& sentries)
{
	const auto dotdir_path = tmp.get_path() + ".newsbeuter/";
	REQUIRE(::mkdir(dotdir_path.c_str(), 0700) == 0);
	REQUIRE(create_file(dotdir_path + "config", sentries.config));
	REQUIRE(create_file(dotdir_path + "urls", sentries.urls));
	REQUIRE(create_file(dotdir_path + "cache.db", sentries.cache));
	REQUIRE(create_file(dotdir_path + "queue", sentries.queue));
	REQUIRE(create_file(dotdir_path + "history.search", sentries.search));
	REQUIRE(create_file(dotdir_path + "history.cmdline", sentries.cmdline));
}

void mock_newsbeuter_xdg_dirs(
	const std::string& config_dir_path,
	const std::string& data_dir_path,
	const FileSentries& sentries)
{
	REQUIRE(utils::mkdir_parents(config_dir_path, 0700) == 0);
	REQUIRE(create_file(config_dir_path + "config", sentries.config));
	REQUIRE(create_file(config_dir_path + "urls", sentries.urls));

	REQUIRE(utils::mkdir_parents(data_dir_path, 0700) == 0);
	REQUIRE(create_file(data_dir_path + "cache.db", sentries.cache));
	REQUIRE(create_file(data_dir_path + "queue", sentries.queue));
	REQUIRE(create_file(data_dir_path + "history.search", sentries.search));
	REQUIRE(create_file(data_dir_path + "history.cmdline", sentries.cmdline));
}

void mock_newsbeuter_xdg_dirs(
	const test_helpers::TempDir& tmp,
	const FileSentries& sentries)
{
	const auto config_dir_path = tmp.get_path() + ".config/newsbeuter/";
	const auto data_dir_path = tmp.get_path() + ".local/share/newsbeuter/";
	mock_newsbeuter_xdg_dirs(config_dir_path, data_dir_path, sentries);
}

void mock_Newsboat_dotdir(
	const test_helpers::TempDir& tmp,
	const FileSentries& sentries)
{
	const auto dotdir_path = tmp.get_path() + ".Newsboat/";
	REQUIRE(::mkdir(dotdir_path.c_str(), 0700) == 0);

	const auto urls_file = dotdir_path + "urls";
	REQUIRE(create_file(urls_file, sentries.urls));
}

void mock_Newsboat_xdg_dirs(
	const std::string& config_dir_path,
	const std::string& /*data_dir_path*/,
	const FileSentries& sentries)
{
	REQUIRE(utils::mkdir_parents(config_dir_path, 0700) == 0);

	const auto urls_file = config_dir_path + "urls";
	REQUIRE(create_file(urls_file, sentries.urls));
}

void mock_Newsboat_xdg_dirs(
	const test_helpers::TempDir& tmp,
	const FileSentries& sentries)
{
	const auto config_dir_path = tmp.get_path() + ".config/Newsboat/";
	const auto data_dir_path = tmp.get_path() + ".local/share/Newsboat/";
	mock_Newsboat_xdg_dirs(config_dir_path, data_dir_path, sentries);
}

TEST_CASE("try_migrate_from_newsbeuter() doesn't migrate if config paths "
	"were specified on the command line",
	"[ConfigPaths]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());
	INFO("Temporary directory (used as HOME): " << tmp.get_path());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	FileSentries beuter_sentries;

	const auto check = [&]() {
		FileSentries boat_sentries;

		const auto url_file = tmp.get_path() + "my urls file";
		REQUIRE(create_file(url_file, boat_sentries.urls));

		const auto cache_file = tmp.get_path() + "new cache.db";
		REQUIRE(create_file(cache_file, boat_sentries.cache));

		const auto config_file = tmp.get_path() + "custom config file";
		REQUIRE(create_file(config_file, boat_sentries.config));

		test_helpers::Opts opts({
			"Newsboat",
			"-u", url_file,
			"-c", cache_file,
			"-C", config_file,
			"-q"});
		CliArgsParser parser(opts.argc(), opts.argv());
		ConfigPaths paths;
		REQUIRE(paths.initialized());
		paths.process_args(parser);

		// No migration should occur, so should return false.
		REQUIRE_FALSE(paths.try_migrate_from_newsbeuter());

		INFO("Newsbeuter's urls file sentry: " << beuter_sentries.urls);
		REQUIRE(test_helpers::file_contents(url_file).at(0) == boat_sentries.urls);

		INFO("Newsbeuter's config file sentry: " << beuter_sentries.config);
		REQUIRE(test_helpers::file_contents(config_file).at(0) == boat_sentries.config);

		INFO("Newsbeuter's cache file sentry: " << beuter_sentries.cache);
		REQUIRE(test_helpers::file_contents(cache_file).at(0) == boat_sentries.cache);
	};

	SECTION("Newsbeuter dotdir exists") {
		mock_newsbeuter_dotdir(tmp, beuter_sentries);
		check();
	}

	SECTION("Newsbeuter XDG dirs exist") {
		mock_newsbeuter_xdg_dirs(tmp, beuter_sentries);
		check();
	}
}

TEST_CASE("try_migrate_from_newsbeuter() doesn't migrate if urls file "
	"already exists",
	"[ConfigPaths]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());
	INFO("Temporary directory (used as HOME): " << tmp.get_path());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	FileSentries beuter_sentries;
	FileSentries boat_sentries;

	const auto check = [&](const std::string& urls_filepath) {
		ConfigPaths paths;
		REQUIRE(paths.initialized());

		// No migration should occur, so should return false.
		REQUIRE_FALSE(paths.try_migrate_from_newsbeuter());

		INFO("Newsbeuter's urls file sentry: " << beuter_sentries.urls);
		REQUIRE(test_helpers::file_contents(urls_filepath).at(0) == boat_sentries.urls);
	};

	SECTION("Newsbeuter dotdir exists") {
		mock_newsbeuter_dotdir(tmp, beuter_sentries);

		SECTION("Newsboat uses dotdir") {
			mock_Newsboat_dotdir(tmp, boat_sentries);
			check(tmp.get_path() + ".Newsboat/urls");
		}

		SECTION("Newsboat uses XDG") {
			SECTION("Default XDG locations") {
				mock_Newsboat_xdg_dirs(tmp, boat_sentries);
				check(tmp.get_path() + ".config/Newsboat/urls");
			}

			SECTION("XDG_CONFIG_HOME redefined") {
				const auto config_dir = tmp.get_path() + "xdg-conf/";
				REQUIRE(::mkdir(config_dir.c_str(), 0700) == 0);
				xdg_config.set(config_dir);
				const auto Newsboat_config_dir = config_dir + "Newsboat/";
				mock_Newsboat_xdg_dirs(
					Newsboat_config_dir,
					tmp.get_path() + ".local/share/Newsboat/",
					boat_sentries);
				check(Newsboat_config_dir + "urls");
			}

			SECTION("XDG_DATA_HOME redefined") {
				const auto data_dir = tmp.get_path() + "xdg-data/";
				REQUIRE(::mkdir(data_dir.c_str(), 0700) == 0);
				xdg_data.set(data_dir);
				const auto Newsboat_config_dir =
					tmp.get_path() + ".config/Newsboat/";
				const auto Newsboat_data_dir = data_dir + "Newsboat/";
				mock_Newsboat_xdg_dirs(
					Newsboat_config_dir,
					Newsboat_data_dir,
					boat_sentries);
				check(Newsboat_config_dir + "urls");
			}

			SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
				const auto config_dir = tmp.get_path() + "xdg-conf/";
				REQUIRE(::mkdir(config_dir.c_str(), 0700) == 0);
				xdg_config.set(config_dir);

				const auto data_dir = tmp.get_path() + "xdg-data/";
				REQUIRE(::mkdir(data_dir.c_str(), 0700) == 0);
				xdg_data.set(data_dir);

				const auto Newsboat_config_dir = config_dir + "Newsboat/";
				const auto Newsboat_data_dir = data_dir + "Newsboat/";

				mock_Newsboat_xdg_dirs(
					Newsboat_config_dir,
					Newsboat_data_dir,
					boat_sentries);

				check(Newsboat_config_dir + "urls");
			}
		}
	}

	SECTION("Newsbeuter XDG dirs exist") {
		SECTION("Default XDG locations") {
			mock_newsbeuter_xdg_dirs(tmp, beuter_sentries);

			SECTION("Newsboat uses dotdir") {
				mock_Newsboat_dotdir(tmp, boat_sentries);
				check(tmp.get_path() + ".Newsboat/urls");
			}

			SECTION("Newsboat uses XDG") {
				mock_Newsboat_xdg_dirs(tmp, boat_sentries);
				check(tmp.get_path() + ".config/Newsboat/urls");
			}
		}

		SECTION("XDG_CONFIG_HOME redefined") {
			const auto config_dir = tmp.get_path() + "xdg-conf/";
			REQUIRE(::mkdir(config_dir.c_str(), 0700) == 0);
			xdg_config.set(config_dir);
			mock_newsbeuter_xdg_dirs(
				config_dir + "newsbeuter/",
				tmp.get_path() + ".local/share/newsbeuter/",
				beuter_sentries);

			SECTION("Newsboat uses dotdir") {
				mock_Newsboat_dotdir(tmp, boat_sentries);
				check(tmp.get_path() + ".Newsboat/urls");
			}

			SECTION("Newsboat uses XDG") {
				const auto Newsboat_config_dir = config_dir + "Newsboat/";
				mock_Newsboat_xdg_dirs(
					Newsboat_config_dir,
					tmp.get_path() + ".local/share/Newsboat/",
					boat_sentries);
				check(Newsboat_config_dir + "urls");
			}
		}

		SECTION("XDG_DATA_HOME redefined") {
			const auto data_dir = tmp.get_path() + "xdg-data/";
			REQUIRE(::mkdir(data_dir.c_str(), 0700) == 0);
			xdg_data.set(data_dir);
			mock_newsbeuter_xdg_dirs(
				tmp.get_path() + ".config/newsbeuter/",
				data_dir,
				beuter_sentries);

			SECTION("Newsboat uses dotdir") {
				mock_Newsboat_dotdir(tmp, boat_sentries);
				check(tmp.get_path() + ".Newsboat/urls");
			}

			SECTION("Newsboat uses XDG") {
				const auto Newsboat_config_dir =
					tmp.get_path() + ".config/Newsboat/";
				mock_Newsboat_xdg_dirs(
					Newsboat_config_dir,
					data_dir + "Newsboat/",
					boat_sentries);
				check(Newsboat_config_dir + "urls");
			}
		}

		SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
			const auto config_dir = tmp.get_path() + "xdg-conf/";
			REQUIRE(::mkdir(config_dir.c_str(), 0700) == 0);
			xdg_config.set(config_dir);

			const auto data_dir = tmp.get_path() + "xdg-data/";
			REQUIRE(::mkdir(data_dir.c_str(), 0700) == 0);
			xdg_data.set(data_dir);

			mock_newsbeuter_xdg_dirs(
				config_dir + "newsbeuter/",
				data_dir + "newsbeuter/",
				beuter_sentries);

			SECTION("Newsboat uses dotdir") {
				mock_Newsboat_dotdir(tmp, boat_sentries);
				check(tmp.get_path() + ".Newsboat/urls");
			}

			SECTION("Newsboat uses XDG") {
				const auto Newsboat_config_dir = config_dir + "Newsboat/";
				mock_Newsboat_xdg_dirs(
					Newsboat_config_dir,
					data_dir + "Newsboat/",
					boat_sentries);
				check(Newsboat_config_dir + "urls");
			}
		}
	}
}

TEST_CASE("try_migrate_from_newsbeuter() migrates Newsbeuter dotdir from "
	"default location to default location of Newsboat dotdir",
	"[ConfigPaths]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());
	INFO("Temporary directory (used as HOME): " << tmp.get_path());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	FileSentries sentries;

	mock_newsbeuter_dotdir(tmp, sentries);

	ConfigPaths paths;
	REQUIRE(paths.initialized());

	// Files should be migrated, so should return true.
	REQUIRE(paths.try_migrate_from_newsbeuter());

	const auto dotdir = tmp.get_path() + ".Newsboat/";

	REQUIRE(test_helpers::file_contents(dotdir + "config").at(0) == sentries.config);
	REQUIRE(test_helpers::file_contents(dotdir + "urls").at(0) == sentries.urls);
	REQUIRE(test_helpers::file_contents(dotdir + "cache.db").at(
			0) == sentries.cache);
	REQUIRE(test_helpers::file_contents(dotdir + "queue").at(0) == sentries.queue);
	REQUIRE(test_helpers::file_contents(dotdir + "history.search").at(0) ==
		sentries.search);
	REQUIRE(test_helpers::file_contents(dotdir + "history.cmdline").at(0) ==
		sentries.cmdline);
}

TEST_CASE("try_migrate_from_newsbeuter() migrates Newsbeuter XDG dirs from "
	"their default location to default locations of Newsboat XDG dirs",
	"[ConfigPaths]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());
	INFO("Temporary directory (used as HOME): " << tmp.get_path());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	FileSentries sentries;

	const auto check =
		[&]
	(const std::string& config_dir, const std::string& data_dir) {
		ConfigPaths paths;
		REQUIRE(paths.initialized());

		// Files should be migrated, so should return true.
		REQUIRE(paths.try_migrate_from_newsbeuter());

		REQUIRE(test_helpers::file_contents(config_dir + "config").at(
				0) == sentries.config);
		REQUIRE(test_helpers::file_contents(config_dir + "urls").at(0) == sentries.urls);

		REQUIRE(test_helpers::file_contents(data_dir + "cache.db").at(
				0) == sentries.cache);
		REQUIRE(test_helpers::file_contents(data_dir + "queue").at(0) == sentries.queue);
		REQUIRE(test_helpers::file_contents(data_dir + "history.search").at(0) ==
			sentries.search);
		REQUIRE(test_helpers::file_contents(data_dir + "history.cmdline").at(0) ==
			sentries.cmdline);
	};

	SECTION("Default XDG locations") {
		mock_newsbeuter_xdg_dirs(tmp, sentries);
		const auto config_dir = tmp.get_path() + ".config/Newsboat/";
		const auto data_dir = tmp.get_path() + ".local/share/Newsboat/";
		check(config_dir, data_dir);
	}

	SECTION("XDG_CONFIG_HOME redefined") {
		const auto config_dir = tmp.get_path() + "xdg-conf/";
		REQUIRE(::mkdir(config_dir.c_str(), 0700) == 0);
		xdg_config.set(config_dir);
		mock_newsbeuter_xdg_dirs(
			config_dir + "newsbeuter/",
			tmp.get_path() + ".local/share/newsbeuter/",
			sentries);

		check(config_dir + "Newsboat/",
			tmp.get_path() + ".local/share/Newsboat/");
	}

	SECTION("XDG_DATA_HOME redefined") {
		const auto data_dir = tmp.get_path() + "xdg-data/";
		REQUIRE(::mkdir(data_dir.c_str(), 0700) == 0);
		xdg_data.set(data_dir);
		mock_newsbeuter_xdg_dirs(
			tmp.get_path() + ".config/newsbeuter/",
			data_dir + "newsbeuter/",
			sentries);

		check(tmp.get_path() + ".config/Newsboat/",
			data_dir + "Newsboat/");
	}

	SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
		const auto config_dir = tmp.get_path() + "xdg-conf/";
		REQUIRE(::mkdir(config_dir.c_str(), 0700) == 0);
		xdg_config.set(config_dir);

		const auto data_dir = tmp.get_path() + "xdg-data/";
		REQUIRE(::mkdir(data_dir.c_str(), 0700) == 0);
		xdg_data.set(data_dir);

		mock_newsbeuter_xdg_dirs(
			config_dir + "newsbeuter/",
			data_dir + "newsbeuter/",
			sentries);

		check(config_dir + "Newsboat/", data_dir + "Newsboat/");
	}
}

void verify_xdg_not_migrated(
	const std::string& config_dir,
	const std::string& data_dir)
{
	ConfigPaths paths;
	REQUIRE(paths.initialized());

	// Shouldn't migrate anything, so should return false.
	REQUIRE_FALSE(paths.try_migrate_from_newsbeuter());

	REQUIRE_FALSE(0 == ::access((config_dir + "config").c_str(), R_OK));
	REQUIRE_FALSE(0 == ::access((config_dir + "urls").c_str(), R_OK));

	REQUIRE_FALSE(0 == ::access((data_dir + "cache.db").c_str(), R_OK));
	REQUIRE_FALSE(0 == ::access((data_dir + "queue").c_str(), R_OK));
	REQUIRE_FALSE(0 == ::access((data_dir + "history.search").c_str(), R_OK));
	REQUIRE_FALSE(0 == ::access((data_dir + "history.cmdline").c_str(), R_OK));
}

TEST_CASE("try_migrate_from_newsbeuter() doesn't migrate files if empty "
	"Newsboat XDG config dir already exists",
	"[ConfigPaths]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());
	INFO("Temporary directory (used as HOME): " << tmp.get_path());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	FileSentries sentries;

	SECTION("Default XDG locations") {
		mock_newsbeuter_xdg_dirs(tmp, sentries);

		const auto config_dir = tmp.get_path() + ".config/Newsboat/";
		REQUIRE(::mkdir(config_dir.c_str(), 0700) == 0);

		const auto data_dir = tmp.get_path() + ".local/share/Newsboat/";
		verify_xdg_not_migrated(config_dir, data_dir);
	}

	SECTION("XDG_CONFIG_HOME redefined") {
		const auto config_home = tmp.get_path() + "xdg-conf/";
		REQUIRE(::mkdir(config_home.c_str(), 0700) == 0);
		xdg_config.set(config_home);
		mock_newsbeuter_xdg_dirs(
			config_home + "newsbeuter/",
			tmp.get_path() + ".local/share/newsbeuter/",
			sentries);

		const auto config_dir = config_home + "Newsboat/";
		REQUIRE(::mkdir(config_dir.c_str(), 0700) == 0);
		verify_xdg_not_migrated(config_dir, tmp.get_path() + ".local/share/Newsboat/");
	}

	SECTION("XDG_DATA_HOME redefined") {
		const auto data_dir = tmp.get_path() + "xdg-data/";
		REQUIRE(::mkdir(data_dir.c_str(), 0700) == 0);
		xdg_data.set(data_dir);
		mock_newsbeuter_xdg_dirs(
			tmp.get_path() + ".config/newsbeuter/",
			data_dir + "newsbeuter/",
			sentries);

		const auto config_dir = tmp.get_path() + ".config/Newsboat/";
		REQUIRE(::mkdir(config_dir.c_str(), 0700) == 0);
		verify_xdg_not_migrated(config_dir, data_dir + "Newsboat/");
	}

	SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
		const auto config_home = tmp.get_path() + "xdg-conf/";
		REQUIRE(::mkdir(config_home.c_str(), 0700) == 0);
		xdg_config.set(config_home);

		const auto data_dir = tmp.get_path() + "xdg-data/";
		REQUIRE(::mkdir(data_dir.c_str(), 0700) == 0);
		xdg_data.set(data_dir);

		mock_newsbeuter_xdg_dirs(
			config_home + "newsbeuter/",
			data_dir + "newsbeuter/",
			sentries);

		const auto config_dir = config_home + "Newsboat/";
		REQUIRE(::mkdir(config_dir.c_str(), 0700) == 0);
		verify_xdg_not_migrated(config_dir, data_dir + "Newsboat/");
	}
}

TEST_CASE("try_migrate_from_newsbeuter() doesn't migrate files if empty "
	"Newsboat XDG data dir already exists",
	"[ConfigPaths]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());
	INFO("Temporary directory (used as HOME): " << tmp.get_path());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	FileSentries sentries;

	SECTION("Default XDG locations") {
		mock_newsbeuter_xdg_dirs(tmp, sentries);

		const auto config_dir = tmp.get_path() + ".config/Newsboat/";
		const auto data_dir = tmp.get_path() + ".local/share/Newsboat/";
		REQUIRE(::mkdir(data_dir.c_str(), 0700) == 0);
		verify_xdg_not_migrated(config_dir, data_dir);
	}

	SECTION("XDG_CONFIG_HOME redefined") {
		const auto config_home = tmp.get_path() + "xdg-conf/";
		REQUIRE(::mkdir(config_home.c_str(), 0700) == 0);
		xdg_config.set(config_home);
		mock_newsbeuter_xdg_dirs(
			config_home + "newsbeuter/",
			tmp.get_path() + ".local/share/newsbeuter/",
			sentries);

		const auto data_dir = tmp.get_path() + ".local/share/Newsboat/";
		REQUIRE(::mkdir(data_dir.c_str(), 0700) == 0);
		verify_xdg_not_migrated(config_home + "Newsboat/", data_dir);
	}

	SECTION("XDG_DATA_HOME redefined") {
		const auto data_home = tmp.get_path() + "xdg-data/";
		REQUIRE(::mkdir(data_home.c_str(), 0700) == 0);
		xdg_data.set(data_home);
		mock_newsbeuter_xdg_dirs(
			tmp.get_path() + ".config/newsbeuter/",
			data_home + "newsbeuter/",
			sentries);

		const auto data_dir = data_home + "Newsboat/";
		REQUIRE(::mkdir(data_dir.c_str(), 0700) == 0);
		verify_xdg_not_migrated(tmp.get_path() + ".config/Newsboat/", data_dir);
	}

	SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
		const auto config_home = tmp.get_path() + "xdg-conf/";
		REQUIRE(::mkdir(config_home.c_str(), 0700) == 0);
		xdg_config.set(config_home);

		const auto data_home = tmp.get_path() + "xdg-data/";
		REQUIRE(::mkdir(data_home.c_str(), 0700) == 0);
		xdg_data.set(data_home);

		mock_newsbeuter_xdg_dirs(
			config_home + "newsbeuter/",
			data_home + "newsbeuter/",
			sentries);

		const auto data_dir = data_home + "Newsboat/";
		REQUIRE(::mkdir(data_dir.c_str(), 0700) == 0);
		verify_xdg_not_migrated(config_home + "Newsboat/", data_dir);
	}
}

TEST_CASE("try_migrate_from_newsbeuter() doesn't migrate files if Newsboat XDG "
	"config dir couldn't be created",
	"[ConfigPaths]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());
	INFO("Temporary directory (used as HOME): " << tmp.get_path());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	FileSentries sentries;

	SECTION("Default XDG locations") {
		mock_newsbeuter_xdg_dirs(tmp, sentries);

		// Making XDG .config unwriteable makes it impossible to create
		// a directory there
		const auto config_home = tmp.get_path() + ".config/";
		test_helpers::Chmod config_home_chmod(config_home, S_IRUSR | S_IXUSR);

		const auto config_dir = config_home + "Newsboat/";
		const auto data_dir = tmp.get_path() + ".local/share/Newsboat/";
		verify_xdg_not_migrated(config_dir, data_dir);
	}

	SECTION("XDG_CONFIG_HOME redefined") {
		const auto config_home = tmp.get_path() + "xdg-conf/";
		REQUIRE(::mkdir(config_home.c_str(), 0700) == 0);
		xdg_config.set(config_home);
		mock_newsbeuter_xdg_dirs(
			config_home + "newsbeuter/",
			tmp.get_path() + ".local/share/newsbeuter/",
			sentries);

		// Making XDG .config unwriteable makes it impossible to create
		// a directory there
		test_helpers::Chmod config_home_chmod(config_home, S_IRUSR | S_IXUSR);

		verify_xdg_not_migrated(
			config_home + "Newsboat/",
			tmp.get_path() + ".local/share/Newsboat/");
	}

	SECTION("XDG_DATA_HOME redefined") {
		const auto data_dir = tmp.get_path() + "xdg-data/";
		REQUIRE(::mkdir(data_dir.c_str(), 0700) == 0);
		xdg_data.set(data_dir);
		mock_newsbeuter_xdg_dirs(
			tmp.get_path() + ".config/newsbeuter/",
			data_dir + "newsbeuter/",
			sentries);

		// Making XDG .config unwriteable makes it impossible to create
		// a directory there
		const auto config_home = tmp.get_path() + ".config/";
		test_helpers::Chmod config_home_chmod(config_home, S_IRUSR | S_IXUSR);

		verify_xdg_not_migrated(
			config_home + "Newsboat/",
			data_dir + "Newsboat/");
	}

	SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
		const auto config_home = tmp.get_path() + "xdg-conf/";
		REQUIRE(::mkdir(config_home.c_str(), 0700) == 0);
		xdg_config.set(config_home);

		const auto data_dir = tmp.get_path() + "xdg-data/";
		REQUIRE(::mkdir(data_dir.c_str(), 0700) == 0);
		xdg_data.set(data_dir);

		mock_newsbeuter_xdg_dirs(
			config_home + "newsbeuter/",
			data_dir + "newsbeuter/",
			sentries);

		// Making XDG .config unwriteable makes it impossible to create
		// a directory there
		test_helpers::Chmod config_home_chmod(config_home, S_IRUSR | S_IXUSR);

		verify_xdg_not_migrated(
			config_home + "Newsboat/",
			data_dir + "Newsboat/");
	}
}

TEST_CASE("try_migrate_from_newsbeuter() doesn't migrate files if Newsboat XDG "
	"data dir couldn't be created",
	"[ConfigPaths]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());
	INFO("Temporary directory (used as HOME): " << tmp.get_path());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	FileSentries sentries;

	SECTION("Default XDG locations") {
		mock_newsbeuter_xdg_dirs(tmp, sentries);

		// Making XDG .local/share unwriteable makes it impossible to create
		// a directory there
		const auto data_home = tmp.get_path() + ".local/share/";
		test_helpers::Chmod data_home_chmod(data_home, S_IRUSR | S_IXUSR);

		const auto config_dir = tmp.get_path() + ".config/Newsboat/";
		const auto data_dir = data_home + "Newsboat/";
		verify_xdg_not_migrated(config_dir, data_dir);
	}

	SECTION("XDG_CONFIG_HOME redefined") {
		const auto config_home = tmp.get_path() + "xdg-conf/";
		REQUIRE(::mkdir(config_home.c_str(), 0700) == 0);
		xdg_config.set(config_home);
		mock_newsbeuter_xdg_dirs(
			config_home + "newsbeuter/",
			tmp.get_path() + ".local/share/newsbeuter/",
			sentries);

		// Making XDG .local/share unwriteable makes it impossible to create
		// a directory there
		const auto data_home = tmp.get_path() + ".local/share/";
		test_helpers::Chmod data_home_chmod(data_home, S_IRUSR | S_IXUSR);

		verify_xdg_not_migrated(
			config_home + "Newsboat/",
			data_home + "Newsboat/");
	}

	SECTION("XDG_DATA_HOME redefined") {
		const auto data_home = tmp.get_path() + "xdg-data/";
		REQUIRE(::mkdir(data_home.c_str(), 0700) == 0);
		xdg_data.set(data_home);
		mock_newsbeuter_xdg_dirs(
			tmp.get_path() + ".config/newsbeuter/",
			data_home + "newsbeuter/",
			sentries);

		// Making XDG .local/share unwriteable makes it impossible to create
		// a directory there
		test_helpers::Chmod data_home_chmod(data_home, S_IRUSR | S_IXUSR);

		verify_xdg_not_migrated(
			tmp.get_path() + ".config/Newsboat/",
			data_home + "Newsboat/");
	}

	SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
		const auto config_home = tmp.get_path() + "xdg-conf/";
		REQUIRE(::mkdir(config_home.c_str(), 0700) == 0);
		xdg_config.set(config_home);

		const auto data_home = tmp.get_path() + "xdg-data/";
		REQUIRE(::mkdir(data_home.c_str(), 0700) == 0);
		xdg_data.set(data_home);

		mock_newsbeuter_xdg_dirs(
			config_home + "newsbeuter/",
			data_home + "newsbeuter/",
			sentries);

		// Making XDG .local/share unwriteable makes it impossible to create
		// a directory there
		test_helpers::Chmod data_home_chmod(data_home, S_IRUSR | S_IXUSR);

		verify_xdg_not_migrated(
			config_home + "Newsboat/",
			data_home + "Newsboat/");
	}
}

void verify_dotdir_not_migrated(const std::string& dotdir)
{
	ConfigPaths paths;
	REQUIRE(paths.initialized());

	// Shouldn't migrate anything, so should return false.
	REQUIRE_FALSE(paths.try_migrate_from_newsbeuter());

	REQUIRE_FALSE(0 == ::access((dotdir + "config").c_str(), R_OK));
	REQUIRE_FALSE(0 == ::access((dotdir + "urls").c_str(), R_OK));

	REQUIRE_FALSE(0 == ::access((dotdir + "cache.db").c_str(), R_OK));
	REQUIRE_FALSE(0 == ::access((dotdir + "queue").c_str(), R_OK));
	REQUIRE_FALSE(0 == ::access((dotdir + "history.search").c_str(), R_OK));
	REQUIRE_FALSE(0 == ::access((dotdir + "history.cmdline").c_str(), R_OK));
}

TEST_CASE("try_migrate_from_newsbeuter() doesn't migrate files if empty "
	"Newsboat dotdir already exists",
	"[ConfigPaths]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());
	INFO("Temporary directory (used as HOME): " << tmp.get_path());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	FileSentries sentries;

	mock_newsbeuter_dotdir(tmp, sentries);

	const auto dotdir = tmp.get_path() + ".Newsboat/";
	REQUIRE(::mkdir(dotdir.c_str(), 0700) == 0);

	verify_dotdir_not_migrated(dotdir);
}

TEST_CASE("try_migrate_from_newsbeuter() doesn't migrate files if Newsboat "
	"dotdir couldn't be created",
	"[ConfigPaths]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());
	INFO("Temporary directory (used as HOME): " << tmp.get_path());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	FileSentries sentries;

	mock_newsbeuter_dotdir(tmp, sentries);

	// Making home directory unwriteable makes it impossible to create
	// a directory there
	test_helpers::Chmod dotdir_chmod(tmp.get_path(), S_IRUSR | S_IXUSR);

	verify_dotdir_not_migrated(tmp.get_path() + ".Newsboat/");
}

void verify_create_dirs_returns_false(const test_helpers::TempDir& tmp)
{
	const mode_t readonly = S_IRUSR | S_IXUSR;
	test_helpers::Chmod home_chmod(tmp.get_path(), readonly);

	ConfigPaths paths;
	REQUIRE(paths.initialized());
	REQUIRE_FALSE(paths.create_dirs());
}

TEST_CASE("create_dirs() returns false if dotdir doesn't exist and couldn't "
	"be created",
	"[ConfigPaths]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());
	INFO("Temporary directory (used as HOME): " << tmp.get_path());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	verify_create_dirs_returns_false(tmp);
}

TEST_CASE("create_dirs() returns false if XDG config dir exists but data dir "
	"doesn't exist and couldn't be created",
	"[ConfigPaths]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());
	INFO("Temporary directory (used as HOME): " << tmp.get_path());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	SECTION("Default XDG locations") {
		const auto config_dir = tmp.get_path() + ".config/Newsboat/";
		REQUIRE(utils::mkdir_parents(config_dir, 0700) == 0);

		verify_create_dirs_returns_false(tmp);
	}

	SECTION("XDG_CONFIG_HOME redefined") {
		const auto config_home = tmp.get_path() + "xdg-cfg/";
		xdg_config.set(config_home);

		const auto config_dir = config_home + "Newsboat/";
		REQUIRE(utils::mkdir_parents(config_dir, 0700) == 0);

		verify_create_dirs_returns_false(tmp);
	}

	SECTION("XDG_DATA_HOME redefined") {
		const auto config_dir = tmp.get_path() + ".config/Newsboat/";
		REQUIRE(utils::mkdir_parents(config_dir, 0700) == 0);

		const auto data_home = tmp.get_path() + "xdg-data/";
		xdg_data.set(data_home);
		// It's important to set the variable, but *not* create the directory
		// - it's the pre-condition of the test that the data dir doesn't exist

		verify_create_dirs_returns_false(tmp);
	}

	SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
		const auto config_home = tmp.get_path() + "xdg-cfg/";
		xdg_config.set(config_home);

		const auto config_dir = config_home + "Newsboat/";
		REQUIRE(utils::mkdir_parents(config_dir, 0700) == 0);

		const auto data_home = tmp.get_path() + "xdg-data/";
		xdg_data.set(data_home);
		// It's important to set the variable, but *not* create the directory
		// - it's the pre-condition of the test that the data dir doesn't exist

		verify_create_dirs_returns_false(tmp);
	}
}

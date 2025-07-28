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

using namespace newsboat;

TEST_CASE("ConfigPaths returns paths to Newsboat dotdir if no Newsboat dirs "
	"exist",
	"[ConfigPaths]")
{
	test_helpers::TempDir tmp;
	const auto newsboat_dir = tmp.get_path().join(".newsboat"_path);
	REQUIRE(0 == utils::mkdir_parents(newsboat_dir, 0700));

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path().to_locale_string());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	ConfigPaths paths;
	REQUIRE(paths.initialized());
	REQUIRE(paths.url_file() == newsboat_dir.join("urls"_path));
	REQUIRE(paths.cache_file() == newsboat_dir.join("cache.db"_path));
	REQUIRE(paths.lock_file() == newsboat_dir.join("cache.db.lock"_path));
	REQUIRE(paths.config_file() == newsboat_dir.join("config"_path));
	REQUIRE(paths.queue_file() == newsboat_dir.join("queue"_path));
	REQUIRE(paths.search_history_file() == newsboat_dir.join("history.search"_path));
	REQUIRE(paths.cmdline_history_file() == newsboat_dir.join("history.cmdline"_path));
}

TEST_CASE("ConfigPaths returns paths to Newsboat XDG dirs if they exist and "
	"the dotdir doesn't",
	"[ConfigPaths]")
{
	test_helpers::TempDir tmp;
	const auto config_dir = tmp.get_path().join(".config/newsboat"_path);
	REQUIRE(0 == utils::mkdir_parents(config_dir, 0700));
	const auto data_dir = tmp.get_path().join(".local/share/newsboat"_path);
	REQUIRE(0 == utils::mkdir_parents(data_dir, 0700));

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path().to_locale_string());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	const auto check = [&]() {
		ConfigPaths paths;
		REQUIRE(paths.initialized());
		REQUIRE(paths.config_file() == config_dir.join("config"_path));
		REQUIRE(paths.url_file() == config_dir.join("urls"_path));
		REQUIRE(paths.cache_file() == data_dir.join("cache.db"_path));
		REQUIRE(paths.lock_file() == data_dir.join("cache.db.lock"_path));
		REQUIRE(paths.queue_file() == data_dir.join("queue"_path));
		REQUIRE(paths.search_history_file() == data_dir.join("history.search"_path));
		REQUIRE(paths.cmdline_history_file() == data_dir.join("history.cmdline"_path));
	};

	SECTION("XDG_CONFIG_HOME is set") {
		test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
		xdg_config.set(tmp.get_path().join(".config"_path).to_locale_string());

		SECTION("XDG_DATA_HOME is set") {
			test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
			xdg_data.set(tmp.get_path().join(".local/share"_path).to_locale_string());
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
			xdg_data.set(tmp.get_path().join(".local/share"_path).to_locale_string());
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

	const auto url_file = "my urls file"_path;
	const auto cache_file = "/path/to/cache file.db"_path;
	auto lock_file = cache_file;
	lock_file.add_extension("lock");
	const auto config_file = "this is a/config"_path;
	test_helpers::Opts opts({
		"newsboat",
		"-u", url_file.to_locale_string(),
		"-c", cache_file.to_locale_string(),
		"-C", config_file.to_locale_string(),
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
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path().to_locale_string());
	INFO("Temporary directory (used as HOME): " << tmp.get_path());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	ConfigPaths paths;
	REQUIRE(paths.initialized());

	const auto newsboat_dir = tmp.get_path().join(".newsboat"_path);
	REQUIRE(paths.cache_file() == newsboat_dir.join("cache.db"_path));
	REQUIRE(paths.lock_file() == newsboat_dir.join("cache.db.lock"_path));

	const auto new_cache = "something/entirely different.sqlite3"_path;
	paths.set_cache_file(new_cache);
	REQUIRE(paths.cache_file() == new_cache);
	auto expected_lock_file = new_cache;
	expected_lock_file.add_extension("lock");
	REQUIRE(paths.lock_file() == expected_lock_file);
}

TEST_CASE("ConfigPaths::create_dirs() returns true if both config and data dirs "
	"were successfully created or already existed", "[ConfigPaths]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path().to_locale_string());
	INFO("Temporary directory (used as HOME): " << tmp.get_path());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	const auto require_exists = [&tmp](std::vector<Filepath> dirs) {
		ConfigPaths paths;
		REQUIRE(paths.initialized());

		paths.url_file().starts_with(tmp.get_path());
		paths.config_file().starts_with(tmp.get_path());
		paths.cache_file().starts_with(tmp.get_path());
		paths.lock_file().starts_with(tmp.get_path());
		paths.queue_file().starts_with(tmp.get_path());
		paths.search_history_file().starts_with(tmp.get_path());
		paths.cmdline_history_file().starts_with(tmp.get_path());

		REQUIRE(paths.create_dirs());

		for (const auto& dir : dirs) {
			INFO("Checking directory " << dir);
			const auto dir_exists = 0 == ::access(dir.to_locale_string().c_str(), R_OK | X_OK);
			REQUIRE(dir_exists);
		}
	};

	const auto dotdir = tmp.get_path().join(".newsboat"_path);
	const auto default_config_dir = tmp.get_path().join(".config/newsboat"_path);
	const auto default_data_dir = tmp.get_path().join(".local/share/newsboat"_path);

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
				tmp.get_path().join(Filepath::from_locale_string("config" + std::to_string(rand())));
			INFO("Config home is " << config_home);
			const auto config_dir = config_home.join("newsboat"_path);

			test_helpers::EnvVar xdg_config_home("XDG_CONFIG_HOME");
			xdg_config_home.set(config_home.to_locale_string());

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
				tmp.get_path().join(Filepath::from_locale_string("data" + std::to_string(rand())));
			INFO("Data home is " << data_home);
			const auto data_dir = data_home.join("newsboat"_path);

			test_helpers::EnvVar xdg_data_home("XDG_DATA_HOME");
			xdg_data_home.set(data_home.to_locale_string());

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
				tmp.get_path().join(Filepath::from_locale_string("config" + std::to_string(rand())));
			INFO("Config home is " << config_home);
			const auto config_dir = config_home.join("newsboat"_path);

			const auto data_home =
				tmp.get_path().join(Filepath::from_locale_string("data" + std::to_string(rand())));
			INFO("Data home is " << data_home);
			const auto data_dir = data_home.join("newsboat"_path);

			test_helpers::EnvVar xdg_config_home("XDG_CONFIG_HOME");
			xdg_config_home.set(config_home.to_locale_string());

			test_helpers::EnvVar xdg_data_home("XDG_DATA_HOME");
			xdg_data_home.set(data_home.to_locale_string());

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
bool create_file(const Filepath& filepath, const std::string& contents)
{
	std::ofstream out(filepath.to_locale_string(), std::ios::trunc);
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
	const auto dotdir_path = tmp.get_path().join(".newsbeuter"_path);
	REQUIRE(test_helpers::mkdir(dotdir_path, 0700) == 0);
	REQUIRE(create_file(dotdir_path.join("config"_path), sentries.config));
	REQUIRE(create_file(dotdir_path.join("urls"_path), sentries.urls));
	REQUIRE(create_file(dotdir_path.join("cache.db"_path), sentries.cache));
	REQUIRE(create_file(dotdir_path.join("queue"_path), sentries.queue));
	REQUIRE(create_file(dotdir_path.join("history.search"_path), sentries.search));
	REQUIRE(create_file(dotdir_path.join("history.cmdline"_path), sentries.cmdline));
}

void mock_newsbeuter_xdg_dirs(
	const Filepath& config_dir_path,
	const Filepath& data_dir_path,
	const FileSentries& sentries)
{
	REQUIRE(utils::mkdir_parents(config_dir_path, 0700) == 0);
	REQUIRE(create_file(config_dir_path.join("config"_path), sentries.config));
	REQUIRE(create_file(config_dir_path.join("urls"_path), sentries.urls));

	REQUIRE(utils::mkdir_parents(data_dir_path, 0700) == 0);
	REQUIRE(create_file(data_dir_path.join("cache.db"_path), sentries.cache));
	REQUIRE(create_file(data_dir_path.join("queue"_path), sentries.queue));
	REQUIRE(create_file(data_dir_path.join("history.search"_path), sentries.search));
	REQUIRE(create_file(data_dir_path.join("history.cmdline"_path), sentries.cmdline));
}

void mock_newsbeuter_xdg_dirs(
	const test_helpers::TempDir& tmp,
	const FileSentries& sentries)
{
	const auto config_dir_path = tmp.get_path().join(".config/newsbeuter"_path);
	const auto data_dir_path = tmp.get_path().join(".local/share/newsbeuter"_path);
	mock_newsbeuter_xdg_dirs(config_dir_path, data_dir_path, sentries);
}

void mock_newsboat_dotdir(
	const test_helpers::TempDir& tmp,
	const FileSentries& sentries)
{
	const auto dotdir_path = tmp.get_path().join(".newsboat"_path);
	REQUIRE(test_helpers::mkdir(dotdir_path, 0700) == 0);

	const auto urls_file = dotdir_path.join("urls"_path);
	REQUIRE(create_file(urls_file, sentries.urls));
}

void mock_newsboat_xdg_dirs(
	const Filepath& config_dir_path,
	const Filepath& /*data_dir_path*/,
	const FileSentries& sentries)
{
	REQUIRE(utils::mkdir_parents(config_dir_path, 0700) == 0);

	const auto urls_file = config_dir_path.join("urls"_path);
	REQUIRE(create_file(urls_file, sentries.urls));
}

void mock_newsboat_xdg_dirs(
	const test_helpers::TempDir& tmp,
	const FileSentries& sentries)
{
	const auto config_dir_path = tmp.get_path().join(".config/newsboat"_path);
	const auto data_dir_path = tmp.get_path().join(".local/share/newsboat"_path);
	mock_newsboat_xdg_dirs(config_dir_path, data_dir_path, sentries);
}

TEST_CASE("try_migrate_from_newsbeuter() doesn't migrate if config paths "
	"were specified on the command line",
	"[ConfigPaths]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path().to_locale_string());
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

		const auto url_file = tmp.get_path().join("my urls file"_path);
		REQUIRE(create_file(url_file, boat_sentries.urls));

		const auto cache_file = tmp.get_path().join("new cache.db"_path);
		REQUIRE(create_file(cache_file, boat_sentries.cache));

		const auto config_file = tmp.get_path().join("custom config file"_path);
		REQUIRE(create_file(config_file, boat_sentries.config));

		test_helpers::Opts opts({
			"newsboat",
			"-u", url_file.to_locale_string(),
			"-c", cache_file.to_locale_string(),
			"-C", config_file.to_locale_string(),
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
	home.set(tmp.get_path().to_locale_string());
	INFO("Temporary directory (used as HOME): " << tmp.get_path());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	FileSentries beuter_sentries;
	FileSentries boat_sentries;

	const auto check = [&](const Filepath& urls_filepath) {
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
			mock_newsboat_dotdir(tmp, boat_sentries);
			check(tmp.get_path().join(".newsboat/urls"_path));
		}

		SECTION("Newsboat uses XDG") {
			SECTION("Default XDG locations") {
				mock_newsboat_xdg_dirs(tmp, boat_sentries);
				check(tmp.get_path().join(".config/newsboat/urls"_path));
			}

			SECTION("XDG_CONFIG_HOME redefined") {
				const auto config_dir = tmp.get_path().join("xdg-conf"_path);
				REQUIRE(test_helpers::mkdir(config_dir, 0700) == 0);
				xdg_config.set(config_dir.to_locale_string());
				const auto newsboat_config_dir = config_dir.join("newsboat"_path);
				mock_newsboat_xdg_dirs(
					newsboat_config_dir,
					tmp.get_path().join(".local/share/newsboat"_path),
					boat_sentries);
				check(newsboat_config_dir.join("urls"_path));
			}

			SECTION("XDG_DATA_HOME redefined") {
				const auto data_dir = tmp.get_path().join("xdg-data"_path);
				REQUIRE(test_helpers::mkdir(data_dir, 0700) == 0);
				xdg_data.set(data_dir.to_locale_string());
				const auto newsboat_config_dir = tmp.get_path().join(".config/newsboat"_path);
				const auto newsboat_data_dir = data_dir.join("newsboat"_path);
				mock_newsboat_xdg_dirs(
					newsboat_config_dir,
					newsboat_data_dir,
					boat_sentries);
				check(newsboat_config_dir.join("urls"_path));
			}

			SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
				const auto config_dir = tmp.get_path().join("xdg-conf"_path);
				REQUIRE(test_helpers::mkdir(config_dir, 0700) == 0);
				xdg_config.set(config_dir.to_locale_string());

				const auto data_dir = tmp.get_path().join("xdg-data"_path);
				REQUIRE(test_helpers::mkdir(data_dir, 0700) == 0);
				xdg_data.set(data_dir.to_locale_string());

				const auto newsboat_config_dir = config_dir.join("newsboat"_path);
				const auto newsboat_data_dir = data_dir.join("newsboat"_path);

				mock_newsboat_xdg_dirs(
					newsboat_config_dir,
					newsboat_data_dir,
					boat_sentries);

				check(newsboat_config_dir.join("urls"_path));
			}
		}
	}

	SECTION("Newsbeuter XDG dirs exist") {
		SECTION("Default XDG locations") {
			mock_newsbeuter_xdg_dirs(tmp, beuter_sentries);

			SECTION("Newsboat uses dotdir") {
				mock_newsboat_dotdir(tmp, boat_sentries);
				check(tmp.get_path().join(".newsboat/urls"_path));
			}

			SECTION("Newsboat uses XDG") {
				mock_newsboat_xdg_dirs(tmp, boat_sentries);
				check(tmp.get_path().join(".config/newsboat/urls"_path));
			}
		}

		SECTION("XDG_CONFIG_HOME redefined") {
			const auto config_dir = tmp.get_path().join("xdg-conf"_path);
			REQUIRE(test_helpers::mkdir(config_dir, 0700) == 0);
			xdg_config.set(config_dir.to_locale_string());
			mock_newsbeuter_xdg_dirs(
				config_dir.join("newsbeuter"_path),
				tmp.get_path().join(".local/share/newsbeuter"_path),
				beuter_sentries);

			SECTION("Newsboat uses dotdir") {
				mock_newsboat_dotdir(tmp, boat_sentries);
				check(tmp.get_path().join(".newsboat/urls"_path));
			}

			SECTION("Newsboat uses XDG") {
				const auto newsboat_config_dir = config_dir.join("newsboat"_path);
				mock_newsboat_xdg_dirs(
					newsboat_config_dir,
					tmp.get_path().join(".local/share/newsboat"_path),
					boat_sentries);
				check(newsboat_config_dir.join("urls"_path));
			}
		}

		SECTION("XDG_DATA_HOME redefined") {
			const auto data_dir = tmp.get_path().join("xdg-data"_path);
			REQUIRE(test_helpers::mkdir(data_dir, 0700) == 0);
			xdg_data.set(data_dir.to_locale_string());
			mock_newsbeuter_xdg_dirs(
				tmp.get_path().join(".config/newsbeuter"_path),
				data_dir,
				beuter_sentries);

			SECTION("Newsboat uses dotdir") {
				mock_newsboat_dotdir(tmp, boat_sentries);
				check(tmp.get_path().join(".newsboat/urls"_path));
			}

			SECTION("Newsboat uses XDG") {
				const auto newsboat_config_dir =
					tmp.get_path().join(".config/newsboat"_path);
				mock_newsboat_xdg_dirs(
					newsboat_config_dir,
					data_dir.join("newsboat"_path),
					boat_sentries);
				check(newsboat_config_dir.join("urls"_path));
			}
		}

		SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
			const auto config_dir = tmp.get_path().join("xdg-conf"_path);
			REQUIRE(test_helpers::mkdir(config_dir, 0700) == 0);
			xdg_config.set(config_dir.to_locale_string());

			const auto data_dir = tmp.get_path().join("xdg-data"_path);
			REQUIRE(test_helpers::mkdir(data_dir, 0700) == 0);
			xdg_data.set(data_dir.to_locale_string());

			mock_newsbeuter_xdg_dirs(
				config_dir.join("newsbeuter"_path),
				data_dir.join("newsbeuter"_path),
				beuter_sentries);

			SECTION("Newsboat uses dotdir") {
				mock_newsboat_dotdir(tmp, boat_sentries);
				check(tmp.get_path().join(".newsboat/urls"_path));
			}

			SECTION("Newsboat uses XDG") {
				const auto newsboat_config_dir = config_dir.join("newsboat"_path);
				mock_newsboat_xdg_dirs(
					newsboat_config_dir,
					data_dir.join("newsboat"_path),
					boat_sentries);
				check(newsboat_config_dir.join("urls"_path));
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
	home.set(tmp.get_path().to_locale_string());
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

	const auto dotdir = tmp.get_path().join(".newsboat"_path);

	REQUIRE(test_helpers::file_contents(dotdir.join("config"_path)).at(0) == sentries.config);
	REQUIRE(test_helpers::file_contents(dotdir.join("urls"_path)).at(0) == sentries.urls);
	REQUIRE(test_helpers::file_contents(dotdir.join("cache.db"_path)).at(0) == sentries.cache);
	REQUIRE(test_helpers::file_contents(dotdir.join("queue"_path)).at(0) == sentries.queue);
	REQUIRE(test_helpers::file_contents(dotdir.join("history.search"_path)).at(0)
		== sentries.search);
	REQUIRE(test_helpers::file_contents(dotdir.join("history.cmdline"_path)).at(0)
		== sentries.cmdline);
}

TEST_CASE("try_migrate_from_newsbeuter() migrates Newsbeuter XDG dirs from "
	"their default location to default locations of Newsboat XDG dirs",
	"[ConfigPaths]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path().to_locale_string());
	INFO("Temporary directory (used as HOME): " << tmp.get_path());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	FileSentries sentries;

	const auto check =
	[&](const Filepath& config_dir, const Filepath& data_dir) {
		ConfigPaths paths;
		REQUIRE(paths.initialized());

		// Files should be migrated, so should return true.
		REQUIRE(paths.try_migrate_from_newsbeuter());

		REQUIRE(test_helpers::file_contents(config_dir.join("config"_path)).at(0)
			== sentries.config);
		REQUIRE(test_helpers::file_contents(config_dir.join("urls"_path)).at(0) == sentries.urls);

		REQUIRE(test_helpers::file_contents(data_dir.join("cache.db"_path)).at(0)
			== sentries.cache);
		REQUIRE(test_helpers::file_contents(data_dir.join("queue"_path)).at(0) == sentries.queue);
		REQUIRE(test_helpers::file_contents(data_dir.join("history.search"_path)).at(0)
			== sentries.search);
		REQUIRE(test_helpers::file_contents(data_dir.join("history.cmdline"_path)).at(0)
			== sentries.cmdline);
	};

	SECTION("Default XDG locations") {
		mock_newsbeuter_xdg_dirs(tmp, sentries);
		const auto config_dir = tmp.get_path().join(".config/newsboat"_path);
		const auto data_dir = tmp.get_path().join(".local/share/newsboat"_path);
		check(config_dir, data_dir);
	}

	SECTION("XDG_CONFIG_HOME redefined") {
		const auto config_dir = tmp.get_path().join("xdg-conf"_path);
		REQUIRE(test_helpers::mkdir(config_dir, 0700) == 0);
		xdg_config.set(config_dir.to_locale_string());
		mock_newsbeuter_xdg_dirs(
			config_dir.join("newsbeuter"_path),
			tmp.get_path().join(".local/share/newsbeuter"_path),
			sentries);

		check(config_dir.join("newsboat"_path),
			tmp.get_path().join(".local/share/newsboat"_path));
	}

	SECTION("XDG_DATA_HOME redefined") {
		const auto data_dir = tmp.get_path().join("xdg-data"_path);
		REQUIRE(test_helpers::mkdir(data_dir, 0700) == 0);
		xdg_data.set(data_dir.to_locale_string());
		mock_newsbeuter_xdg_dirs(
			tmp.get_path().join(".config/newsbeuter"_path),
			data_dir.join("newsbeuter"_path),
			sentries);

		check(tmp.get_path().join(".config/newsboat"_path),
			data_dir.join("newsboat"_path));
	}

	SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
		const auto config_dir = tmp.get_path().join("xdg-conf"_path);
		REQUIRE(test_helpers::mkdir(config_dir, 0700) == 0);
		xdg_config.set(config_dir.to_locale_string());

		const auto data_dir = tmp.get_path().join("xdg-data"_path);
		REQUIRE(test_helpers::mkdir(data_dir, 0700) == 0);
		xdg_data.set(data_dir.to_locale_string());

		mock_newsbeuter_xdg_dirs(
			config_dir.join("newsbeuter"_path),
			data_dir.join("newsbeuter"_path),
			sentries);

		check(config_dir.join("newsboat"_path),
			data_dir.join("newsboat"_path));
	}
}

void verify_xdg_not_migrated(
	const Filepath& config_dir,
	const Filepath& data_dir)
{
	ConfigPaths paths;
	REQUIRE(paths.initialized());

	// Shouldn't migrate anything, so should return false.
	REQUIRE_FALSE(paths.try_migrate_from_newsbeuter());

	REQUIRE_FALSE(test_helpers::file_available_for_reading(config_dir.join("config"_path)));
	REQUIRE_FALSE(test_helpers::file_available_for_reading(config_dir.join("urls"_path)));

	REQUIRE_FALSE(test_helpers::file_available_for_reading(data_dir.join("cache.db"_path)));
	REQUIRE_FALSE(test_helpers::file_available_for_reading(data_dir.join("queue"_path)));
	REQUIRE_FALSE(test_helpers::file_available_for_reading(
			data_dir.join("history.search"_path)));
	REQUIRE_FALSE(test_helpers::file_available_for_reading(
			data_dir.join("history.cmdline"_path)));
}

TEST_CASE("try_migrate_from_newsbeuter() doesn't migrate files if empty "
	"Newsboat XDG config dir already exists",
	"[ConfigPaths]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path().to_locale_string());
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

		const auto config_dir = tmp.get_path().join(".config/newsboat"_path);
		REQUIRE(test_helpers::mkdir(config_dir, 0700) == 0);

		const auto data_dir = tmp.get_path().join(".local/share/newsboat"_path);
		verify_xdg_not_migrated(config_dir, data_dir);
	}

	SECTION("XDG_CONFIG_HOME redefined") {
		const auto config_home = tmp.get_path().join("xdg-conf"_path);
		REQUIRE(test_helpers::mkdir(config_home, 0700) == 0);
		xdg_config.set(config_home.to_locale_string());
		mock_newsbeuter_xdg_dirs(
			config_home.join("newsbeuter"_path),
			tmp.get_path().join(".local/share/newsbeuter"_path),
			sentries);

		const auto config_dir = config_home.join("newsboat"_path);
		REQUIRE(test_helpers::mkdir(config_dir, 0700) == 0);
		verify_xdg_not_migrated(config_dir,
			tmp.get_path().join(".local/share/newsboat"_path));
	}

	SECTION("XDG_DATA_HOME redefined") {
		const auto data_dir = tmp.get_path().join("xdg-data"_path);
		REQUIRE(test_helpers::mkdir(data_dir, 0700) == 0);
		xdg_data.set(data_dir.to_locale_string());
		mock_newsbeuter_xdg_dirs(
			tmp.get_path().join(".config/newsbeuter"_path),
			data_dir.join("newsbeuter"_path),
			sentries);

		const auto config_dir = tmp.get_path().join(".config/newsboat"_path);
		REQUIRE(test_helpers::mkdir(config_dir, 0700) == 0);
		verify_xdg_not_migrated(config_dir, data_dir.join("newsboat"_path));
	}

	SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
		const auto config_home = tmp.get_path().join("xdg-conf"_path);
		REQUIRE(test_helpers::mkdir(config_home, 0700) == 0);
		xdg_config.set(config_home.to_locale_string());

		const auto data_dir = tmp.get_path().join("xdg-data"_path);
		REQUIRE(test_helpers::mkdir(data_dir, 0700) == 0);
		xdg_data.set(data_dir.to_locale_string());

		mock_newsbeuter_xdg_dirs(
			config_home.join("newsbeuter"_path),
			data_dir.join("newsbeuter"_path),
			sentries);

		const auto config_dir = config_home.join("newsboat"_path);
		REQUIRE(test_helpers::mkdir(config_dir, 0700) == 0);
		verify_xdg_not_migrated(config_dir, data_dir.join("newsboat"_path));
	}
}

TEST_CASE("try_migrate_from_newsbeuter() doesn't migrate files if empty "
	"Newsboat XDG data dir already exists",
	"[ConfigPaths]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path().to_locale_string());
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

		const auto config_dir = tmp.get_path().join(".config/newsboat"_path);
		const auto data_dir = tmp.get_path().join(".local/share/newsboat"_path);
		REQUIRE(test_helpers::mkdir(data_dir, 0700) == 0);
		verify_xdg_not_migrated(config_dir, data_dir);
	}

	SECTION("XDG_CONFIG_HOME redefined") {
		const auto config_home = tmp.get_path().join("xdg-conf"_path);
		REQUIRE(test_helpers::mkdir(config_home, 0700) == 0);
		xdg_config.set(config_home.to_locale_string());
		mock_newsbeuter_xdg_dirs(
			config_home.join("newsbeuter"_path),
			tmp.get_path().join(".local/share/newsbeuter"_path),
			sentries);

		const auto data_dir = tmp.get_path().join(".local/share/newsboat"_path);
		REQUIRE(test_helpers::mkdir(data_dir, 0700) == 0);
		verify_xdg_not_migrated(config_home.join("newsboat"_path), data_dir);
	}

	SECTION("XDG_DATA_HOME redefined") {
		const auto data_home = tmp.get_path().join("xdg-data"_path);
		REQUIRE(test_helpers::mkdir(data_home, 0700) == 0);
		xdg_data.set(data_home.to_locale_string());
		mock_newsbeuter_xdg_dirs(
			tmp.get_path().join(".config/newsbeuter"_path),
			data_home.join("newsbeuter"_path),
			sentries);

		const auto data_dir = data_home.join("newsboat"_path);
		REQUIRE(test_helpers::mkdir(data_dir, 0700) == 0);
		verify_xdg_not_migrated(tmp.get_path().join(".config/newsboat"_path), data_dir);
	}

	SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
		const auto config_home = tmp.get_path().join("xdg-conf"_path);
		REQUIRE(test_helpers::mkdir(config_home, 0700) == 0);
		xdg_config.set(config_home.to_locale_string());

		const auto data_home = tmp.get_path().join("xdg-data"_path);
		REQUIRE(test_helpers::mkdir(data_home, 0700) == 0);
		xdg_data.set(data_home.to_locale_string());

		mock_newsbeuter_xdg_dirs(
			config_home.join("newsbeuter"_path),
			data_home.join("newsbeuter"_path),
			sentries);

		const auto data_dir = data_home.join("newsboat"_path);
		REQUIRE(test_helpers::mkdir(data_dir, 0700) == 0);
		verify_xdg_not_migrated(config_home.join("newsboat"_path), data_dir);
	}
}

TEST_CASE("try_migrate_from_newsbeuter() doesn't migrate files if Newsboat XDG "
	"config dir couldn't be created",
	"[ConfigPaths]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path().to_locale_string());
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
		const auto config_home = tmp.get_path().join(".config"_path);
		test_helpers::Chmod config_home_chmod(config_home, S_IRUSR | S_IXUSR);

		const auto config_dir = config_home.join("newsboat"_path);
		const auto data_dir = tmp.get_path().join(".local/share/newsboat"_path);
		verify_xdg_not_migrated(config_dir, data_dir);
	}

	SECTION("XDG_CONFIG_HOME redefined") {
		const auto config_home = tmp.get_path().join("xdg-conf"_path);
		REQUIRE(test_helpers::mkdir(config_home, 0700) == 0);
		xdg_config.set(config_home.to_locale_string());
		mock_newsbeuter_xdg_dirs(
			config_home.join("newsbeuter"_path),
			tmp.get_path().join(".local/share/newsbeuter"_path),
			sentries);

		// Making XDG .config unwriteable makes it impossible to create
		// a directory there
		test_helpers::Chmod config_home_chmod(config_home, S_IRUSR | S_IXUSR);

		verify_xdg_not_migrated(
			config_home.join("newsboat"_path),
			tmp.get_path().join(".local/share/newsboat"_path));
	}

	SECTION("XDG_DATA_HOME redefined") {
		const auto data_dir = tmp.get_path().join("xdg-data"_path);
		REQUIRE(test_helpers::mkdir(data_dir, 0700) == 0);
		xdg_data.set(data_dir.to_locale_string());
		mock_newsbeuter_xdg_dirs(
			tmp.get_path().join(".config/newsbeuter"_path),
			data_dir.join("newsbeuter"_path),
			sentries);

		// Making XDG .config unwriteable makes it impossible to create
		// a directory there
		const auto config_home = tmp.get_path().join(".config"_path);
		test_helpers::Chmod config_home_chmod(config_home, S_IRUSR | S_IXUSR);

		verify_xdg_not_migrated(config_home.join("newsboat"_path), data_dir.join("newsboat"_path));
	}

	SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
		const auto config_home = tmp.get_path().join("xdg-conf"_path);
		REQUIRE(test_helpers::mkdir(config_home, 0700) == 0);
		xdg_config.set(config_home.to_locale_string());

		const auto data_dir = tmp.get_path().join("xdg-data"_path);
		REQUIRE(test_helpers::mkdir(data_dir, 0700) == 0);
		xdg_data.set(data_dir.to_locale_string());

		mock_newsbeuter_xdg_dirs(
			config_home.join("newsbeuter"_path),
			data_dir.join("newsbeuter"_path),
			sentries);

		// Making XDG .config unwriteable makes it impossible to create
		// a directory there
		test_helpers::Chmod config_home_chmod(config_home, S_IRUSR | S_IXUSR);

		verify_xdg_not_migrated(config_home.join("newsboat"_path), data_dir.join("newsboat"_path));
	}
}

TEST_CASE("try_migrate_from_newsbeuter() doesn't migrate files if Newsboat XDG "
	"data dir couldn't be created",
	"[ConfigPaths]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path().to_locale_string());
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
		const auto data_home = tmp.get_path().join(".local/share"_path);
		test_helpers::Chmod data_home_chmod(data_home, S_IRUSR | S_IXUSR);

		const auto config_dir = tmp.get_path().join(".config/newsboat"_path);
		const auto data_dir = data_home.join("newsboat"_path);
		verify_xdg_not_migrated(config_dir, data_dir);
	}

	SECTION("XDG_CONFIG_HOME redefined") {
		const auto config_home = tmp.get_path().join("xdg-conf"_path);
		REQUIRE(test_helpers::mkdir(config_home, 0700) == 0);
		xdg_config.set(config_home.to_locale_string());
		mock_newsbeuter_xdg_dirs(
			config_home.join("newsbeuter"_path),
			tmp.get_path().join(".local/share/newsbeuter"_path),
			sentries);

		// Making XDG .local/share unwriteable makes it impossible to create
		// a directory there
		const auto data_home = tmp.get_path().join(".local/share"_path);
		test_helpers::Chmod data_home_chmod(data_home, S_IRUSR | S_IXUSR);

		verify_xdg_not_migrated(config_home.join("newsboat"_path),
			data_home.join("newsboat"_path));
	}

	SECTION("XDG_DATA_HOME redefined") {
		const auto data_home = tmp.get_path().join("xdg-data"_path);
		REQUIRE(test_helpers::mkdir(data_home, 0700) == 0);
		xdg_data.set(data_home.to_locale_string());
		mock_newsbeuter_xdg_dirs(
			tmp.get_path().join(".config/newsbeuter"_path),
			data_home.join("newsbeuter"_path),
			sentries);

		// Making XDG .local/share unwriteable makes it impossible to create
		// a directory there
		test_helpers::Chmod data_home_chmod(data_home, S_IRUSR | S_IXUSR);

		verify_xdg_not_migrated(
			tmp.get_path().join(".config/newsboat"_path),
			data_home.join("newsboat"_path));
	}

	SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
		const auto config_home = tmp.get_path().join("xdg-conf"_path);
		REQUIRE(test_helpers::mkdir(config_home, 0700) == 0);
		xdg_config.set(config_home.to_locale_string());

		const auto data_home = tmp.get_path().join("xdg-data"_path);
		REQUIRE(test_helpers::mkdir(data_home, 0700) == 0);
		xdg_data.set(data_home.to_locale_string());

		mock_newsbeuter_xdg_dirs(
			config_home.join("newsbeuter"_path),
			data_home.join("newsbeuter"_path),
			sentries);

		// Making XDG .local/share unwriteable makes it impossible to create
		// a directory there
		test_helpers::Chmod data_home_chmod(data_home, S_IRUSR | S_IXUSR);

		verify_xdg_not_migrated(config_home.join("newsboat"_path),
			data_home.join("newsboat"_path));
	}
}

void verify_dotdir_not_migrated(const Filepath& dotdir)
{
	ConfigPaths paths;
	REQUIRE(paths.initialized());

	// Shouldn't migrate anything, so should return false.
	REQUIRE_FALSE(paths.try_migrate_from_newsbeuter());

	REQUIRE_FALSE(test_helpers::file_available_for_reading(dotdir.join("config"_path)));
	REQUIRE_FALSE(test_helpers::file_available_for_reading(dotdir.join("urls"_path)));

	REQUIRE_FALSE(test_helpers::file_available_for_reading(dotdir.join("cache.db"_path)));
	REQUIRE_FALSE(test_helpers::file_available_for_reading(dotdir.join("queue"_path)));
	REQUIRE_FALSE(test_helpers::file_available_for_reading(
			dotdir.join("history.search"_path)));
	REQUIRE_FALSE(test_helpers::file_available_for_reading(
			dotdir.join("history.cmdline"_path)));
}

TEST_CASE("try_migrate_from_newsbeuter() doesn't migrate files if empty "
	"Newsboat dotdir already exists",
	"[ConfigPaths]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path().to_locale_string());
	INFO("Temporary directory (used as HOME): " << tmp.get_path());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	FileSentries sentries;

	mock_newsbeuter_dotdir(tmp, sentries);

	const auto dotdir = tmp.get_path().join(".newsboat"_path);
	REQUIRE(test_helpers::mkdir(dotdir, 0700) == 0);

	verify_dotdir_not_migrated(dotdir);
}

TEST_CASE("try_migrate_from_newsbeuter() doesn't migrate files if Newsboat "
	"dotdir couldn't be created",
	"[ConfigPaths]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path().to_locale_string());
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

	verify_dotdir_not_migrated(tmp.get_path().join(".newsboat"_path));
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
	home.set(tmp.get_path().to_locale_string());
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
	home.set(tmp.get_path().to_locale_string());
	INFO("Temporary directory (used as HOME): " << tmp.get_path());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	test_helpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	test_helpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	SECTION("Default XDG locations") {
		const auto config_dir = tmp.get_path().join(".config/newsboat"_path);
		REQUIRE(utils::mkdir_parents(config_dir, 0700) == 0);

		verify_create_dirs_returns_false(tmp);
	}

	SECTION("XDG_CONFIG_HOME redefined") {
		const auto config_home = tmp.get_path().join("xdg-cfg"_path);
		xdg_config.set(config_home.to_locale_string());

		const auto config_dir = config_home.join("newsboat"_path);
		REQUIRE(utils::mkdir_parents(config_dir, 0700) == 0);

		verify_create_dirs_returns_false(tmp);
	}

	SECTION("XDG_DATA_HOME redefined") {
		const auto config_dir = tmp.get_path().join(".config/newsboat"_path);
		REQUIRE(utils::mkdir_parents(config_dir, 0700) == 0);

		const auto data_home = tmp.get_path().join("xdg-data"_path);
		xdg_data.set(data_home.to_locale_string());
		// It's important to set the variable, but *not* create the directory
		// - it's the pre-condition of the test that the data dir doesn't exist

		verify_create_dirs_returns_false(tmp);
	}

	SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
		const auto config_home = tmp.get_path().join("xdg-cfg"_path);
		xdg_config.set(config_home.to_locale_string());

		const auto config_dir = config_home.join("newsboat"_path);
		REQUIRE(utils::mkdir_parents(config_dir, 0700) == 0);

		const auto data_home = tmp.get_path().join("xdg-data"_path);
		xdg_data.set(data_home.to_locale_string());
		// It's important to set the variable, but *not* create the directory
		// - it's the pre-condition of the test that the data dir doesn't exist

		verify_create_dirs_returns_false(tmp);
	}
}

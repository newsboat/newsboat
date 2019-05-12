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

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	TestHelpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	TestHelpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

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

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	TestHelpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	TestHelpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

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
	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	TestHelpers::EnvVar home("HOME");
	home.unset();
	TestHelpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	TestHelpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

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

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	TestHelpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	TestHelpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

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

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	TestHelpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	TestHelpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

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

/// Returns the contents of the file at `filepath`, or an empty string on error.
std::string file_contents(const std::string& filepath)
{
	std::ifstream in(filepath);
	if (in.is_open()) {
		std::string result;
		in >> result;
		return result;
	}

	return "";
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
		const TestHelpers::TempDir& tmp,
		const FileSentries& sentries)
{
	const auto dotdir_path = tmp.getPath() + ".newsbeuter/";
	::mkdir(dotdir_path.c_str(), 0700);
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
	utils::mkdir_parents(config_dir_path, 0700);
	REQUIRE(create_file(config_dir_path + "config", sentries.config));
	REQUIRE(create_file(config_dir_path + "urls", sentries.urls));

	utils::mkdir_parents(data_dir_path, 0700);
	REQUIRE(create_file(data_dir_path + "cache.db", sentries.cache));
	REQUIRE(create_file(data_dir_path + "queue", sentries.queue));
	REQUIRE(create_file(data_dir_path + "history.search", sentries.search));
	REQUIRE(create_file(data_dir_path + "history.cmdline", sentries.cmdline));
}

void mock_newsbeuter_xdg_dirs(
		const TestHelpers::TempDir& tmp,
		const FileSentries& sentries)
{
	const auto config_dir_path = tmp.getPath() + ".config/newsbeuter/";
	const auto data_dir_path = tmp.getPath() + ".local/share/newsbeuter/";
	mock_newsbeuter_xdg_dirs(config_dir_path, data_dir_path, sentries);
}

void mock_newsboat_dotdir(
		const TestHelpers::TempDir& tmp,
		const FileSentries& sentries)
{
	const auto dotdir_path = tmp.getPath() + ".newsboat/";
	::mkdir(dotdir_path.c_str(), 0700);

	const auto urls_file = dotdir_path + "urls";
	REQUIRE(create_file(urls_file, sentries.urls));
}

void mock_newsboat_xdg_dirs(
		const std::string& config_dir_path,
		const std::string& /*data_dir_path*/,
		const FileSentries& sentries)
{
	utils::mkdir_parents(config_dir_path, 0700);

	const auto urls_file = config_dir_path + "urls";
	REQUIRE(create_file(urls_file, sentries.urls));
}

void mock_newsboat_xdg_dirs(
		const TestHelpers::TempDir& tmp,
		const FileSentries& sentries)
{
	const auto config_dir_path = tmp.getPath() + ".config/newsboat/";
	const auto data_dir_path = tmp.getPath() + ".local/share/newsboat/";
	mock_newsboat_xdg_dirs(config_dir_path, data_dir_path, sentries);
}

TEST_CASE("try_migrate_from_newsbeuter() doesn't migrate if config paths "
		"were specified on the command line",
		"[ConfigPaths]")
{
	TestHelpers::TempDir tmp;

	TestHelpers::EnvVar home("HOME");
	home.set(tmp.getPath());
	INFO("Temporary directory (used as HOME): " << tmp.getPath());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	TestHelpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	TestHelpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	FileSentries beuterSentries;

	SECTION("Newsbeuter dotdir exists") {
		mock_newsbeuter_dotdir(tmp, beuterSentries);
	}

	SECTION("Newsbeuter XDG dirs exist") {
		mock_newsbeuter_xdg_dirs(tmp, beuterSentries);
	}

	FileSentries boatSentries;

	const auto url_file = tmp.getPath() + "my urls file";
	REQUIRE(create_file(url_file, boatSentries.urls));

	const auto cache_file = tmp.getPath() + "new cache.db";
	REQUIRE(create_file(cache_file, boatSentries.cache));

	const auto config_file = tmp.getPath() + "custom config file";
	REQUIRE(create_file(config_file, boatSentries.config));

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

	// No migration should occur, so should return false.
	REQUIRE_FALSE(paths.try_migrate_from_newsbeuter());

	INFO("Newsbeuter's urls file sentry: " << beuterSentries.urls);
	REQUIRE(file_contents(url_file) == boatSentries.urls);

	INFO("Newsbeuter's config file sentry: " << beuterSentries.config);
	REQUIRE(file_contents(config_file) == boatSentries.config);

	INFO("Newsbeuter's cache file sentry: " << beuterSentries.cache);
	REQUIRE(file_contents(cache_file) == boatSentries.cache);
}

TEST_CASE("try_migrate_from_newsbeuter() doesn't migrate if urls file "
		"already exists",
		"[ConfigPaths]")
{
	TestHelpers::TempDir tmp;

	TestHelpers::EnvVar home("HOME");
	home.set(tmp.getPath());
	INFO("Temporary directory (used as HOME): " << tmp.getPath());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	TestHelpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	TestHelpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	FileSentries beuterSentries;
	FileSentries boatSentries;

	const auto check = [&](const std::string& urls_filepath) {
		ConfigPaths paths;
		REQUIRE(paths.initialized());

		// No migration should occur, so should return false.
		REQUIRE_FALSE(paths.try_migrate_from_newsbeuter());

		INFO("Newsbeuter's urls file sentry: " << beuterSentries.urls);
		REQUIRE(file_contents(urls_filepath) == boatSentries.urls);
	};

	SECTION("Newsbeuter dotdir exists") {
		mock_newsbeuter_dotdir(tmp, beuterSentries);

		SECTION("Newsboat uses dotdir") {
			mock_newsboat_dotdir(tmp, boatSentries);
			check(tmp.getPath() + ".newsboat/urls");
		}

		SECTION("Newsboat uses XDG") {
			SECTION("Default XDG locations") {
				mock_newsboat_xdg_dirs(tmp, boatSentries);
				check(tmp.getPath() + ".config/newsboat/urls");
			}

			SECTION("XDG_CONFIG_HOME redefined") {
				const auto config_dir = tmp.getPath() + "xdg-conf/";
				::mkdir(config_dir.c_str(), 0700);
				xdg_config.set(config_dir);
				const auto newsboat_config_dir = config_dir + "newsboat/";
				mock_newsboat_xdg_dirs(
						newsboat_config_dir,
						tmp.getPath() + ".local/share/newsboat/",
						boatSentries);
				check(newsboat_config_dir + "urls");
			}

			SECTION("XDG_DATA_HOME redefined") {
				const auto data_dir = tmp.getPath() + "xdg-data/";
				::mkdir(data_dir.c_str(), 0700);
				xdg_data.set(data_dir);
				const auto newsboat_config_dir =
						tmp.getPath() + ".config/newsboat/";
				const auto newsboat_data_dir = data_dir + "newsboat/";
				mock_newsboat_xdg_dirs(
						newsboat_config_dir,
						newsboat_data_dir,
						boatSentries);
				check(newsboat_config_dir + "urls");
			}

			SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
				const auto config_dir = tmp.getPath() + "xdg-conf/";
				::mkdir(config_dir.c_str(), 0700);
				xdg_config.set(config_dir);

				const auto data_dir = tmp.getPath() + "xdg-data/";
				::mkdir(data_dir.c_str(), 0700);
				xdg_data.set(data_dir);

				const auto newsboat_config_dir = config_dir + "newsboat/";
				const auto newsboat_data_dir = data_dir + "newsboat/";

				mock_newsboat_xdg_dirs(
						newsboat_config_dir,
						newsboat_data_dir,
						boatSentries);

				check(newsboat_config_dir + "urls");
			}
		}
	}

	SECTION("Newsbeuter XDG dirs exist") {
		SECTION("Default XDG locations") {
			mock_newsbeuter_xdg_dirs(tmp, beuterSentries);

			SECTION("Newsboat uses dotdir") {
				mock_newsboat_dotdir(tmp, boatSentries);
				check(tmp.getPath() + ".newsboat/urls");
			}

			SECTION("Newsboat uses XDG") {
				mock_newsboat_xdg_dirs(tmp, boatSentries);
				check(tmp.getPath() + ".config/newsboat/urls");
			}
		}

		SECTION("XDG_CONFIG_HOME redefined") {
			const auto config_dir = tmp.getPath() + "xdg-conf/";
			::mkdir(config_dir.c_str(), 0700);
			xdg_config.set(config_dir);
			mock_newsbeuter_xdg_dirs(
					config_dir + "newsbeuter/",
					tmp.getPath() + ".local/share/newsbeuter/",
					beuterSentries);

			SECTION("Newsboat uses dotdir") {
				mock_newsboat_dotdir(tmp, boatSentries);
				check(tmp.getPath() + ".newsboat/urls");
			}

			SECTION("Newsboat uses XDG") {
				const auto newsboat_config_dir = config_dir + "newsboat/";
				mock_newsboat_xdg_dirs(
						newsboat_config_dir,
						tmp.getPath() + ".local/share/newsboat/",
						boatSentries);
				check(newsboat_config_dir + "urls");
			}
		}

		SECTION("XDG_DATA_HOME redefined") {
			const auto data_dir = tmp.getPath() + "xdg-data/";
			::mkdir(data_dir.c_str(), 0700);
			xdg_data.set(data_dir);
			mock_newsbeuter_xdg_dirs(
					tmp.getPath() + ".config/newsbeuter/",
					data_dir,
					beuterSentries);

			SECTION("Newsboat uses dotdir") {
				mock_newsboat_dotdir(tmp, boatSentries);
				check(tmp.getPath() + ".newsboat/urls");
			}

			SECTION("Newsboat uses XDG") {
				const auto newsboat_config_dir =
					tmp.getPath() + ".config/newsboat/";
				mock_newsboat_xdg_dirs(
						newsboat_config_dir,
						data_dir + "newsboat/",
						boatSentries);
				check(newsboat_config_dir + "urls");
			}
		}

		SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
			const auto config_dir = tmp.getPath() + "xdg-conf/";
			::mkdir(config_dir.c_str(), 0700);
			xdg_config.set(config_dir);

			const auto data_dir = tmp.getPath() + "xdg-data/";
			::mkdir(data_dir.c_str(), 0700);
			xdg_data.set(data_dir);

			mock_newsbeuter_xdg_dirs(
					config_dir + "newsbeuter/",
					data_dir + "newsbeuter/",
					beuterSentries);

			SECTION("Newsboat uses dotdir") {
				mock_newsboat_dotdir(tmp, boatSentries);
				check(tmp.getPath() + ".newsboat/urls");
			}

			SECTION("Newsboat uses XDG") {
				const auto newsboat_config_dir = config_dir + "newsboat/";
				mock_newsboat_xdg_dirs(
						newsboat_config_dir,
						data_dir + "newsboat/",
						boatSentries);
				check(newsboat_config_dir + "urls");
			}
		}
	}
}

TEST_CASE("try_migrate_from_newsbeuter() migrates Newsbeuter dotdir from "
		"default location to default location of Newsboat dotdir",
		"[ConfigPaths]")
{
	TestHelpers::TempDir tmp;

	TestHelpers::EnvVar home("HOME");
	home.set(tmp.getPath());
	INFO("Temporary directory (used as HOME): " << tmp.getPath());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	TestHelpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	TestHelpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	FileSentries sentries;

	mock_newsbeuter_dotdir(tmp, sentries);

	ConfigPaths paths;
	REQUIRE(paths.initialized());

	// Files should be migrated, so should return true.
	REQUIRE(paths.try_migrate_from_newsbeuter());

	const auto dotdir = tmp.getPath() + ".newsboat/";

	REQUIRE(file_contents(dotdir + "config") == sentries.config);
	REQUIRE(file_contents(dotdir + "urls") == sentries.urls);
	REQUIRE(file_contents(dotdir + "cache.db") == sentries.cache);
	REQUIRE(file_contents(dotdir + "queue") == sentries.queue);
	REQUIRE(file_contents(dotdir + "history.search") == sentries.search);
	REQUIRE(file_contents(dotdir + "history.cmdline") == sentries.cmdline);
}

TEST_CASE("try_migrate_from_newsbeuter() migrates Newsbeuter XDG dirs from "
		"their default location to default locations of Newsboat XDG dirs",
		"[ConfigPaths]")
{
	TestHelpers::TempDir tmp;

	TestHelpers::EnvVar home("HOME");
	home.set(tmp.getPath());
	INFO("Temporary directory (used as HOME): " << tmp.getPath());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	TestHelpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	TestHelpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	FileSentries sentries;

	const auto check =
	[&]
	(const std::string& config_dir, const std::string& data_dir) {
		ConfigPaths paths;
		REQUIRE(paths.initialized());

		// Files should be migrated, so should return true.
		REQUIRE(paths.try_migrate_from_newsbeuter());

		REQUIRE(file_contents(config_dir + "config") == sentries.config);
		REQUIRE(file_contents(config_dir + "urls") == sentries.urls);

		REQUIRE(file_contents(data_dir + "cache.db") == sentries.cache);
		REQUIRE(file_contents(data_dir + "queue") == sentries.queue);
		REQUIRE(file_contents(data_dir + "history.search") == sentries.search);
		REQUIRE(file_contents(data_dir + "history.cmdline") == sentries.cmdline);
	};

	SECTION("Default XDG locations") {
		mock_newsbeuter_xdg_dirs(tmp, sentries);
		const auto config_dir = tmp.getPath() + ".config/newsboat/";
		const auto data_dir = tmp.getPath() + ".local/share/newsboat/";
		check(config_dir, data_dir);
	}

	SECTION("XDG_CONFIG_HOME redefined") {
		const auto config_dir = tmp.getPath() + "xdg-conf/";
		::mkdir(config_dir.c_str(), 0700);
		xdg_config.set(config_dir);
		mock_newsbeuter_xdg_dirs(
				config_dir + "newsbeuter/",
				tmp.getPath() + ".local/share/newsbeuter/",
				sentries);

		check(config_dir + "newsboat/",
				tmp.getPath() + ".local/share/newsboat/");
	}

	SECTION("XDG_DATA_HOME redefined") {
		const auto data_dir = tmp.getPath() + "xdg-data/";
		::mkdir(data_dir.c_str(), 0700);
		xdg_data.set(data_dir);
		mock_newsbeuter_xdg_dirs(
				tmp.getPath() + ".config/newsbeuter/",
				data_dir + "newsbeuter/",
				sentries);

		check(tmp.getPath() + ".config/newsboat/",
				data_dir + "newsboat/");
	}

	SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
		const auto config_dir = tmp.getPath() + "xdg-conf/";
		::mkdir(config_dir.c_str(), 0700);
		xdg_config.set(config_dir);

		const auto data_dir = tmp.getPath() + "xdg-data/";
		::mkdir(data_dir.c_str(), 0700);
		xdg_data.set(data_dir);

		mock_newsbeuter_xdg_dirs(
				config_dir + "newsbeuter/",
				data_dir + "newsbeuter/",
				sentries);

		check(config_dir + "newsboat/", data_dir + "newsboat/");
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
	TestHelpers::TempDir tmp;

	TestHelpers::EnvVar home("HOME");
	home.set(tmp.getPath());
	INFO("Temporary directory (used as HOME): " << tmp.getPath());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	TestHelpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	TestHelpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	FileSentries sentries;

	SECTION("Default XDG locations") {
		mock_newsbeuter_xdg_dirs(tmp, sentries);

		const auto config_dir = tmp.getPath() + ".config/newsboat/";
		::mkdir(config_dir.c_str(), 0700);

		const auto data_dir = tmp.getPath() + ".local/share/newsboat/";
		verify_xdg_not_migrated(config_dir, data_dir);
	}

	SECTION("XDG_CONFIG_HOME redefined") {
		const auto config_home = tmp.getPath() + "xdg-conf/";
		::mkdir(config_home.c_str(), 0700);
		xdg_config.set(config_home);
		mock_newsbeuter_xdg_dirs(
				config_home + "newsbeuter/",
				tmp.getPath() + ".local/share/newsbeuter/",
				sentries);

		const auto config_dir = config_home + "newsboat/";
		::mkdir(config_dir.c_str(), 0700);
		verify_xdg_not_migrated(config_dir, tmp.getPath() + ".local/share/newsboat/");
	}

	SECTION("XDG_DATA_HOME redefined") {
		const auto data_dir = tmp.getPath() + "xdg-data/";
		::mkdir(data_dir.c_str(), 0700);
		xdg_data.set(data_dir);
		mock_newsbeuter_xdg_dirs(
				tmp.getPath() + ".config/newsbeuter/",
				data_dir + "newsbeuter/",
				sentries);

		const auto config_dir = tmp.getPath() + ".config/newsboat/";
		::mkdir(config_dir.c_str(), 0700);
		verify_xdg_not_migrated(config_dir, data_dir + "newsboat/");
	}

	SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
		const auto config_home = tmp.getPath() + "xdg-conf/";
		::mkdir(config_home.c_str(), 0700);
		xdg_config.set(config_home);

		const auto data_dir = tmp.getPath() + "xdg-data/";
		::mkdir(data_dir.c_str(), 0700);
		xdg_data.set(data_dir);

		mock_newsbeuter_xdg_dirs(
				config_home + "newsbeuter/",
				data_dir + "newsbeuter/",
				sentries);

		const auto config_dir = config_home + "newsboat/";
		::mkdir(config_dir.c_str(), 0700);
		verify_xdg_not_migrated(config_dir, data_dir + "newsboat/");
	}
}

TEST_CASE("try_migrate_from_newsbeuter() doesn't migrate files if empty "
		"Newsboat XDG data dir already exists",
		"[ConfigPaths]")
{
	TestHelpers::TempDir tmp;

	TestHelpers::EnvVar home("HOME");
	home.set(tmp.getPath());
	INFO("Temporary directory (used as HOME): " << tmp.getPath());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	TestHelpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	TestHelpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	FileSentries sentries;

	SECTION("Default XDG locations") {
		mock_newsbeuter_xdg_dirs(tmp, sentries);

		const auto config_dir = tmp.getPath() + ".config/newsboat/";
		const auto data_dir = tmp.getPath() + ".local/share/newsboat/";
		::mkdir(data_dir.c_str(), 0700);
		verify_xdg_not_migrated(config_dir, data_dir);
	}

	SECTION("XDG_CONFIG_HOME redefined") {
		const auto config_home = tmp.getPath() + "xdg-conf/";
		::mkdir(config_home.c_str(), 0700);
		xdg_config.set(config_home);
		mock_newsbeuter_xdg_dirs(
				config_home + "newsbeuter/",
				tmp.getPath() + ".local/share/newsbeuter/",
				sentries);

		const auto data_dir = tmp.getPath() + ".local/share/newsboat/";
		::mkdir(data_dir.c_str(), 0700);
		verify_xdg_not_migrated(config_home + "newsboat/", data_dir);
	}

	SECTION("XDG_DATA_HOME redefined") {
		const auto data_home = tmp.getPath() + "xdg-data/";
		::mkdir(data_home.c_str(), 0700);
		xdg_data.set(data_home);
		mock_newsbeuter_xdg_dirs(
				tmp.getPath() + ".config/newsbeuter/",
				data_home + "newsbeuter/",
				sentries);

		const auto data_dir = data_home + "newsboat/";
		::mkdir(data_dir.c_str(), 0700);
		verify_xdg_not_migrated(tmp.getPath() + ".config/newsboat/", data_dir);
	}

	SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
		const auto config_home = tmp.getPath() + "xdg-conf/";
		::mkdir(config_home.c_str(), 0700);
		xdg_config.set(config_home);

		const auto data_home = tmp.getPath() + "xdg-data/";
		::mkdir(data_home.c_str(), 0700);
		xdg_data.set(data_home);

		mock_newsbeuter_xdg_dirs(
				config_home + "newsbeuter/",
				data_home + "newsbeuter/",
				sentries);

		const auto data_dir = data_home + "newsboat/";
		::mkdir(data_dir.c_str(), 0700);
		verify_xdg_not_migrated(config_home + "newsboat/", data_dir);
	}
}

TEST_CASE("try_migrate_from_newsbeuter() doesn't migrate files if Newsboat XDG "
		"config dir couldn't be created",
		"[ConfigPaths]")
{
	TestHelpers::TempDir tmp;

	TestHelpers::EnvVar home("HOME");
	home.set(tmp.getPath());
	INFO("Temporary directory (used as HOME): " << tmp.getPath());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	TestHelpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	TestHelpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	FileSentries sentries;

	SECTION("Default XDG locations") {
		mock_newsbeuter_xdg_dirs(tmp, sentries);

		// Making XDG .config unwriteable makes it impossible to create
		// a directory there
		const auto config_home = tmp.getPath() + ".config/";
		TestHelpers::Chmod config_home_chmod(config_home, S_IRUSR | S_IXUSR);

		const auto config_dir = config_home + "newsboat/";
		const auto data_dir = tmp.getPath() + ".local/share/newsboat/";
		verify_xdg_not_migrated(config_dir, data_dir);
	}

	SECTION("XDG_CONFIG_HOME redefined") {
		const auto config_home = tmp.getPath() + "xdg-conf/";
		::mkdir(config_home.c_str(), 0700);
		xdg_config.set(config_home);
		mock_newsbeuter_xdg_dirs(
				config_home + "newsbeuter/",
				tmp.getPath() + ".local/share/newsbeuter/",
				sentries);

		// Making XDG .config unwriteable makes it impossible to create
		// a directory there
		TestHelpers::Chmod config_home_chmod(config_home, S_IRUSR | S_IXUSR);

		verify_xdg_not_migrated(
				config_home + "newsboat/",
				tmp.getPath() + ".local/share/newsboat/");
	}

	SECTION("XDG_DATA_HOME redefined") {
		const auto data_dir = tmp.getPath() + "xdg-data/";
		::mkdir(data_dir.c_str(), 0700);
		xdg_data.set(data_dir);
		mock_newsbeuter_xdg_dirs(
				tmp.getPath() + ".config/newsbeuter/",
				data_dir + "newsbeuter/",
				sentries);

		// Making XDG .config unwriteable makes it impossible to create
		// a directory there
		const auto config_home = tmp.getPath() + ".config/";
		TestHelpers::Chmod config_home_chmod(config_home, S_IRUSR | S_IXUSR);

		verify_xdg_not_migrated(
				config_home + "newsboat/",
				data_dir + "newsboat/");
	}

	SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
		const auto config_home = tmp.getPath() + "xdg-conf/";
		::mkdir(config_home.c_str(), 0700);
		xdg_config.set(config_home);

		const auto data_dir = tmp.getPath() + "xdg-data/";
		::mkdir(data_dir.c_str(), 0700);
		xdg_data.set(data_dir);

		mock_newsbeuter_xdg_dirs(
				config_home + "newsbeuter/",
				data_dir + "newsbeuter/",
				sentries);

		// Making XDG .config unwriteable makes it impossible to create
		// a directory there
		TestHelpers::Chmod config_home_chmod(config_home, S_IRUSR | S_IXUSR);

		verify_xdg_not_migrated(
				config_home + "newsboat/",
				data_dir + "newsboat/");
	}
}

TEST_CASE("try_migrate_from_newsbeuter() doesn't migrate files if Newsboat XDG "
		"data dir couldn't be created",
		"[ConfigPaths]")
{
	TestHelpers::TempDir tmp;

	TestHelpers::EnvVar home("HOME");
	home.set(tmp.getPath());
	INFO("Temporary directory (used as HOME): " << tmp.getPath());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	TestHelpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	TestHelpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	FileSentries sentries;

	SECTION("Default XDG locations") {
		mock_newsbeuter_xdg_dirs(tmp, sentries);

		// Making XDG .local/share unwriteable makes it impossible to create
		// a directory there
		const auto data_home = tmp.getPath() + ".local/share/";
		TestHelpers::Chmod data_home_chmod(data_home, S_IRUSR | S_IXUSR);

		const auto config_dir = tmp.getPath() + ".config/newsboat/";
		const auto data_dir = data_home + "newsboat/";
		verify_xdg_not_migrated(config_dir, data_dir);
	}

	SECTION("XDG_CONFIG_HOME redefined") {
		const auto config_home = tmp.getPath() + "xdg-conf/";
		::mkdir(config_home.c_str(), 0700);
		xdg_config.set(config_home);
		mock_newsbeuter_xdg_dirs(
				config_home + "newsbeuter/",
				tmp.getPath() + ".local/share/newsbeuter/",
				sentries);

		// Making XDG .local/share unwriteable makes it impossible to create
		// a directory there
		const auto data_home = tmp.getPath() + ".local/share/";
		TestHelpers::Chmod data_home_chmod(data_home, S_IRUSR | S_IXUSR);

		verify_xdg_not_migrated(
				config_home + "newsboat/",
				data_home + "newsboat/");
	}

	SECTION("XDG_DATA_HOME redefined") {
		const auto data_home = tmp.getPath() + "xdg-data/";
		::mkdir(data_home.c_str(), 0700);
		xdg_data.set(data_home);
		mock_newsbeuter_xdg_dirs(
				tmp.getPath() + ".config/newsbeuter/",
				data_home + "newsbeuter/",
				sentries);

		// Making XDG .local/share unwriteable makes it impossible to create
		// a directory there
		TestHelpers::Chmod data_home_chmod(data_home, S_IRUSR | S_IXUSR);

		verify_xdg_not_migrated(
				tmp.getPath() + ".config/newsboat/",
				data_home + "newsboat/");
	}

	SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
		const auto config_home = tmp.getPath() + "xdg-conf/";
		::mkdir(config_home.c_str(), 0700);
		xdg_config.set(config_home);

		const auto data_home = tmp.getPath() + "xdg-data/";
		::mkdir(data_home.c_str(), 0700);
		xdg_data.set(data_home);

		mock_newsbeuter_xdg_dirs(
				config_home + "newsbeuter/",
				data_home + "newsbeuter/",
				sentries);

		// Making XDG .local/share unwriteable makes it impossible to create
		// a directory there
		TestHelpers::Chmod data_home_chmod(data_home, S_IRUSR | S_IXUSR);

		verify_xdg_not_migrated(
				config_home + "newsboat/",
				data_home + "newsboat/");
	}
}

void verify_dotdir_not_migrated(const std::string& dotdir)
{
	ConfigPaths paths;
	REQUIRE(paths.initialized());

	// Shouldn't migrate anything, so should return false.
	REQUIRE_FALSE(paths.try_migrate_from_newsbeuter());

	const auto config = dotdir + "config";
	REQUIRE_FALSE(0 == ::access(config.c_str(), R_OK));
	const auto urls = dotdir + "urls";
	REQUIRE_FALSE(0 == ::access(urls.c_str(), R_OK));

	const auto cache = dotdir + "cache.db";
	REQUIRE_FALSE(0 == ::access(cache.c_str(), R_OK));
	const auto queue = dotdir + "queue";
	REQUIRE_FALSE(0 == ::access(queue.c_str(), R_OK));
	const auto search = dotdir + "history.search";
	REQUIRE_FALSE(0 == ::access(search.c_str(), R_OK));
	const auto cmdline = dotdir + "history.cmdline";
	REQUIRE_FALSE(0 == ::access(cmdline.c_str(), R_OK));
}

TEST_CASE("try_migrate_from_newsbeuter() doesn't migrate files if empty "
		"Newsboat dotdir already exists",
		"[ConfigPaths]")
{
	TestHelpers::TempDir tmp;

	TestHelpers::EnvVar home("HOME");
	home.set(tmp.getPath());
	INFO("Temporary directory (used as HOME): " << tmp.getPath());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	TestHelpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	TestHelpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	FileSentries sentries;

	mock_newsbeuter_dotdir(tmp, sentries);

	const auto dotdir = tmp.getPath() + ".newsboat/";
	::mkdir(dotdir.c_str(), 0700);

	verify_dotdir_not_migrated(dotdir);
}

TEST_CASE("try_migrate_from_newsbeuter() doesn't migrate files if Newsboat "
		"dotdir couldn't be created",
		"[ConfigPaths]")
{
	TestHelpers::TempDir tmp;

	TestHelpers::EnvVar home("HOME");
	home.set(tmp.getPath());
	INFO("Temporary directory (used as HOME): " << tmp.getPath());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	TestHelpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	TestHelpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	FileSentries sentries;

	mock_newsbeuter_dotdir(tmp, sentries);

	// Making home directory unwriteable makes it impossible to create
	// a directory there
	TestHelpers::Chmod dotdir_chmod(tmp.getPath(), S_IRUSR | S_IXUSR);

	verify_dotdir_not_migrated(tmp.getPath() + ".newsboat/");
}

void verify_create_dirs_returns_false(const TestHelpers::TempDir& tmp)
{
	const mode_t readonly = S_IRUSR | S_IXUSR;
	TestHelpers::Chmod home_chmod(tmp.getPath(), readonly);

	ConfigPaths paths;
	REQUIRE(paths.initialized());
	REQUIRE_FALSE(paths.create_dirs());
}

TEST_CASE("create_dirs() returns false if dotdir doesn't exist and couldn't "
		"be created",
		"[ConfigPaths]")
{
	TestHelpers::TempDir tmp;

	TestHelpers::EnvVar home("HOME");
	home.set(tmp.getPath());
	INFO("Temporary directory (used as HOME): " << tmp.getPath());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	TestHelpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	TestHelpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	verify_create_dirs_returns_false(tmp);
}

TEST_CASE("create_dirs() returns false if XDG config dir exists but data dir "
		"doesn't exist and couldn't be created",
		"[ConfigPaths]")
{
	TestHelpers::TempDir tmp;

	TestHelpers::EnvVar home("HOME");
	home.set(tmp.getPath());
	INFO("Temporary directory (used as HOME): " << tmp.getPath());

	// ConfigPaths rely on these variables, so let's sanitize them to ensure
	// that the tests aren't affected
	TestHelpers::EnvVar xdg_config("XDG_CONFIG_HOME");
	xdg_config.unset();
	TestHelpers::EnvVar xdg_data("XDG_DATA_HOME");
	xdg_data.unset();

	SECTION("Default XDG locations") {
		const auto config_dir = tmp.getPath() + ".config/newsboat/";
		::mkdir(config_dir.c_str(), 0700);

		verify_create_dirs_returns_false(tmp);
	}

	SECTION("XDG_CONFIG_HOME redefined") {
		const auto config_home = tmp.getPath() + "xdg-cfg/";
		xdg_config.set(config_home);

		const auto config_dir = config_home + "newsboat/";
		::mkdir(config_dir.c_str(), 0700);

		verify_create_dirs_returns_false(tmp);
	}

	SECTION("XDG_DATA_HOME redefined") {
		const auto data_home = tmp.getPath() + "xdg-data/";
		xdg_data.set(data_home);

		const auto data_dir = data_home + "newsboat/";
		::mkdir(data_dir.c_str(), 0700);

		verify_create_dirs_returns_false(tmp);
	}

	SECTION("Both XDG_CONFIG_HOME and XDG_DATA_HOME redefined") {
		const auto config_home = tmp.getPath() + "xdg-cfg/";
		xdg_config.set(config_home);

		const auto config_dir = config_home + "newsboat/";
		::mkdir(config_dir.c_str(), 0700);

		const auto data_home = tmp.getPath() + "xdg-data/";
		xdg_data.set(data_home);

		const auto data_dir = data_home + "newsboat/";
		::mkdir(data_dir.c_str(), 0700);

		verify_create_dirs_returns_false(tmp);
	}
}

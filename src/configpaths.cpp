#include "configpaths.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <pwd.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "globals.h"
#include "strprintf.h"
#include "utils.h"

namespace newsboat {

ConfigPaths::ConfigPaths()
	: m_url_file("urls")
	, m_cache_file("cache.db")
	, m_config_file("config")
	, m_queue_file("queue")
{
	const char* env_home = nullptr;
	if (!(env_home = ::getenv("HOME"))) {
		const struct passwd* spw = ::getpwuid(::getuid());
		if (spw) {
			env_home = spw->pw_dir;
		} else {
			m_error_message = strprintf::fmt(
				_("Fatal error: couldn't determine home "
				  "directory!\nPlease set the HOME environment "
				  "variable or add a valid user for UID %u!"),
				::getuid());
		}
	}

	if (env_home) {
		m_env_home = env_home;
	}

	find_dirs();
}

bool ConfigPaths::initialized() const
{
	return !m_env_home.empty();
}

std::string ConfigPaths::error_message() const
{
	return m_error_message;
}

void ConfigPaths::find_dirs()
{
	m_config_dir = m_env_home;
	m_config_dir.append(NEWSBEUTER_PATH_SEP);
	m_config_dir.append(NEWSBOAT_CONFIG_SUBDIR);

	m_data_dir = m_config_dir;

	/* Will change config_dir and data_dir to point to XDG if XDG
	 * directories are available. */
	find_dirs_xdg();

	/* in config */
	const std::string cfg = m_config_dir + std::string(NEWSBEUTER_PATH_SEP);
	m_url_file = cfg + m_url_file;
	m_config_file = cfg + m_config_file;

	/* in data */
	const std::string data = m_data_dir + std::string(NEWSBEUTER_PATH_SEP);
	m_cache_file = data + m_cache_file;
	m_lock_file = m_cache_file + LOCK_SUFFIX;
	m_queue_file = data + m_queue_file;
	m_search_file = data + std::string("history.search");
	m_cmdline_file = data + std::string("history.cmdline");
}

/**
 * \brief Try to setup XDG style dirs.
 *
 * returns false, if that fails
 */
bool ConfigPaths::find_dirs_xdg()
{
	std::string xdg_config_dir;
	const char* env_xdg_config = ::getenv("XDG_CONFIG_HOME");
	if (env_xdg_config) {
		xdg_config_dir = env_xdg_config;
	} else {
		xdg_config_dir = m_env_home;
		xdg_config_dir.append(NEWSBEUTER_PATH_SEP);
		xdg_config_dir.append(".config");
	}

	std::string xdg_data_dir;
	const char* env_xdg_data = ::getenv("XDG_DATA_HOME");
	if (env_xdg_data) {
		xdg_data_dir = env_xdg_data;
	} else {
		xdg_data_dir = m_env_home;
		xdg_data_dir.append(NEWSBEUTER_PATH_SEP);
		xdg_data_dir.append(".local");
		xdg_data_dir.append(NEWSBEUTER_PATH_SEP);
		xdg_data_dir.append("share");
	}

	xdg_config_dir.append(NEWSBEUTER_PATH_SEP);
	xdg_config_dir.append(NEWSBOAT_SUBDIR_XDG);

	xdg_data_dir.append(NEWSBEUTER_PATH_SEP);
	xdg_data_dir.append(NEWSBOAT_SUBDIR_XDG);

	bool config_dir_exists =
		0 == access(xdg_config_dir.c_str(), R_OK | X_OK);

	if (!config_dir_exists) {
		return false;
	}

	/* Invariant: config dir exists.
	 *
	 * At this point, we're confident we'll be using XDG. We don't check if
	 * data dir exists, because if it doesn't we'll create it. */

	m_config_dir = xdg_config_dir;
	m_data_dir = xdg_data_dir;
	return true;
}

void ConfigPaths::process_args(const CliArgsParser& args)
{
	if (args.set_url_file()) {
		m_url_file = args.url_file();
	}

	if (args.set_cache_file()) {
		m_cache_file = args.cache_file();
	}

	if (args.set_lock_file()) {
		m_lock_file = args.lock_file();
	}

	if (args.set_config_file()) {
		m_config_file = args.config_file();
	}

	m_silent = args.silent();
	m_using_nonstandard_configs = args.using_nonstandard_configs();
}

bool ConfigPaths::try_migrate_from_newsbeuter()
{
	if ((!m_using_nonstandard_configs) &&
		(0 != access(m_url_file.c_str(), F_OK))) {
		return migrate_data_from_newsbeuter();
	}

	// No migration occurred.
	return false;
}

void copy_file(const std::string& input_filepath,
	const std::string& output_filepath)
{
	std::cerr << input_filepath << "  ->  " << output_filepath << '\n';

	std::ifstream src(input_filepath, std::ios_base::binary);
	std::ofstream dst(output_filepath, std::ios_base::binary);
	dst << src.rdbuf();
}

bool ConfigPaths::migrate_data_from_newsbeuter_xdg()
{
	const char* env_xdg_config = ::getenv("XDG_CONFIG_HOME");
	std::string xdg_config_dir;
	if (env_xdg_config) {
		xdg_config_dir = env_xdg_config;
	} else {
		xdg_config_dir = m_env_home;
		xdg_config_dir.append(NEWSBEUTER_PATH_SEP);
		xdg_config_dir.append(".config");
	}

	const char* env_xdg_data = ::getenv("XDG_DATA_HOME");
	std::string xdg_data_dir;
	if (env_xdg_data) {
		xdg_data_dir = env_xdg_data;
	} else {
		xdg_data_dir = m_env_home;
		xdg_data_dir.append(NEWSBEUTER_PATH_SEP);
		xdg_data_dir.append(".local");
		xdg_data_dir.append(NEWSBEUTER_PATH_SEP);
		xdg_data_dir.append("share");
	}

	const auto newsbeuter_config_dir = xdg_config_dir +
		NEWSBEUTER_PATH_SEP + NEWSBEUTER_SUBDIR_XDG +
		NEWSBEUTER_PATH_SEP;
	const auto newsbeuter_data_dir = xdg_data_dir + NEWSBEUTER_PATH_SEP +
		NEWSBEUTER_SUBDIR_XDG + NEWSBEUTER_PATH_SEP;

	const auto newsboat_config_dir = xdg_config_dir + NEWSBEUTER_PATH_SEP +
		NEWSBOAT_SUBDIR_XDG + NEWSBEUTER_PATH_SEP;
	const auto newsboat_data_dir = xdg_data_dir + NEWSBEUTER_PATH_SEP +
		NEWSBOAT_SUBDIR_XDG + NEWSBEUTER_PATH_SEP;

	bool newsbeuter_config_dir_exists =
		0 == access(newsbeuter_config_dir.c_str(), R_OK | X_OK);

	if (!newsbeuter_config_dir_exists) {
		return false;
	}

	auto exists = [](const std::string& dir) -> bool {
		bool dir_exists = 0 == access(dir.c_str(), F_OK);
		if (dir_exists) {
			LOG(Level::DEBUG,
				"%s already exists, aborting XDG migration.",
				dir);
		}
		return dir_exists;
	};
	if (exists(newsboat_config_dir)) {
		return false;
	}
	if (exists(newsboat_data_dir)) {
		return false;
	}
	m_config_dir = newsboat_config_dir;
	m_data_dir = newsboat_data_dir;

	if (!m_silent) {
		std::cerr << "Migrating configs and data from Newsbeuter's XDG "
			     "dirs..."
			  << std::endl;
	}

	auto try_mkdir = [](const std::string& dir) -> bool {
		bool result = 0 == utils::mkdir_parents(dir, 0700);
		// If dir already exists, it's an error, so we won't check the
		// errno (unlike in many other places in the code)
		if (!result) {
			LOG(Level::DEBUG,
				"Aborting XDG migration because mkdir on %s "
				"failed: %s",
				dir,
				strerror(errno));
		}
		return result;
	};
	if (!try_mkdir(newsboat_config_dir)) {
		return false;
	}
	if (!try_mkdir(newsboat_data_dir)) {
		return false;
	}

	/* in config */
	copy_file(newsbeuter_config_dir + "urls", newsboat_config_dir + "urls");
	copy_file(newsbeuter_config_dir + "config",
		newsboat_config_dir + "config");

	/* in data */
	copy_file(newsbeuter_data_dir + "cache.db",
		newsboat_data_dir + "cache.db");
	copy_file(newsbeuter_data_dir + "queue", newsboat_data_dir + "queue");
	copy_file(newsbeuter_data_dir + "history.search",
		newsboat_data_dir + "history.search");
	copy_file(newsbeuter_data_dir + "history.cmdline",
		newsboat_data_dir + "history.cmdline");

	return true;
}

bool ConfigPaths::migrate_data_from_newsbeuter_dotdir()
{
	std::string newsbeuter_dir = m_env_home;
	newsbeuter_dir += NEWSBEUTER_PATH_SEP;
	newsbeuter_dir += NEWSBEUTER_CONFIG_SUBDIR;
	newsbeuter_dir += NEWSBEUTER_PATH_SEP;

	bool newsbeuter_dir_exists =
		0 == access(newsbeuter_dir.c_str(), R_OK | X_OK);
	if (!newsbeuter_dir_exists) {
		return false;
	}

	std::string newsboat_dir = m_env_home;
	newsboat_dir += NEWSBEUTER_PATH_SEP;
	newsboat_dir += NEWSBOAT_CONFIG_SUBDIR;
	newsboat_dir += NEWSBEUTER_PATH_SEP;

	bool newsboat_dir_exists = 0 == access(newsboat_dir.c_str(), F_OK);
	if (newsboat_dir_exists) {
		LOG(Level::DEBUG,
			"%s already exists, aborting migration.",
			newsboat_dir);
		return false;
	}

	if (!m_silent) {
		std::cerr << "Migrating configs and data from Newsbeuter's "
			     "dotdir..."
			  << std::endl;
	}

	if (0 != ::mkdir(newsboat_dir.c_str(), 0700) && errno != EEXIST) {
		if (!m_silent) {
			std::cerr << "Aborting migration because mkdir on "
				  << newsboat_dir
				  << " failed: " << strerror(errno)
				  << std::endl;
		}
		return false;
	}

	copy_file(newsbeuter_dir + "urls", newsboat_dir + "urls");
	copy_file(newsbeuter_dir + "cache.db", newsboat_dir + "cache.db");
	copy_file(newsbeuter_dir + "config", newsboat_dir + "config");
	copy_file(newsbeuter_dir + "queue", newsboat_dir + "queue");
	copy_file(newsbeuter_dir + "history.search",
		newsboat_dir + "history.search");
	copy_file(newsbeuter_dir + "history.cmdline",
		newsboat_dir + "history.cmdline");

	return true;
}

bool ConfigPaths::migrate_data_from_newsbeuter()
{
	bool migrated = migrate_data_from_newsbeuter_xdg();

	if (migrated) {
		// Re-running to pick up XDG dirs
		m_url_file = "urls";
		m_cache_file = "cache.db";
		m_config_file = "config";
		m_queue_file = "queue";
		find_dirs();
	} else {
		migrated = migrate_data_from_newsbeuter_dotdir();
	}

	return migrated;
}

bool ConfigPaths::create_dirs() const
{
	auto try_mkdir = [](const std::string& dir) -> bool {
		const bool result = 0 == utils::mkdir_parents(dir, 0700);
		if (!result && errno != EEXIST) {
			LOG(Level::CRITICAL,
				"Couldn't create `%s': (%i) %s",
				dir,
				errno,
				strerror(errno));
			return false;
		} else {
			return true;
		}
	};

	return try_mkdir(m_config_dir) && try_mkdir(m_data_dir);
}

void ConfigPaths::set_cache_file(const std::string& new_cachefile)
{
	m_cache_file = new_cachefile;
	m_lock_file = m_cache_file + LOCK_SUFFIX;
}

} // namespace newsboat

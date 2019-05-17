#ifndef NEWSBOAT_CONFIGPATHS_H_
#define NEWSBOAT_CONFIGPATHS_H_

#include <string>

#include "cliargsparser.h"

namespace newsboat {
class ConfigPaths {
	std::string m_env_home;
	std::string m_error_message;

	std::string m_data_dir;
	std::string m_config_dir;

	std::string m_url_file;
	std::string m_cache_file;
	std::string m_config_file;
	std::string m_lock_file;
	std::string m_queue_file;
	std::string m_search_file;
	std::string m_cmdline_file;

	bool m_silent = false;
	bool m_using_nonstandard_configs = false;

	bool find_dirs_xdg();
	void find_dirs();

	/// Looks for Newsbeuter's XDG directories and, if found, copies their
	/// contents to Newsboat's XDG dirs. Returns true if copied something,
	/// false otherwise.
	bool migrate_data_from_newsbeuter_xdg();

	/// Looks for Newsbeuter's dot directory and, if found, copies its contents
	/// to Newsboat's dotdir. Returns true if copied something, false
	/// otherwise.
	bool migrate_data_from_newsbeuter_dotdir();

	/// Looks for Newsbeuter's directories and, if found, copies their contents
	/// to corresponding Newsboat's directory. Returns true if copied
	/// something, false otherwise.
	bool migrate_data_from_newsbeuter();

public:
	ConfigPaths();

	/// \brief Indicates if the object can be used.
	///
	/// If this method returned `false`, the cause for initialization
	/// failure can be found using `error_message()`.
	bool initialized() const;

	/// Returns explanation why initialization failed.
	///
	/// \note You shouldn't call this unless `initialized()` returns
	/// `false`.
	std::string error_message() const;

	/// Initializes paths to config, cache etc. from CLI arguments.
	void process_args(const CliArgsParser& args);

	/// If user didn't specify paths to configs on the command line, and the
	/// config file wasn't found in the standard directories, looks for
	/// Newsbeuter's directories, and copies their contents if found. Returns
	/// true if copied something, false otherwise.
	bool try_migrate_from_newsbeuter();

	/// Creates Newsboat's dotdir or XDG config & data dirs (depending on what
	/// was configured during initialization, when processing CLI args, and if
	/// migration found anything).
	bool create_dirs() const;

	/// Path to the URLs file.
	std::string url_file() const
	{
		return m_url_file;
	}

	/// Path to the cache file.
	std::string cache_file() const
	{
		return m_cache_file;
	}

	/// Sets path to the cache file.
	// FIXME: this is actually a kludge that lets Controller change the path
	// midway. That logic should be moved into ConfigPaths, and this method
	// removed.
	void set_cache_file(const std::string&);

	/// Path to the config file.
	std::string config_file() const
	{
		return m_config_file;
	}

	/// Path to the lock file.
	///
	/// \note This changes when path to config file changes.
	std::string lock_file() const
	{
		return m_lock_file;
	}

	/// \brief Path to the queue file.
	///
	/// Queue file stores enqueued podcasts. It's written by Newsboat, and
	/// read by Podboat.
	std::string queue_file() const
	{
		return m_queue_file;
	}

	/// Path to the file with previous search queries.
	std::string search_file() const
	{
		return m_search_file;
	}

	/// Path to the file with command-line history.
	std::string cmdline_file() const
	{
		return m_cmdline_file;
	}
};
} // namespace newsboat

#endif /* NEWSBOAT_CONFIGPATHS_H_ */

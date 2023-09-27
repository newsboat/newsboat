#ifndef NEWSBOAT_CONFIGPATHS_H_
#define NEWSBOAT_CONFIGPATHS_H_

#include "libnewsboat-ffi/src/configpaths.rs.h" // IWYU pragma: export

#include <string>

#include "cliargsparser.h"

namespace newsboat {
class ConfigPaths {
public:
	ConfigPaths();
	~ConfigPaths() = default;

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
	Filepath url_file() const;

	/// Path to the cache file.
	Filepath cache_file() const;

	/// Sets path to the cache file.
	// FIXME: this is actually a kludge that lets Controller change the path
	// midway. That logic should be moved into ConfigPaths, and this method
	// removed.
	void set_cache_file(const std::string&);

	/// Path to the config file.
	Filepath config_file() const;

	/// Path to the lock file.
	///
	/// \note This changes when path to config file changes.
	Filepath lock_file() const;

	/// \brief Path to the queue file.
	///
	/// Queue file stores enqueued podcasts. It's written by Newsboat, and
	/// read by Podboat.
	Filepath queue_file() const;

	/// Path to the file with previous search queries.
	Filepath search_history_file() const;

	/// Path to the file with command-line history.
	Filepath cmdline_history_file() const;
private:
	rust::Box<configpaths::bridged::ConfigPaths> rs_object;
};
} // namespace newsboat

#endif /* NEWSBOAT_CONFIGPATHS_H_ */

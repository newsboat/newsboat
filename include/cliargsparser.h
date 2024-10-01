#ifndef NEWSBOAT_CLIARGSPARSER_H_
#define NEWSBOAT_CLIARGSPARSER_H_

#include "libnewsboat-ffi/src/cliargsparser.rs.h"

#include <string>
#include <vector>

#include "3rd-party/optional.hpp"

#include "logger.h"

namespace newsboat {
class CliArgsParser {
public:
	CliArgsParser(int argc, char* argv[]);
	~CliArgsParser() = default;

	bool do_import() const;

	bool do_export() const;

	bool export_as_opml2() const;

	bool do_vacuum() const;

	bool do_cleanup() const;

	Filepath importfile() const;

	/// If non-null, Newsboat should import read articles info from this
	/// filepath.
	nonstd::optional<Filepath> readinfo_import_file() const;

	/// If non-null, Newsboat should export read articles info to this
	/// filepath.
	nonstd::optional<Filepath> readinfo_export_file() const;

	std::string program_name() const;

	unsigned int show_version() const;

	bool silent() const;

	bool using_nonstandard_configs() const;

	/// If non-null, the creator of `CliArgsParser` object should call `exit()`
	/// with this code.
	nonstd::optional<int> return_code() const;

	/// If `display_msg()` is not empty, the creator of `CliArgsParser` should
	/// print its contents to stderr.
	///
	/// \note The contents of this string should be checked before processing
	/// `return_code()`.
	std::string display_msg() const;

	/// If `should_print_usage()` is `true`, the creator of `CliArgsParser`
	/// object should print usage information.
	///
	/// \note This field should be checked before processing
	/// `should_return`.
	bool should_print_usage() const;

	bool refresh_on_start() const;

	nonstd::optional<Filepath> url_file() const;

	nonstd::optional<Filepath> lock_file() const;

	nonstd::optional<Filepath> cache_file() const;

	nonstd::optional<Filepath> config_file() const;

	nonstd::optional<std::string> queue_file() const;

	nonstd::optional<std::string> search_history_file() const;

	nonstd::optional<std::string> cmdline_history_file() const;

	/// If non-empty, Newsboat should execute these commands and then quit.
	///
	/// \note The parser does not check if the passed commands are valid.
	std::vector<std::string> cmds_to_execute() const;

	nonstd::optional<Filepath> log_file() const;

	nonstd::optional<Level> log_level() const;

	/// Returns the reference to the Rust object.
	///
	/// This is only meant to be used in situations when one wants to pass
	/// a reference back to Rust.
	const cliargsparser::bridged::CliArgsParser& get_rust_ref() const;

private:
	rust::Box<cliargsparser::bridged::CliArgsParser> rs_object;
};
} // namespace newsboat

#endif /* NEWSBOAT_CLIARGSPARSER_H_ */

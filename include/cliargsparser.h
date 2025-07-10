#ifndef NEWSBOAT_CLIARGSPARSER_H_
#define NEWSBOAT_CLIARGSPARSER_H_

#include "libNewsboat-ffi/src/cliargsparser.rs.h" // IWYU pragma: export

#include <optional>
#include <string>
#include <vector>

#include "logger.h"

namespace Newsboat {
class CliArgsParser {
public:
	CliArgsParser(int argc, char* argv[]);
	~CliArgsParser() = default;

	bool do_import() const;

	bool do_export() const;

	bool export_as_opml2() const;

	bool do_vacuum() const;

	bool do_cleanup() const;

	std::string importfile() const;

	/// If non-null, Newsboat should import read articles info from this
	/// filepath.
	std::optional<std::string> readinfo_import_file() const;

	/// If non-null, Newsboat should export read articles info to this
	/// filepath.
	std::optional<std::string> readinfo_export_file() const;

	std::string program_name() const;

	unsigned int show_version() const;

	bool silent() const;

	bool using_nonstandard_configs() const;

	/// If non-null, the creator of `CliArgsParser` object should call `exit()`
	/// with this code.
	std::optional<int> return_code() const;

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

	std::optional<std::string> url_file() const;

	std::optional<std::string> lock_file() const;

	std::optional<std::string> cache_file() const;

	std::optional<std::string> config_file() const;

	std::optional<std::string> queue_file() const;

	std::optional<std::string> search_history_file() const;

	std::optional<std::string> cmdline_history_file() const;

	/// If non-empty, Newsboat should execute these commands and then quit.
	///
	/// \note The parser does not check if the passed commands are valid.
	std::vector<std::string> cmds_to_execute() const;

	std::optional<std::string> log_file() const;

	std::optional<Level> log_level() const;

	/// Returns the reference to the Rust object.
	///
	/// This is only meant to be used in situations when one wants to pass
	/// a reference back to Rust.
	const cliargsparser::bridged::CliArgsParser& get_rust_ref() const;

private:
	rust::Box<cliargsparser::bridged::CliArgsParser> rs_object;
};
} // namespace Newsboat

#endif /* NEWSBOAT_CLIARGSPARSER_H_ */

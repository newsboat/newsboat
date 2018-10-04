#ifndef NEWSBOAT_CLIARGSPARSER_H_
#define NEWSBOAT_CLIARGSPARSER_H_

#include <string>
#include <vector>

#include "logger.h"

namespace newsboat {
class CLIArgsParser {
public:
	CLIArgsParser(int argc, char* argv[]);

	bool do_import = false;
	bool do_export = false;
	bool do_vacuum = false;
	std::string importfile;
	bool do_read_import = false;
	bool do_read_export = false;
	std::string program_name;
	std::string readinfofile;
	unsigned int show_version = 0;
	bool silent = false;
	bool using_nonstandard_configs = false;

	/// If `should_return` is `true`, the creator of `CLIArgsParser` object
	/// should call `exit(return_code)`.
	// TODO: replace this with std::optional once we upgraded to C++17.
	bool should_return = false;
	int return_code = 0;

	/// If `display_msg` is not empty, the creator of `CLIArgsParser` should
	/// print its contents to stderr.
	///
	/// \note The contents of this string should be checked before
	/// processing `should_return`.
	std::string display_msg;

	/// If `should_print_usage` is `true`, the creator of `CLIArgsParser`
	/// object should print usage information.
	///
	/// \note This field should be checked before processing
	/// `should_return`.
	bool should_print_usage = false;

	bool refresh_on_start = false;

	/// The value of `url_file` should only be used if `set_url_file` is
	/// `true`.
	// TODO: replace this with std::optional once we upgraded to C++17.
	bool set_url_file = false;
	std::string url_file;

	/// The value of `lock_file` should only be used if `set_lock_file` is
	/// `true`.
	// TODO: replace this with std::optional once we upgraded to C++17.
	bool set_lock_file = false;
	std::string lock_file;

	/// The value of `cache_file` should only be used if `set_cache_file` is
	/// `true`.
	// TODO: replace this with std::optional once we upgraded to C++17.
	bool set_cache_file = false;
	std::string cache_file;

	/// The value of `config_file` should only be used if `set_config_file`
	/// is `true`.
	// TODO: replace this with std::optional once we upgraded to C++17.
	bool set_config_file = false;
	std::string config_file;

	/// If 'execute_cmds' is true, the 'CLIArgsParser' object holds commands
	/// that should be executed in cmds_to_execute vector.
	///
	/// \note The parser does not check if the passed commands are valid.
	// TODO: replace this with std::optional once we upgraded to C++17.
	bool execute_cmds = false;
	std::vector<std::string> cmds_to_execute;

	/// The value of `log_file` should only be used if `set_log_file` is
	/// `true`.
	// TODO: replace this with std::optional once we upgraded to C++17.
	bool set_log_file = false;
	std::string log_file;

	/// The value of `log_level` should only be used if `set_log_level` is
	/// `true`.
	// TODO: replace this with std::optional once we upgraded to C++17.
	bool set_log_level = false;
	Level log_level = Level::NONE;
};
} // namespace newsboat

#endif /* NEWSBOAT_CLIARGSPARSER_H_ */

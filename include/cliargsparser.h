#ifndef NEWSBOAT_CLIARGSPARSER_H_
#define NEWSBOAT_CLIARGSPARSER_H_

#include <string>
#include <vector>

#include "logger.h"

namespace newsboat {
class CliArgsParser {
	bool m_do_import = false;
	bool m_do_export = false;
	bool m_do_vacuum = false;
	std::string m_importfile;
	bool m_do_read_import = false;
	bool m_do_read_export = false;
	std::string m_program_name;
	std::string m_readinfofile;
	unsigned int m_show_version = 0;
	bool m_silent = false;
	bool m_using_nonstandard_configs = false;

	/// If `should_return` is `true`, the creator of `CliArgsParser` object
	/// should call `exit(return_code)`.
	// TODO: replace this with std::optional once we upgraded to C++17.
	bool m_should_return = false;
	int m_return_code = 0;

	/// If `display_msg` is not empty, the creator of `CliArgsParser` should
	/// print its contents to stderr.
	///
	/// \note The contents of this string should be checked before
	/// processing `should_return`.
	std::string m_display_msg;

	/// If `should_print_usage` is `true`, the creator of `CliArgsParser`
	/// object should print usage information.
	///
	/// \note This field should be checked before processing
	/// `should_return`.
	bool m_should_print_usage = false;

	bool m_refresh_on_start = false;

	/// The value of `url_file` should only be used if `set_url_file` is
	/// `true`.
	// TODO: replace this with std::optional once we upgraded to C++17.
	bool m_set_url_file = false;
	std::string m_url_file;

	/// The value of `lock_file` should only be used if `set_lock_file` is
	/// `true`.
	// TODO: replace this with std::optional once we upgraded to C++17.
	bool m_set_lock_file = false;
	std::string m_lock_file;

	/// The value of `cache_file` should only be used if `set_cache_file` is
	/// `true`.
	// TODO: replace this with std::optional once we upgraded to C++17.
	bool m_set_cache_file = false;
	std::string m_cache_file;

	/// The value of `config_file` should only be used if `set_config_file`
	/// is `true`.
	// TODO: replace this with std::optional once we upgraded to C++17.
	bool m_set_config_file = false;
	std::string m_config_file;

	/// If 'execute_cmds' is true, the 'CliArgsParser' object holds commands
	/// that should be executed in cmds_to_execute vector.
	///
	/// \note The parser does not check if the passed commands are valid.
	// TODO: replace this with std::optional once we upgraded to C++17.
	bool m_execute_cmds = false;
	std::vector<std::string> m_cmds_to_execute;

	/// The value of `log_file` should only be used if `set_log_file` is
	/// `true`.
	// TODO: replace this with std::optional once we upgraded to C++17.
	bool m_set_log_file = false;
	std::string m_log_file;

	/// The value of `log_level` should only be used if `set_log_level` is
	/// `true`.
	// TODO: replace this with std::optional once we upgraded to C++17.
	bool m_set_log_level = false;
	Level m_log_level = Level::NONE;

public:
	CliArgsParser(int argc, char* argv[]);

	bool do_import() const;

	bool do_export() const;

	bool do_vacuum() const;

	std::string importfile() const;

	bool do_read_import() const;

	bool do_read_export() const;

	std::string readinfofile() const;

	std::string program_name() const;

	unsigned int show_version() const;

	bool silent() const;

	bool using_nonstandard_configs() const;

	/// If `should_return()` is `true`, the creator of `CliArgsParser` object
	/// should call `exit(return_code())`.
	bool should_return() const;
	int return_code() const;

	/// If `display_msg()` is not empty, the creator of `CliArgsParser` should
	/// print its contents to stderr.
	///
	/// \note The contents of this string should be checked before processing
	/// `should_return`.
	std::string display_msg() const;

	/// If `should_print_usage()` is `true`, the creator of `CliArgsParser`
	/// object should print usage information.
	///
	/// \note This field should be checked before processing
	/// `should_return`.
	bool should_print_usage() const;

	bool refresh_on_start() const;

	/// `url_file()` should only be called if `set_url_file()` returned `true`.
	bool set_url_file() const;
	std::string url_file() const;

	/// `lock_file()` should only be called if `set_lock_file()` returned
	/// `true`.
	bool set_lock_file() const;
	std::string lock_file() const;

	/// `cache_file()` should only be called if `set_cache_file()` returned
	/// `true`.
	bool set_cache_file() const;
	std::string cache_file() const;

	/// `config_file()` should only be called if `set_config_file` returned
	/// `true`.
	bool set_config_file() const;
	std::string config_file() const;

	/// If 'execute_cmds()' is true, Newsboat should execute commands from
	/// `cmds_to_execute()`.
	///
	/// \note The parser does not check if the passed commands are valid.
	bool execute_cmds() const;
	std::vector<std::string> cmds_to_execute() const;

	/// `log_file()` should only be called if `set_log_file()` returned `true`.
	bool set_log_file() const;
	std::string log_file() const;

	/// `log_level()` should only be called if `set_log_level` returned `true`.
	bool set_log_level() const;
	Level log_level() const;
};
} // namespace newsboat

#endif /* NEWSBOAT_CLIARGSPARSER_H_ */

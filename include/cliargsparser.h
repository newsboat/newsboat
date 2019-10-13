#ifndef NEWSBOAT_CLIARGSPARSER_H_
#define NEWSBOAT_CLIARGSPARSER_H_

#include <string>
#include <vector>

#include "logger.h"

namespace newsboat {
class CliArgsParser {
	void* rs_cliargsparser = nullptr;

public:
	CliArgsParser(int argc, char* argv[]);
	~CliArgsParser();

	bool do_import() const;

	bool do_export() const;

	bool do_vacuum() const;

	std::string importfile() const;

	/// If `do_read_import()` is `true`, Newsboat should import read
	/// articles info from  `readinfo_import_file()`.
	bool do_read_import() const;
	std::string readinfo_import_file() const;

	/// If `do_read_export()` is `true`, Newsboat should export read
	/// articles info to `readinfo_export_file()`.
	bool do_read_export() const;
	std::string readinfo_export_file() const;

	std::string program_name() const;

	unsigned int show_version() const;

	bool silent() const;

	bool using_nonstandard_configs() const;

	/// If `should_return()` is `true`, the creator of `CliArgsParser`
	/// object should call `exit(return_code())`.
	bool should_return() const;
	int return_code() const;

	/// If `display_msg()` is not empty, the creator of `CliArgsParser`
	/// should print its contents to stderr.
	///
	/// \note The contents of this string should be checked before
	/// processing `should_return`.
	std::string display_msg() const;

	/// If `should_print_usage()` is `true`, the creator of `CliArgsParser`
	/// object should print usage information.
	///
	/// \note This field should be checked before processing
	/// `should_return`.
	bool should_print_usage() const;

	bool refresh_on_start() const;

	/// `url_file()` should only be called if `set_url_file()` returned
	/// `true`.
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

	/// `log_file()` should only be called if `set_log_file()` returned
	/// `true`.
	bool set_log_file() const;
	std::string log_file() const;

	/// `log_level()` should only be called if `set_log_level` returned
	/// `true`.
	bool set_log_level() const;
	Level log_level() const;

	/// Returns the pointer to the Rust object.
	///
	/// This is only meant to be used in situations when one wants to pass
	/// a pointer to CliArgsParser back to Rust.
	void* get_rust_pointer() const;
};
} // namespace newsboat

#endif /* NEWSBOAT_CLIARGSPARSER_H_ */

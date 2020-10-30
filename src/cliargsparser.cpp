#include "cliargsparser.h"

#include <getopt.h>
#include <cstdlib>

#include "globals.h"
#include "ruststring.h"
#include "strprintf.h"

extern "C" {
	void* create_rs_cliargsparser(int argc, char* argv[]);

	void destroy_rs_cliargsparser(void*);

	bool rs_cliargsparser_do_import(void* rs_cliargsparser);

	bool rs_cliargsparser_do_export(void* rs_cliargsparser);

	bool rs_cliargsparser_do_vacuum(void* rs_cliargsparser);

	bool rs_cliargsparser_do_cleanup(void* rs_cliargsparser);

	char* rs_cliargsparser_importfile(void* rs_cliargsparser);

	char* rs_cliargsparser_program_name(void* rs_cliargsparser);

	bool rs_cliargsparser_do_read_import(void* rs_cliargsparser);

	char* rs_cliargsparser_readinfo_import_file(void* rs_cliargsparser);

	bool rs_cliargsparser_do_read_export(void* rs_cliargsparser);

	char* rs_cliargsparser_readinfo_export_file(void* rs_cliargsparser);

	unsigned int rs_cliargsparser_show_version(void* rs_cliargsparser);

	bool rs_cliargsparser_silent(void* rs_cliargsparser);

	bool rs_cliargsparser_using_nonstandard_configs(void* rs_cliargsparser);

	bool rs_cliargsparser_should_return(void* rs_cliargsparser);

	int rs_cliargsparser_return_code(void* rs_cliargsparser);

	char* rs_cliargsparser_display_msg(void* rs_cliargsparser);

	bool rs_cliargsparser_should_print_usage(void* rs_cliargsparser);

	bool rs_cliargsparser_refresh_on_start(void* rs_cliargsparser);

	bool rs_cliargsparser_set_url_file(void* rs_cliargsparser);

	char* rs_cliargsparser_url_file(void* rs_cliargsparser);

	bool rs_cliargsparser_set_lock_file(void* rs_cliargsparser);

	char* rs_cliargsparser_lock_file(void* rs_cliargsparser);

	bool rs_cliargsparser_set_cache_file(void* rs_cliargsparser);

	char* rs_cliargsparser_cache_file(void* rs_cliargsparser);

	bool rs_cliargsparser_set_config_file(void* rs_cliargsparser);

	char* rs_cliargsparser_config_file(void* rs_cliargsparser);

	bool rs_cliargsparser_execute_cmds(void* rs_cliargsparser);

	unsigned int rs_cliargsparser_cmds_to_execute_count(void* rs_cliargsparser);
	char* rs_cliargsparser_cmd_to_execute_n(void* rs_cliargsparser, unsigned int n);

	bool rs_cliargsparser_set_log_file(void* rs_cliargsparser);

	char* rs_cliargsparser_log_file(void* rs_cliargsparser);

	bool rs_cliargsparser_set_log_level(void* rs_cliargsparser);

	unsigned char rs_cliargsparser_log_level(void* rs_cliargsparser);
}

#define GET_VALUE(NAME, DEFAULT) \
	if (rs_cliargsparser) { \
		return rs_cliargsparser_ ## NAME (rs_cliargsparser); \
	} else { \
		return DEFAULT; \
	}

#define GET_STRING(NAME) \
	if (rs_cliargsparser) { \
		return RustString(rs_cliargsparser_ ## NAME (rs_cliargsparser)); \
	} else { \
		return {}; \
	}

#define GET_OPTIONAL_VALUE(CHECKER, GETTER, DEFAULT) \
	if (rs_cliargsparser) { \
		if (rs_cliargsparser_ ## CHECKER (rs_cliargsparser)) { \
			return rs_cliargsparser_ ## GETTER (rs_cliargsparser); \
		} else { \
			return nonstd::nullopt; \
		} \
	} else { \
		return DEFAULT; \
	}

#define GET_OPTIONAL_STRING(CHECKER, GETTER) \
	if (rs_cliargsparser) { \
		if (rs_cliargsparser_ ## CHECKER (rs_cliargsparser)) { \
			return RustString(rs_cliargsparser_ ## GETTER (rs_cliargsparser)); \
		} else { \
			return nonstd::nullopt; \
		} \
	} else { \
		return {}; \
	}

namespace newsboat {

CliArgsParser::CliArgsParser(int argc, char* argv[])
{
	rs_cliargsparser = create_rs_cliargsparser(argc, argv);
}

CliArgsParser::~CliArgsParser()
{
	if (rs_cliargsparser) {
		destroy_rs_cliargsparser(rs_cliargsparser);
	}
}

bool CliArgsParser::do_import() const
{
	GET_VALUE(do_import, false);
}

bool CliArgsParser::do_export() const
{
	GET_VALUE(do_export, false);
}

bool CliArgsParser::do_vacuum() const
{
	GET_VALUE(do_vacuum, false);
}

bool CliArgsParser::do_cleanup() const
{
	GET_VALUE(do_cleanup, false);
}

std::string CliArgsParser::importfile() const
{
	GET_STRING(importfile);
}

nonstd::optional<std::string> CliArgsParser::readinfo_import_file() const
{
	GET_OPTIONAL_STRING(do_read_import, readinfo_import_file);
}

nonstd::optional<std::string> CliArgsParser::readinfo_export_file() const
{
	GET_OPTIONAL_STRING(do_read_export, readinfo_export_file);
}

std::string CliArgsParser::program_name() const
{
	GET_STRING(program_name);
}

unsigned int CliArgsParser::show_version() const
{
	GET_VALUE(show_version, 0);
}

bool CliArgsParser::silent() const
{
	GET_VALUE(silent, false);
}

bool CliArgsParser::using_nonstandard_configs() const
{
	GET_VALUE(using_nonstandard_configs, false);
}

nonstd::optional<int> CliArgsParser::return_code() const
{
	GET_OPTIONAL_VALUE(should_return, return_code, 0);
}

std::string CliArgsParser::display_msg() const
{
	GET_STRING(display_msg);
}

bool CliArgsParser::should_print_usage() const
{
	GET_VALUE(should_print_usage, false);
}

bool CliArgsParser::refresh_on_start() const
{
	GET_VALUE(refresh_on_start, false);
}

nonstd::optional<std::string> CliArgsParser::url_file() const
{
	GET_OPTIONAL_STRING(set_url_file, url_file);
}

nonstd::optional<std::string> CliArgsParser::lock_file() const
{
	GET_OPTIONAL_STRING(set_lock_file, lock_file);
}

nonstd::optional<std::string> CliArgsParser::cache_file() const
{
	GET_OPTIONAL_STRING(set_cache_file, cache_file);
}

nonstd::optional<std::string> CliArgsParser::config_file() const
{
	GET_OPTIONAL_STRING(set_config_file, config_file);
}

nonstd::optional<std::vector<std::string>> CliArgsParser::cmds_to_execute()
	const
{
	if (rs_cliargsparser) {
		if (rs_cliargsparser_execute_cmds(rs_cliargsparser)) {
			std::vector<std::string> result;

			const auto count = rs_cliargsparser_cmds_to_execute_count(rs_cliargsparser);
			for (unsigned int i = 0; i < count; ++i) {
				result.push_back(RustString(rs_cliargsparser_cmd_to_execute_n(rs_cliargsparser,
							i)));
			}

			return result;
		} else {
			return nonstd::nullopt;
		}
	} else {
		return {};
	}
}

nonstd::optional<std::string> CliArgsParser::log_file() const
{
	GET_OPTIONAL_STRING(set_log_file, log_file);
}

nonstd::optional<Level> CliArgsParser::log_level() const
{
	if (rs_cliargsparser) {
		if (rs_cliargsparser_set_log_level(rs_cliargsparser)) {
			return static_cast<Level>(rs_cliargsparser_log_level(rs_cliargsparser));
		} else {
			return nonstd::nullopt;
		}
	} else {
		return Level::NONE;
	}
}

void* CliArgsParser::get_rust_pointer() const
{
	return rs_cliargsparser;
}

} // namespace newsboat


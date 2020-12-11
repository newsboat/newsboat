#include "cliargsparser.h"

#include <getopt.h>
#include <cstdlib>

#include "globals.h"
#include "ruststring.h"
#include "strprintf.h"

extern "C" {
	void* create_rs_cliargsparser(int argc, char* argv[]);

	void destroy_rs_cliargsparser(void*);

	bool rs_cliargsparser_execute_cmds(void* rs_cliargsparser);

	unsigned int rs_cliargsparser_cmds_to_execute_count(void* rs_cliargsparser);
	char* rs_cliargsparser_cmd_to_execute_n(void* rs_cliargsparser, unsigned int n);

	bool rs_cliargsparser_set_log_level(void* rs_cliargsparser);

	char rs_cliargsparser_log_level(void* rs_cliargsparser);
}

namespace newsboat {


rust::Vec<rust::String> argv_to_rust_args(int argc, char* argv[])
{
	rust::Vec<rust::String> args;
	for (int i = 0; i < argc; ++i) {
		// TODO: Handle invalid utf-8 codepoints gracefully?
		args.push_back(argv[i]);
	}
	return args;
}

CliArgsParser::CliArgsParser(int argc, char* argv[])
	: rs_object(cliargsparser::bridged::create(argv_to_rust_args(argc, argv)))
{
	rs_cliargsparser = create_rs_cliargsparser(argc, argv); // TODO: Remove
}

CliArgsParser::~CliArgsParser()
{
	// TODO: Remove, mark destructor with `= default`
	if (rs_cliargsparser) {
		destroy_rs_cliargsparser(rs_cliargsparser);
	}
}

bool CliArgsParser::do_import() const
{
	return newsboat::cliargsparser::bridged::do_import(*rs_object);
}

bool CliArgsParser::do_export() const
{
	return newsboat::cliargsparser::bridged::do_export(*rs_object);
}

bool CliArgsParser::do_vacuum() const
{
	return newsboat::cliargsparser::bridged::do_vacuum(*rs_object);
}

bool CliArgsParser::do_cleanup() const
{
	return newsboat::cliargsparser::bridged::do_cleanup(*rs_object);
}

std::string CliArgsParser::importfile() const
{
	return std::string(newsboat::cliargsparser::bridged::importfile(*rs_object));
}

nonstd::optional<std::string> CliArgsParser::readinfo_import_file() const
{
	rust::String path;
	if (newsboat::cliargsparser::bridged::readinfo_import_file(*rs_object, path)) {
		return std::string(path);
	}
	return nonstd::nullopt;
}

nonstd::optional<std::string> CliArgsParser::readinfo_export_file() const
{
	rust::String path;
	if (newsboat::cliargsparser::bridged::readinfo_export_file(*rs_object, path)) {
		return std::string(path);
	}
	return nonstd::nullopt;
}

std::string CliArgsParser::program_name() const
{
	return std::string(newsboat::cliargsparser::bridged::program_name(*rs_object));
}

unsigned int CliArgsParser::show_version() const
{
	return newsboat::cliargsparser::bridged::do_show_version(*rs_object);
}

bool CliArgsParser::silent() const
{
	return newsboat::cliargsparser::bridged::silent(*rs_object);
}

bool CliArgsParser::using_nonstandard_configs() const
{
	return newsboat::cliargsparser::bridged::using_nonstandard_configs(*rs_object);
}

nonstd::optional<int> CliArgsParser::return_code() const
{
	rust::isize code = 0;
	if (newsboat::cliargsparser::bridged::return_code(*rs_object, code)) {
		return static_cast<int>(code);
	}
	return nonstd::nullopt;
}

std::string CliArgsParser::display_msg() const
{
	return std::string(newsboat::cliargsparser::bridged::display_msg(*rs_object));
}

bool CliArgsParser::should_print_usage() const
{
	return newsboat::cliargsparser::bridged::should_print_usage(*rs_object);
}

bool CliArgsParser::refresh_on_start() const
{
	return newsboat::cliargsparser::bridged::refresh_on_start(*rs_object);
}

nonstd::optional<std::string> CliArgsParser::url_file() const
{
	rust::String path;
	if (newsboat::cliargsparser::bridged::url_file(*rs_object, path)) {
		return std::string(path);
	}
	return nonstd::nullopt;
}

nonstd::optional<std::string> CliArgsParser::lock_file() const
{
	rust::String path;
	if (newsboat::cliargsparser::bridged::lock_file(*rs_object, path)) {
		return std::string(path);
	}
	return nonstd::nullopt;
}

nonstd::optional<std::string> CliArgsParser::cache_file() const
{
	rust::String path;
	if (newsboat::cliargsparser::bridged::cache_file(*rs_object, path)) {
		return std::string(path);
	}
	return nonstd::nullopt;
}

nonstd::optional<std::string> CliArgsParser::config_file() const
{
	rust::String path;
	if (newsboat::cliargsparser::bridged::config_file(*rs_object, path)) {
		return std::string(path);
	}
	return nonstd::nullopt;
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
	rust::String path;
	if (newsboat::cliargsparser::bridged::log_file(*rs_object, path)) {
		return std::string(path);
	}
	return nonstd::nullopt;
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
		return nonstd::nullopt;
	}
}

void* CliArgsParser::get_rust_pointer() const
{
	return rs_cliargsparser;
}

} // namespace newsboat


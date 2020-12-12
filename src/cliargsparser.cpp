#include "cliargsparser.h"

#include <getopt.h>
#include <cstdlib>

#include "globals.h"
#include "ruststring.h"
#include "strprintf.h"

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

std::vector<std::string> CliArgsParser::cmds_to_execute()
const
{
	const auto rs_cmds = newsboat::cliargsparser::bridged::cmds_to_execute(
			*rs_object);

	std::vector<std::string> cmds;
	for (const auto& cmd : rs_cmds) {
		cmds.push_back(std::string(cmd));
	}
	return cmds;
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
	std::int8_t level;
	if (newsboat::cliargsparser::bridged::log_level(*rs_object, level)) {
		return static_cast<Level>(level);
	}
	return nonstd::nullopt;
}

void* CliArgsParser::get_rust_pointer() const
{
	return (void*)&*rs_object;
}

} // namespace newsboat


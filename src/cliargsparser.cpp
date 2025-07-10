#include "cliargsparser.h"

namespace Newsboat {

rust::Vec<cliargsparser::bridged::BytesVec> argv_to_rust_args(int argc, char* argv[])
{
	rust::Vec<cliargsparser::bridged::BytesVec> args;
	args.reserve(argc);
	for (int i = 0; i < argc; ++i) {
		cliargsparser::bridged::BytesVec arg;
		rust::Vec<uint8_t> data;
		for (auto j = 0; argv[i][j] != '\0'; ++j) {
			data.push_back(argv[i][j]);
		}
		arg.data = std::move(data);
		args.push_back(arg);
	}
	return args;
}

CliArgsParser::CliArgsParser(int argc, char* argv[])
	: rs_object(cliargsparser::bridged::create(argv_to_rust_args(argc, argv)))
{
}

bool CliArgsParser::do_import() const
{
	return Newsboat::cliargsparser::bridged::do_import(*rs_object);
}

bool CliArgsParser::do_export() const
{
	return Newsboat::cliargsparser::bridged::do_export(*rs_object);
}

bool CliArgsParser::export_as_opml2() const
{
	return Newsboat::cliargsparser::bridged::export_as_opml2(*rs_object);
}

bool CliArgsParser::do_vacuum() const
{
	return Newsboat::cliargsparser::bridged::do_vacuum(*rs_object);
}

bool CliArgsParser::do_cleanup() const
{
	return Newsboat::cliargsparser::bridged::do_cleanup(*rs_object);
}

std::string CliArgsParser::importfile() const
{
	return std::string(Newsboat::cliargsparser::bridged::importfile(*rs_object));
}

std::optional<std::string> CliArgsParser::readinfo_import_file() const
{
	rust::String path;
	if (Newsboat::cliargsparser::bridged::readinfo_import_file(*rs_object, path)) {
		return std::string(path);
	}
	return std::nullopt;
}

std::optional<std::string> CliArgsParser::readinfo_export_file() const
{
	rust::String path;
	if (Newsboat::cliargsparser::bridged::readinfo_export_file(*rs_object, path)) {
		return std::string(path);
	}
	return std::nullopt;
}

std::string CliArgsParser::program_name() const
{
	return std::string(Newsboat::cliargsparser::bridged::program_name(*rs_object));
}

unsigned int CliArgsParser::show_version() const
{
	return Newsboat::cliargsparser::bridged::do_show_version(*rs_object);
}

bool CliArgsParser::silent() const
{
	return Newsboat::cliargsparser::bridged::silent(*rs_object);
}

bool CliArgsParser::using_nonstandard_configs() const
{
	return Newsboat::cliargsparser::bridged::using_nonstandard_configs(*rs_object);
}

std::optional<int> CliArgsParser::return_code() const
{
	rust::isize code = 0;
	if (Newsboat::cliargsparser::bridged::return_code(*rs_object, code)) {
		return static_cast<int>(code);
	}
	return std::nullopt;
}

std::string CliArgsParser::display_msg() const
{
	return std::string(Newsboat::cliargsparser::bridged::display_msg(*rs_object));
}

bool CliArgsParser::should_print_usage() const
{
	return Newsboat::cliargsparser::bridged::should_print_usage(*rs_object);
}

bool CliArgsParser::refresh_on_start() const
{
	return Newsboat::cliargsparser::bridged::refresh_on_start(*rs_object);
}

std::optional<std::string> CliArgsParser::url_file() const
{
	rust::String path;
	if (Newsboat::cliargsparser::bridged::url_file(*rs_object, path)) {
		return std::string(path);
	}
	return std::nullopt;
}

std::optional<std::string> CliArgsParser::lock_file() const
{
	rust::String path;
	if (Newsboat::cliargsparser::bridged::lock_file(*rs_object, path)) {
		return std::string(path);
	}
	return std::nullopt;
}

std::optional<std::string> CliArgsParser::cache_file() const
{
	rust::String path;
	if (Newsboat::cliargsparser::bridged::cache_file(*rs_object, path)) {
		return std::string(path);
	}
	return std::nullopt;
}

std::optional<std::string> CliArgsParser::config_file() const
{
	rust::String path;
	if (Newsboat::cliargsparser::bridged::config_file(*rs_object, path)) {
		return std::string(path);
	}
	return std::nullopt;
}

std::optional<std::string> CliArgsParser::queue_file() const
{
	rust::String path;
	if (Newsboat::cliargsparser::bridged::queue_file(*rs_object, path)) {
		return std::string(path);
	}
	return std::nullopt;
}

std::optional<std::string> CliArgsParser::search_history_file() const
{
	rust::String path;
	if (Newsboat::cliargsparser::bridged::search_history_file(*rs_object, path)) {
		return std::string(path);
	}
	return std::nullopt;
}

std::optional<std::string> CliArgsParser::cmdline_history_file() const
{
	rust::String path;
	if (Newsboat::cliargsparser::bridged::cmdline_history_file(*rs_object, path)) {
		return std::string(path);
	}
	return std::nullopt;
}

std::vector<std::string> CliArgsParser::cmds_to_execute()
const
{
	const auto rs_cmds = Newsboat::cliargsparser::bridged::cmds_to_execute(
			*rs_object);

	std::vector<std::string> cmds;
	for (const auto& cmd : rs_cmds) {
		cmds.push_back(std::string(cmd));
	}
	return cmds;
}

std::optional<std::string> CliArgsParser::log_file() const
{
	rust::String path;
	if (Newsboat::cliargsparser::bridged::log_file(*rs_object, path)) {
		return std::string(path);
	}
	return std::nullopt;
}

std::optional<Level> CliArgsParser::log_level() const
{
	std::int8_t level;
	if (Newsboat::cliargsparser::bridged::log_level(*rs_object, level)) {
		return static_cast<Level>(level);
	}
	return std::nullopt;
}

const cliargsparser::bridged::CliArgsParser& CliArgsParser::get_rust_ref() const
{
	return *rs_object;
}

} // namespace Newsboat

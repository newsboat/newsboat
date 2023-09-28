#include "cliargsparser.h"

namespace newsboat {

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
	return newsboat::cliargsparser::bridged::do_import(*rs_object);
}

bool CliArgsParser::do_export() const
{
	return newsboat::cliargsparser::bridged::do_export(*rs_object);
}

bool CliArgsParser::export_as_opml2() const
{
	return newsboat::cliargsparser::bridged::export_as_opml2(*rs_object);
}

bool CliArgsParser::do_vacuum() const
{
	return newsboat::cliargsparser::bridged::do_vacuum(*rs_object);
}

bool CliArgsParser::do_cleanup() const
{
	return newsboat::cliargsparser::bridged::do_cleanup(*rs_object);
}

Filepath CliArgsParser::importfile() const
{
	return std::string(newsboat::cliargsparser::bridged::importfile(*rs_object));
}

std::optional<Filepath> CliArgsParser::readinfo_import_file() const
{
	rust::String path;
	if (newsboat::cliargsparser::bridged::readinfo_import_file(*rs_object, path)) {
		return std::string(path);
	}
	return std::nullopt;
}

std::optional<Filepath> CliArgsParser::readinfo_export_file() const
{
	rust::String path;
	if (newsboat::cliargsparser::bridged::readinfo_export_file(*rs_object, path)) {
		return std::string(path);
	}
	return std::nullopt;
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

std::optional<int> CliArgsParser::return_code() const
{
	rust::isize code = 0;
	if (newsboat::cliargsparser::bridged::return_code(*rs_object, code)) {
		return static_cast<int>(code);
	}
	return std::nullopt;
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

std::optional<Filepath> CliArgsParser::url_file() const
{
	rust::String path;
	if (newsboat::cliargsparser::bridged::url_file(*rs_object, path)) {
		return std::string(path);
	}
	return std::nullopt;
}

std::optional<Filepath> CliArgsParser::lock_file() const
{
	rust::String path;
	if (newsboat::cliargsparser::bridged::lock_file(*rs_object, path)) {
		return std::string(path);
	}
	return std::nullopt;
}

std::optional<Filepath> CliArgsParser::cache_file() const
{
	rust::String path;
	if (newsboat::cliargsparser::bridged::cache_file(*rs_object, path)) {
		return std::string(path);
	}
	return std::nullopt;
}

std::optional<Filepath> CliArgsParser::config_file() const
{
	rust::String path;
	if (newsboat::cliargsparser::bridged::config_file(*rs_object, path)) {
		return std::string(path);
	}
	return std::nullopt;
}

std::optional<std::string> CliArgsParser::queue_file() const
{
	rust::String path;
	if (newsboat::cliargsparser::bridged::queue_file(*rs_object, path)) {
		return std::string(path);
	}
	return std::nullopt;
}

std::optional<std::string> CliArgsParser::search_history_file() const
{
	rust::String path;
	if (newsboat::cliargsparser::bridged::search_history_file(*rs_object, path)) {
		return std::string(path);
	}
	return std::nullopt;
}

std::optional<std::string> CliArgsParser::cmdline_history_file() const
{
	rust::String path;
	if (newsboat::cliargsparser::bridged::cmdline_history_file(*rs_object, path)) {
		return std::string(path);
	}
	return std::nullopt;
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

std::optional<Filepath> CliArgsParser::log_file() const
{
	rust::String path;
	if (newsboat::cliargsparser::bridged::log_file(*rs_object, path)) {
		return std::string(path);
	}
	return std::nullopt;
}

std::optional<Level> CliArgsParser::log_level() const
{
	std::int8_t level;
	if (newsboat::cliargsparser::bridged::log_level(*rs_object, level)) {
		return static_cast<Level>(level);
	}
	return std::nullopt;
}

const cliargsparser::bridged::CliArgsParser& CliArgsParser::get_rust_ref() const
{
	return *rs_object;
}

} // namespace newsboat

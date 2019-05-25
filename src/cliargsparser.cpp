#include "cliargsparser.h"

#include <getopt.h>
#include <cstdlib>

#include "globals.h"
#include "strprintf.h"

namespace newsboat {

CliArgsParser::CliArgsParser(int argc, char* argv[])
{
	int c;

	m_program_name = argv[0];

	static const char getopt_str[] = "i:erhqu:c:C:d:l:vVx:XI:E:";
	static const struct option longopts[] = {
		{"cache-file", required_argument, 0, 'c'},
		{"config-file", required_argument, 0, 'C'},
		{"execute", required_argument, 0, 'x'},
		{"export-to-file", required_argument, 0, 'E'},
		{"export-to-opml", no_argument, 0, 'e'},
		{"help", no_argument, 0, 'h'},
		{"import-from-file", required_argument, 0, 'I'},
		{"import-from-opml", required_argument, 0, 'i'},
		{"log-file", required_argument, 0, 'd'},
		{"log-level", required_argument, 0, 'l'},
		{"quiet", no_argument, 0, 'q'},
		{"refresh-on-start", no_argument, 0, 'r'},
		{"url-file", required_argument, 0, 'u'},
		{"vacuum", no_argument, 0, 'X'},
		{"version", no_argument, 0, 'v'},
		{0, 0, 0, 0}};

	// Ask getopt to re-initialize itself.
	//
	// This isn't necessary for real-world use, because we parse arguments
	// only once; but this is *very* important in tests, where CliArgsParser
	// is ran dozens of times.
	//
	// Note we use 0, not 1. getopt(3) says that the value of 0 forces
	// getopt() to re-initialize some more internal stuff. This shouldn't
	// make a difference in our case, but somehow it does - if we use 1,
	// tests start to fail.
	optind = 0;

	while ((c = ::getopt_long(argc, argv, getopt_str, longopts, nullptr)) !=
		-1) {
		switch (c) {
		case ':': /* fall-through */
		case '?': /* missing option */
			m_should_print_usage = true;

			m_should_return = true;
			m_return_code = EXIT_FAILURE;

			break;
		case 'i':
			if (m_do_export) {
				m_should_print_usage = true;

				m_should_return = true;
				m_return_code = EXIT_FAILURE;

				break;
			}
			m_do_import = true;
			m_importfile = optarg;
			break;
		case 'r':
			m_refresh_on_start = true;
			break;
		case 'e':
			// disable logging of newsboat's startup progress to
			// stdout, because the OPML export will be printed to
			// stdout.
			m_silent = true;
			if (m_do_import) {
				m_should_print_usage = true;

				m_should_return = true;
				m_return_code = EXIT_FAILURE;

				break;
			}
			m_do_export = true;
			break;
		case 'h':
			m_should_print_usage = true;
			m_should_return = true;
			m_return_code = EXIT_SUCCESS;
			break;
		case 'u':
			m_set_url_file = true;
			m_url_file = optarg;
			m_using_nonstandard_configs = true;
			break;
		case 'c':
			m_set_cache_file = true;
			m_cache_file = optarg;
			m_set_lock_file = true;
			m_lock_file = std::string(m_cache_file) + LOCK_SUFFIX;
			m_using_nonstandard_configs = true;
			break;
		case 'C':
			m_set_config_file = true;
			m_config_file = optarg;
			m_using_nonstandard_configs = true;
			break;
		case 'X':
			m_do_vacuum = true;
			break;
		case 'v':
		case 'V':
			m_show_version++;
			break;
		case 'x':
			// disable logging of newsboat's startup progress to
			// stdout, because the command execution result will be
			// printed to stdout
			m_silent = true;

			m_execute_cmds = true;
			m_cmds_to_execute.push_back(optarg);
			while (optind < argc && *argv[optind] != '-') {
				m_cmds_to_execute.push_back(argv[optind]);
				optind++;
			}
			break;
		case 'q':
			m_silent = true;
			break;
		case 'd':
			m_set_log_file = true;
			m_log_file = optarg;
			break;
		case 'l': {
			Level l = static_cast<Level>(atoi(optarg));
			if (l > Level::NONE && l <= Level::DEBUG) {
				m_set_log_level = true;
				m_log_level = l;
			} else {
				m_display_msg =
					strprintf::fmt(_("%s: %s: invalid "
							 "loglevel value"),
						argv[0],
						optarg);

				m_should_return = true;
				m_return_code = EXIT_FAILURE;

				break;
			}
		} break;
		case 'I':
			if (m_do_read_export) {
				m_should_print_usage = true;

				m_should_return = true;
				m_return_code = EXIT_FAILURE;

				break;
			}
			m_do_read_import = true;
			m_readinfofile = optarg;
			break;
		case 'E':
			if (m_do_read_import) {
				m_should_print_usage = true;

				m_should_return = true;
				m_return_code = EXIT_FAILURE;

				break;
			}
			m_do_read_export = true;
			m_readinfofile = optarg;
			break;
		}
	};
}

bool CliArgsParser::do_import() const {
	return m_do_import;
}

bool CliArgsParser::do_export() const {
	return m_do_export;
}

bool CliArgsParser::do_vacuum() const {
	return m_do_vacuum;
}

std::string CliArgsParser::importfile() const {
	return m_importfile;
}

bool CliArgsParser::do_read_import() const {
	return m_do_read_import;
}

bool CliArgsParser::do_read_export() const {
	return m_do_read_export;
}

std::string CliArgsParser::readinfofile() const {
	return m_readinfofile;
}

std::string CliArgsParser::program_name() const {
	return m_program_name;
}

unsigned int CliArgsParser::show_version() const {
	return m_show_version;
}

bool CliArgsParser::silent() const {
	return m_silent;
}

bool CliArgsParser::using_nonstandard_configs() const {
	return m_using_nonstandard_configs;
}

bool CliArgsParser::should_return() const {
	return m_should_return;
}

int CliArgsParser::return_code() const {
	return m_return_code;
}

std::string CliArgsParser::display_msg() const {
	return m_display_msg;
}

bool CliArgsParser::should_print_usage() const {
	return m_should_print_usage;
}

bool CliArgsParser::refresh_on_start() const {
	return m_refresh_on_start;
}

bool CliArgsParser::set_url_file() const {
	return m_set_url_file;
}

std::string CliArgsParser::url_file() const {
	return m_url_file;
}

bool CliArgsParser::set_lock_file() const {
	return m_set_lock_file;
}

std::string CliArgsParser::lock_file() const {
	return m_lock_file;
}

bool CliArgsParser::set_cache_file() const {
	return m_set_cache_file;
}

std::string CliArgsParser::cache_file() const {
	return m_cache_file;
}

bool CliArgsParser::set_config_file() const {
	return m_set_config_file;
}

std::string CliArgsParser::config_file() const {
	return m_config_file;
}

bool CliArgsParser::execute_cmds() const {
	return m_execute_cmds;
}

std::vector<std::string> CliArgsParser::cmds_to_execute() const {
	return m_cmds_to_execute;
}

bool CliArgsParser::set_log_file() const {
	return m_set_log_file;
}

std::string CliArgsParser::log_file() const {
	return m_log_file;
}

bool CliArgsParser::set_log_level() const {
	return m_set_log_level;
}

Level CliArgsParser::log_level() const {
	return m_log_level;
}

} // namespace newsboat


#include "cliargsparser.h"

#include <getopt.h>
#include <cstdlib>

#include "globals.h"
#include "strprintf.h"

namespace newsboat {

CliArgsParser::CliArgsParser(int argc, char* argv[])
{
	int c;

	program_name = argv[0];

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
			should_print_usage = true;

			should_return = true;
			return_code = EXIT_FAILURE;

			break;
		case 'i':
			if (do_export) {
				should_print_usage = true;

				should_return = true;
				return_code = EXIT_FAILURE;

				break;
			}
			do_import = true;
			importfile = optarg;
			break;
		case 'r':
			refresh_on_start = true;
			break;
		case 'e':
			// disable logging of newsboat's startup progress to
			// stdout, because the OPML export will be printed to
			// stdout.
			silent = true;
			if (do_import) {
				should_print_usage = true;

				should_return = true;
				return_code = EXIT_FAILURE;

				break;
			}
			do_export = true;
			break;
		case 'h':
			should_print_usage = true;
			should_return = true;
			return_code = EXIT_SUCCESS;
			break;
		case 'u':
			set_url_file = true;
			url_file = optarg;
			using_nonstandard_configs = true;
			break;
		case 'c':
			set_cache_file = true;
			cache_file = optarg;
			set_lock_file = true;
			lock_file = std::string(cache_file) + LOCK_SUFFIX;
			using_nonstandard_configs = true;
			break;
		case 'C':
			set_config_file = true;
			config_file = optarg;
			using_nonstandard_configs = true;
			break;
		case 'X':
			do_vacuum = true;
			break;
		case 'v':
		case 'V':
			show_version++;
			break;
		case 'x':
			// disable logging of newsboat's startup progress to
			// stdout, because the command execution result will be
			// printed to stdout
			silent = true;

			execute_cmds = true;
			cmds_to_execute.push_back(optarg);
			while (optind < argc && *argv[optind] != '-') {
				cmds_to_execute.push_back(argv[optind]);
				optind++;
			}
			break;
		case 'q':
			silent = true;
			break;
		case 'd':
			set_log_file = true;
			log_file = optarg;
			break;
		case 'l': {
			Level l = static_cast<Level>(atoi(optarg));
			if (l > Level::NONE && l <= Level::DEBUG) {
				set_log_level = true;
				log_level = l;
			} else {
				display_msg =
					strprintf::fmt(_("%s: %d: invalid "
							 "loglevel value"),
						argv[0],
						static_cast<int>(l));

				should_return = true;
				return_code = EXIT_FAILURE;

				break;
			}
		} break;
		case 'I':
			if (do_read_export) {
				should_print_usage = true;

				should_return = true;
				return_code = EXIT_FAILURE;

				break;
			}
			do_read_import = true;
			readinfofile = optarg;
			break;
		case 'E':
			if (do_read_import) {
				should_print_usage = true;

				should_return = true;
				return_code = EXIT_FAILURE;

				break;
			}
			do_read_export = true;
			readinfofile = optarg;
			break;
		}
	};
}

} // namespace newsboat

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <ncurses.h>
#include <sstream>
#include <string>
#include <sys/utsname.h>
#include <utility>
#include <vector>

#include "cache.h"
#include "cliargsparser.h"
#include "config.h"
#include "configpaths.h"
#include "controller.h"
#include "dbexception.h"
#include "exception.h"
#include "matcherexception.h"
#include "rss/parser.h"
#include "stflpp.h"
#include "utils.h"
#include "view.h"
#include "xlicense.h"

extern "C" {
	void rs_setup_human_panic(void);
}

using namespace Newsboat;

void print_usage(const std::string& argv0, const ConfigPaths& configpaths)
{
	auto msg = strprintf::fmt(
			_("%s %s\nusage: %s [-i <file>|-e] [-u <urlfile>] "
				"[-c <cachefile>] [-x <command> ...] [-h]\n"),
			PROGRAM_NAME,
			utils::program_version(),
			argv0);
	std::cout << msg;

	struct arg {
		const char name;
		const std::string longname;
		const std::string params;
		const std::string desc;
	};

	static const std::vector<arg> args = {
		{'e', "export-to-opml", "", _s("export OPML feed to stdout")},
		{'-', "export-to-opml2", "", _s("export OPML 2.0 feed including tags to stdout")},
		{'r', "refresh-on-start", "", _s("refresh feeds on start")},
		{'i', "import-from-opml", _s("<file>"), _s("import OPML file")},
		{
			'u',
			"url-file",
			_s("<file>"),
			_s("read RSS feed URLs from <file>")
		},
		{
			'c',
			"cache-file",
			_s("<file>"),
			_s("use <file> as cache file")
		},
		{
			'C',
			"config-file",
			_s("<file>"),
			_s("read configuration from <file>")
		},
		{
			'-',
			"queue-file",
			_s("<file>"),
			_s("use <file> as podcast queue file")
		},
		{
			'-',
			"search-history-file",
			_s("<file>"),
			_s("save the input history of the search to <file>")
		},
		{
			'-',
			"cmdline-history-file",
			_s("<file>"),
			_s("save the input history of the command line to <file>")
		},
		{'X', "vacuum", "", _s("compact the cache")},
		{
			'x',
			"execute",
			_s("<command>..."),
			_s("execute list of commands")
		},
		{'q', "quiet", "", _s("quiet startup")},
		{'v', "version", "", _s("get version information")},
		{
			'l',
			"log-level",
			_s("<loglevel>"),
			_s("write a log with a certain log level (valid values: 1 to 6,"
				" for user error, critical, error, warning, info, and debug respectively)")
		},
		{
			'd',
			"log-file",
			_s("<file>"),
			_s("use <file> as output log file")
		},
		{
			'E',
			"export-to-file",
			_s("<file>"),
			_s("export list of read articles to <file>")
		},
		{
			'I',
			"import-from-file",
			_s("<file>"),
			_s("import list of read articles from <file>")
		},
		{'h', "help", "", _s("this help")},
		{'-', "cleanup", "", _s("remove unreferenced items from cache")}
	};

	std::vector<std::pair<std::string, std::string>> helpLines;
	std::size_t maxLength = 0;
	for (const auto& a : args) {
		std::string longcolumn;
		if (a.name != '-') {
			longcolumn += "-";
			longcolumn += a.name;
			longcolumn += ", ";
		} else {
			longcolumn += "    ";
		}
		longcolumn += "--" + a.longname;
		longcolumn += a.params.size() > 0 ? "=" + a.params : "";

		maxLength = std::max(maxLength, longcolumn.length());
		helpLines.push_back({longcolumn, a.desc});
	}
	for (const auto& helpLine : helpLines) {
		std::cout << std::string(8, ' ') << helpLine.first;
		const auto padding = maxLength - helpLine.first.length();
		std::cout << std::string(2 + padding, ' ') << helpLine.second;
		std::cout << std::endl;
	}
	std::cout << std::endl;

	std::cout << _("Files:") << '\n';
	// i18n: This is printed out by --help before the path to the config file
	const std::string tr_config = _("configuration");
	// i18n This is printed out by --help before the path to the urls file
	const std::string tr_urls = _("feed URLs");
	// i18n This is printed out by --help before the path to the cache file
	const std::string tr_cache = _("cache");
	// i18n: This is printed out by --help before the path to the queue file
	const std::string tr_queue = _("podcast queue");
	// i18n: This is printed out by --help before the path to the search history file
	const std::string tr_search = _("search history");
	// i18n: This is printed out by --help before the path to the cmdline history file
	const std::string tr_cmdline = _("command line history");
	const auto widest = std::max({tr_config.length(), tr_urls.length(), tr_cache.length(),
				tr_search.length(), tr_cmdline.length(), tr_queue.length()});

	const auto print_filepath = [widest](const std::string& name,
	const std::string& value) {
		std::cout << "\t- " << name << ":  " << std::string(
				widest - name.length(), ' ') << value << '\n';
	};

	print_filepath(tr_config, configpaths.config_file());
	print_filepath(tr_urls, configpaths.url_file());
	print_filepath(tr_cache, configpaths.cache_file());
	print_filepath(tr_queue, configpaths.queue_file());
	print_filepath(tr_search, configpaths.search_history_file());
	print_filepath(tr_cmdline, configpaths.cmdline_history_file());

	std::cout << std::endl
		<< _("Support at #Newsboat at https://libera.chat or on our mailing "
			"list https://groups.google.com/g/Newsboat")
		<< std::endl
		<< _("For more information, check out https://Newsboat.org/")
		<< std::endl;
}

void print_version(const std::string& argv0, unsigned int level)
{
	if (level <= 1) {
		std::stringstream ss;
		ss << PROGRAM_NAME << " " << utils::program_version() << " - "
			<< PROGRAM_URL << std::endl;
		ss << "Copyright (C) 2006-2015 Andreas Krennmair"
			<< std::endl;
		ss << "Copyright (C) 2015-2025 Alexander Batischev"
			<< std::endl;
		ss << "Copyright (C) 2006-2017 Newsbeuter contributors"
			<< std::endl;
		ss << "Copyright (C) 2017-2025 Newsboat contributors"
			<< std::endl;
		ss << std::endl;

		ss << strprintf::fmt(
				_("Newsboat is free software licensed "
					"under the MIT License. (Type `%s -vv' "
					"to see the full text.)"),
				argv0)
			<< std::endl;
		ss << _("It bundles:") << std::endl;
		ss << _("- JSON for Modern C++ library, licensed under the MIT License: "
				"https://github.com/nlohmann/json")
			<< std::endl;
		ss << _("- expected-lite library, licensed under the Boost Software "
				"License: https://github.com/martinmoene/expected-lite")
			<< std::endl;
		ss << std::endl;

		struct utsname xuts;
		uname(&xuts);
		ss << PROGRAM_NAME << " " << utils::program_version()
			<< std::endl;
		ss << "System: " << xuts.sysname << " " << xuts.release
			<< " (" << xuts.machine << ")" << std::endl;
#if defined(__GNUC__) && defined(__VERSION__)
		ss << "Compiler: g++ " << __VERSION__ << std::endl;
#endif
		ss << "ncurses: " << curses_version()
			<< " (compiled with " << NCURSES_VERSION << ")"
			<< std::endl;
		ss << "libcurl: " << curl_version() << " (compiled with "
			<< LIBCURL_VERSION << ")" << std::endl;
		ss << "SQLite: " << sqlite3_libversion()
			<< " (compiled with " << SQLITE_VERSION << ")"
			<< std::endl;
		ss << "libxml2: compiled with " << LIBXML_DOTTED_VERSION
			<< std::endl
			<< std::endl;
		std::cout << ss.str();
	} else {
		std::cout << LICENSE_str << std::endl;
	}
}

int main(int argc, char* argv[])
{
	rs_setup_human_panic();
	utils::initialize_ssl_implementation();

	setlocale(LC_CTYPE, "");
	setlocale(LC_MESSAGES, "");

	textdomain(PACKAGE);
	bindtextdomain(PACKAGE, LOCALEDIR);
	// Internally, Newsboat stores all strings in UTF-8, so we require gettext
	// to return messages in that encoding.
	bind_textdomain_codeset(PACKAGE, "UTF-8");

	rsspp::Parser::global_init();

	ConfigPaths configpaths;
	if (!configpaths.initialized()) {
		std::cerr << configpaths.error_message() << std::endl;
		return EXIT_FAILURE;
	}

	Controller c(configpaths);
	Newsboat::View v(c);
	c.set_view(&v);
	CliArgsParser args(argc, argv);

	configpaths.process_args(args);

	if (args.should_print_usage()) {
		print_usage(args.program_name(), configpaths);
		if (args.return_code().has_value()) {
			return args.return_code().value();
		}
	} else if (args.show_version()) {
		print_version(args.program_name(), args.show_version());
		return EXIT_SUCCESS;
	}

	int ret;
	try {
		ret = c.run(args);
	} catch (const Newsboat::DbException& e) {
		Stfl::reset();
		std::cerr << strprintf::fmt(
				_("Caught Newsboat::DbException with message: %s"),
				e.what())
			<< std::endl;
		::exit(EXIT_FAILURE);
	} catch (const Newsboat::MatcherException& e) {
		Stfl::reset();
		std::cerr << strprintf::fmt(
				_("Caught Newsboat::MatcherException with message: %s"),
				e.what())
			<< std::endl;
		::exit(EXIT_FAILURE);
	} catch (const Newsboat::Exception& e) {
		Stfl::reset();
		std::cerr << strprintf::fmt(_("Caught Newsboat::Exception with message: %s"),
				e.what())
			<< std::endl;
		::exit(EXIT_FAILURE);
	}

	rsspp::Parser::global_cleanup();

	return ret;
}

#include <cstring>
#include <errno.h>
#include <iostream>
#include <ncurses.h>
#include <sstream>
#include <sys/utsname.h>

#include "cache.h"
#include "cliargsparser.h"
#include "config.h"
#include "controller.h"
#include "exception.h"
#include "exceptions.h"
#include "rss.h"
#include "rsspp.h"
#include "view.h"
#include "xlicense.h"

extern "C" {
	void rs_setup_human_panic(void);
}

using namespace newsboat;

void print_usage(const std::string& argv0)
{
	auto msg = strprintf::fmt(
		_("%s %s\nusage: %s [-i <file>|-e] [-u <urlfile>] "
		  "[-c <cachefile>] [-x <command> ...] [-h]\n"),
		PROGRAM_NAME,
		PROGRAM_VERSION,
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
		{'r', "refresh-on-start", "", _s("refresh feeds on start")},
		{'i', "import-from-opml", _s("<file>"), _s("import OPML file")},
		{'u',
			"url-file",
			_s("<urlfile>"),
			_s("read RSS feed URLs from <urlfile>")},
		{'c',
			"cache-file",
			_s("<cachefile>"),
			_s("use <cachefile> as cache file")},
		{'C',
			"config-file",
			_s("<configfile>"),
			_s("read configuration from <configfile>")},
		{'X', "vacuum", "", _s("compact the cache")},
		{'x',
			"execute",
			_s("<command>..."),
			_s("execute list of commands")},
		{'q', "quiet", "", _s("quiet startup")},
		{'v', "version", "", _s("get version information")},
		{'l',
			"log-level",
			_s("<loglevel>"),
			_s("write a log with a certain loglevel (valid values: "
			   "1 to "
			   "6)")},
		{'d',
			"log-file",
			_s("<logfile>"),
			_s("use <logfile> as output log file")},
		{'E',
			"export-to-file",
			_s("<file>"),
			_s("export list of read articles to <file>")},
		{'I',
			"import-from-file",
			_s("<file>"),
			_s("import list of read articles from <file>")},
		{'h', "help", "", _s("this help")}};

	std::stringstream ss;
	for (const auto& a : args) {
		std::string longcolumn("-");
		longcolumn += a.name;
		longcolumn += ", --" + a.longname;
		longcolumn += a.params.size() > 0 ? "=" + a.params : "";
		ss << "\t" << longcolumn;
		for (unsigned int j = 0; j < utils::gentabs(longcolumn); j++) {
			ss << "\t";
		}
		ss << a.desc << std::endl;
	}
	std::cout << ss.str();
}

void print_version(const std::string& argv0, unsigned int level)
{
	if (level <= 1) {
		std::stringstream ss;
		ss << PROGRAM_NAME << " " << PROGRAM_VERSION << " - "
			  << PROGRAM_URL << std::endl;
		ss << "Copyright (C) 2006-2015 Andreas Krennmair"
			  << std::endl;
		ss << "Copyright (C) 2015-2019 Alexander Batischev"
			  << std::endl;
		ss << "Copyright (C) 2006-2017 Newsbeuter contributors"
			  << std::endl;
		ss << "Copyright (C) 2017-2019 Newsboat contributors"
			  << std::endl;
		ss << std::endl;

		ss << strprintf::fmt(
				     _("Newsboat is free software licensed "
				       "under the MIT License. (Type `%s -vv' "
				       "to see the full text.)"),
				     argv0)
			  << std::endl;
		ss << _("It bundles JSON for Modern C++ library, "
			       "licensed under the MIT License: "
			       "https://github.com/nlohmann/json")
			  << std::endl;
		ss << _("It bundles an alphanum algorithm implementation "
			"licensed under the MIT license: "
			"http://www.davekoelle.com/alphanum.html")
			<< std::endl;
		ss << std::endl;

		struct utsname xuts;
		uname(&xuts);
		ss << PROGRAM_NAME << " " << PROGRAM_VERSION
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

	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	rsspp::Parser::global_init();

	Controller c;
	newsboat::View v(&c);
	c.set_view(&v);
	CliArgsParser args(argc, argv);

	if (args.should_print_usage) {
		print_usage(args.program_name);
		if (args.should_return) {
			return args.return_code;
		}
	} else if (args.show_version) {
		print_version(args.program_name, args.show_version);
		return EXIT_SUCCESS;
	}

	int ret;
	try {
		ret = c.run(args);
	} catch (const newsboat::DbException& e) {
		std::cerr << strprintf::fmt(
				     _("Caught newsboat::DbException with "
				       "message: %s"),
				     e.what())
			  << std::endl;
		::exit(EXIT_FAILURE);
	} catch (const newsboat::MatcherException& e) {
		std::cerr << strprintf::fmt(
				     _("Caught newsboat::MatcherException with "
				       "message: %s"),
				     e.what())
			  << std::endl;
		::exit(EXIT_FAILURE);
	} catch (const newsboat::Exception& e) {
		std::cerr << strprintf::fmt(_("Caught newsboat::Exception with "
					      "message: %s"),
				     e.what())
			  << std::endl;
		::exit(EXIT_FAILURE);
	}

	rsspp::Parser::global_cleanup();

	return ret;
}

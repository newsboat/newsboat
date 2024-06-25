#define ENABLE_IMPLICIT_FILEPATH_CONVERSIONS

// This has to be included before Catch2 in order to provide the comparison
// operator
#include "filepath.h"

#include "3rd-party/catch.hpp"

#include "cliargsparser.h"
#include "test_helpers/envvar.h"
#include "test_helpers/opts.h"
#include "test_helpers/stringmaker/optional.h"
#include "test_helpers/tempdir.h"

using namespace newsboat;

TEST_CASE(
	"Asks to print usage info and exit with failure if unknown option is "
	"provided",
	"[CliArgsParser]")
{
	auto check = [](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.should_print_usage());
		REQUIRE(args.return_code() == EXIT_FAILURE);
	};

	SECTION("Example No.1") {
		check({"newsboat", "--some-unknown-option"});
	}

	SECTION("Example No.2") {
		check({"newsboat", "-s"});
	}

	SECTION("Example No.3") {
		check({"newsboat", "-m ix"});
	}

	SECTION("Example No.4") {
		check({"newsboat", "-wtf"});
	}
}

TEST_CASE(
	"Sets `do_import` and `importfile` if -i/--import-from-opml is "
	"provided",
	"[CliArgsParser]")
{
	const std::string filename("blogroll.opml");

	auto check = [&filename](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.do_import());
		REQUIRE(args.importfile() == filename);
	};

	SECTION("-i") {
		check({"newsboat", "-i", filename});
	}

	SECTION("--import-from-opml") {
		check({"newsboat", "--import-from-opml=" + filename});
	}
}

TEST_CASE("Resolves tilde to homedir in -i/--import-from-opml",
	"[CliArgsParser]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());

	const std::string filename("feedlist.opml");
	const std::string arg = std::string("~/") + filename;

	auto check = [&filename, &tmp](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.do_import());
		REQUIRE(Filepath::from_locale_string(args.importfile()) == tmp.get_path().join(filename));
	};

	SECTION("-i") {
		check({"newsboat", "-i", arg});
	}

	SECTION("--import-from-opml") {
		check({"newsboat", "--import-from-opml", arg});
	}
}

TEST_CASE(
	"Asks to print usage and exit with failure if both "
	"-i/--import-from-opml and -e/--export-to-opml are provided",
	"[CliArgsParser]")
{
	const std::string importf("import.opml");
	const std::string exportf("export.opml");

	auto check = [](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.should_print_usage());
		REQUIRE(args.return_code() == EXIT_FAILURE);
	};

	SECTION("-i first") {
		check({"newsboat", "-i", importf, "-e", exportf});
	}

	SECTION("-e first") {
		check({"newsboat", "-e", exportf, "-i", importf});
	}
}

TEST_CASE("Sets `refresh_on_start` if -r/--refresh-on-start is provided",
	"[CliArgsParser]")
{
	auto check = [](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.refresh_on_start());
	};

	SECTION("-r") {
		check({"newsboat", "-r"});
	}

	SECTION("--refresh-on-start") {
		check({"newsboat", "--refresh-on-start"});
	}
}

TEST_CASE("Requests silent mode if -e/--export-to-opml is provided",
	"[CliArgsParser]")
{
	const test_helpers::Opts opts{"newsboat", "-e"};

	CliArgsParser args(opts.argc(), opts.argv());

	REQUIRE(args.silent());
}

TEST_CASE("Sets `do_export` if -e/--export-to-opml is provided",
	"[CliArgsParser]")
{
	auto check = [](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.do_export());
	};

	SECTION("-e") {
		check({"newsboat", "-e"});
	}

	SECTION("--export-to-opml") {
		check({"newsboat", "--export-to-opml"});
	}
}

TEST_CASE("Asks to print usage and exit with success if -h/--help is provided",
	"[CliArgsParser]")
{
	auto check = [](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.should_print_usage());
		REQUIRE(args.return_code() == EXIT_SUCCESS);
	};

	SECTION("-h") {
		check({"newsboat", "-h"});
	}

	SECTION("--help") {
		check({"newsboat", "--help"});
	}
}

TEST_CASE(
	"Sets `url_file` and `using_nonstandard_configs` if -u/--url-file "
	"is provided",
	"[CliArgsParser]")
{
	const std::string filename("urlfile");

	auto check = [&filename](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.url_file() == filename);
		REQUIRE(args.using_nonstandard_configs());
	};

	SECTION("-u") {
		check({"newsboat", "-u", filename});
	}

	SECTION("--url-file") {
		check({"newsboat", "--url-file=" + filename});
	}
}

TEST_CASE("Resolves tilde to homedir in -u/--url-file", "[CliArgsParser]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());

	const std::string filename("urlfile");
	const std::string arg = std::string("~/") + filename;

	auto check = [&filename, &tmp](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(Filepath::from_locale_string(args.url_file().value()) == tmp.get_path().join(
				filename));
	};

	SECTION("-u") {
		check({"newsboat", "-u", arg});
	}

	SECTION("--url-file") {
		check({"newsboat", "--url-file", arg});
	}
}

TEST_CASE(
	"Sets `cache_file`, `lock_file`, and `using_nonstandard_configs` "
	"if -c/--cache-file is provided",
	"[CliArgsParser]")
{
	const std::string filename("cache.db");

	auto check = [&filename](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.cache_file() == filename);
		REQUIRE(args.lock_file() == filename + ".lock");
		REQUIRE(args.using_nonstandard_configs());
	};

	SECTION("-c") {
		check({"newsboat", "-c", filename});
	}

	SECTION("--cache-file") {
		check({"newsboat", "--cache-file=" + filename});
	}
}

TEST_CASE("Supports combined short options", "[CliArgsParser]")
{
	const std::string filename("cache.db");

	test_helpers::Opts opts = {"newsboat", "-vc", filename};
	CliArgsParser args(opts.argc(), opts.argv());

	REQUIRE(args.cache_file() == filename);
	REQUIRE(args.lock_file() == filename + ".lock");
	REQUIRE(args.using_nonstandard_configs());
	REQUIRE(args.show_version() == 1);
}

TEST_CASE("Supports combined short option and value", "[CliArgsParser]")
{
	const std::string filename("cache.db");

	test_helpers::Opts opts = {"newsboat", "-c" + filename};
	CliArgsParser args(opts.argc(), opts.argv());

	REQUIRE(args.cache_file() == filename);
	REQUIRE(args.lock_file() == filename + ".lock");
	REQUIRE(args.using_nonstandard_configs());
}

TEST_CASE("Supports `=` between short option and value",
	"[CliArgsParser]")
{
	const std::string filename("cache.db");

	test_helpers::Opts opts = {"newsboat", "-c=" + filename};
	CliArgsParser args(opts.argc(), opts.argv());

	REQUIRE(args.cache_file() == filename);
	REQUIRE(args.lock_file() == filename + ".lock");
	REQUIRE(args.using_nonstandard_configs());
}

TEST_CASE("Resolves tilde to homedir in -c/--cache-file", "[CliArgsParser]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());

	const std::string filename("mycache.db");
	const std::string arg = std::string("~/") + filename;

	auto check = [&filename, &tmp](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(Filepath::from_locale_string(args.cache_file().value()) == tmp.get_path().join(
				filename));
		REQUIRE(Filepath::from_locale_string(args.lock_file().value()) == tmp.get_path().join(
				filename + ".lock"));
	};

	SECTION("-c") {
		check({"newsboat", "-c", arg});
	}

	SECTION("--cache-file") {
		check({"newsboat", "--cache-file", arg});
	}
}

TEST_CASE(
	"Sets `config_file` and `using_nonstandard_configs` if -C/--config-file "
	"is provided",
	"[CliArgsParser]")
{
	const std::string filename("config file");

	auto check = [&filename](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.config_file() == filename);
		REQUIRE(args.using_nonstandard_configs());
	};

	SECTION("-C") {
		check({"newsboat", "-C", filename});
	}

	SECTION("--config-file") {
		check({"newsboat", "--config-file=" + filename});
	}
}

TEST_CASE("Resolves tilde to homedir in -C/--config-file", "[CliArgsParser]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());

	const std::string filename("newsboat-config");
	const std::string arg = std::string("~/") + filename;

	auto check = [&filename, &tmp](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(Filepath::from_locale_string(args.config_file().value()) == tmp.get_path().join(
				filename));
		REQUIRE(args.using_nonstandard_configs());
	};

	SECTION("-C") {
		check({"newsboat", "-C", arg});
	}

	SECTION("--config-file") {
		check({"newsboat", "--config-file", arg});
	}
}

TEST_CASE(
	"Sets `queue_file` and `using_nonstandard_configs` if --queue-file "
	"is provided",
	"[CliArgsParser]")
{
	const std::string filename("queuefile");

	auto check = [&filename](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.queue_file() == filename);
		REQUIRE(args.using_nonstandard_configs());
	};

	SECTION("--queue-file") {
		check({"newsboat", "--queue-file=" + filename});
	}
}

TEST_CASE("Resolves tilde to homedir in --queue-file", "[CliArgsParser]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());

	const std::string filename("queuefile");
	const std::string arg = std::string("~/") + filename;

	auto check = [&filename, &tmp](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(Filepath::from_locale_string(args.queue_file().value()) == tmp.get_path().join(
				filename));
	};

	SECTION("--queue-file") {
		check({"newsboat", "--queue-file", arg});
	}
}

TEST_CASE(
	"Sets `search_history_file` and `using_nonstandard_configs` if --search-history-file "
	"is provided",
	"[CliArgsParser]")
{
	const std::string filename("searchfile");

	auto check = [&filename](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.search_history_file() == filename);
		REQUIRE(args.using_nonstandard_configs());
	};

	SECTION("--search-history-file") {
		check({"newsboat", "--search-history-file=" + filename});
	}
}

TEST_CASE("Resolves tilde to homedir in --search-history-file", "[CliArgsParser]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());

	const std::string filename("searchfile");
	const std::string arg = std::string("~/") + filename;

	auto check = [&filename, &tmp](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(Filepath::from_locale_string(args.search_history_file().value()) ==
			tmp.get_path().join(filename));
	};

	SECTION("--search-history-file") {
		check({"newsboat", "--search-history-file", arg});
	}
}

TEST_CASE(
	"Sets `cmdline_history_file` and `using_nonstandard_configs` if --cmdline-history-file "
	"is provided",
	"[CliArgsParser]")
{
	const std::string filename("cmdlinefile");

	auto check = [&filename](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.cmdline_history_file() == filename);
		REQUIRE(args.using_nonstandard_configs());
	};

	SECTION("--cmdline-history-file") {
		check({"newsboat", "--cmdline-history-file=" + filename});
	}
}

TEST_CASE("Resolves tilde to homedir in --cmdline-history-file", "[CliArgsParser]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());

	const std::string filename("cmdlinefile");
	const std::string arg = std::string("~/") + filename;

	auto check = [&filename, &tmp](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(Filepath::from_locale_string(args.cmdline_history_file().value()) ==
			tmp.get_path().join(filename));
	};

	SECTION("--cmdline-history-file") {
		check({"newsboat", "--cmdline-history-file", arg});
	}
}

TEST_CASE("Sets `do_vacuum` if -X/--vacuum is provided", "[CliArgsParser]")
{
	auto check = [](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.do_vacuum());
	};

	SECTION("-X") {
		check({"newsboat", "-X"});
	}

	SECTION("--vacuum") {
		check({"newsboat", "--vacuum"});
	}
}

TEST_CASE("Sets `do_cleanup` if --cleanup is provided", "[CliArgsParser]")
{
	auto check = [](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.do_cleanup());
	};

	SECTION("--cleanup") {
		check({"newsboat", "--cleanup"});
	}
}

TEST_CASE("Increases `show_version` with each -v/-V/--version provided",
	"[CliArgsParser]")
{
	auto check = [](test_helpers::Opts opts, unsigned int expected_version) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.show_version() == expected_version);
	};

	SECTION("-v => 1") {
		check({"newsboat", "-v"}, 1);
	}

	SECTION("-V => 1") {
		check({"newsboat", "-V"}, 1);
	}

	SECTION("--version => 1") {
		check({"newsboat", "--version"}, 1);
	}

	SECTION("-vV => 2") {
		check({"newsboat", "-vV"}, 2);
	}

	SECTION("--version -v => 2") {
		check({"newsboat", "--version", "-v"}, 2);
	}

	SECTION("-V --version -v => 3") {
		check({"newsboat", "-V", "--version", "-v"}, 3);
	}

	SECTION("-VvVVvvvvV => 9") {
		check({"newsboat", "-VvVVvvvvV"}, 9);
	}
}

TEST_CASE("Requests silent mode if -x/--execute is provided", "[CliArgsParser]")
{
	auto check = [](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.silent());
	};

	SECTION("-x") {
		check({"newsboat", "-x", "reload"});
	}

	SECTION("--execute") {
		check({"newsboat", "--execute", "reload"});
	}
}

TEST_CASE("Sets `execute_cmds` if -x/--execute is provided", "[CliArgsParser]")
{
	auto check = [](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE_FALSE(args.cmds_to_execute().empty());
	};

	SECTION("-x") {
		check({"newsboat", "-x", "reload"});
	}

	SECTION("--execute") {
		check({"newsboat", "--execute", "reload"});
	}
}

TEST_CASE("Inserts commands to cmds_to_execute if -x/--execute is provided",
	"[CliArgsParser]")
{
	auto check = [](test_helpers::Opts opts, const std::vector<std::string>& cmds) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.cmds_to_execute() == cmds);
	};

	SECTION("-x reload") {
		check({"newsboat", "-x", "reload"}, {"reload"});
	}

	SECTION("--execute reload") {
		check({"newsboat", "--execute", "reload"}, {"reload"});
	}

	SECTION("-x reload print-unread") {
		check({"newsboat", "-x", "reload", "print-unread"},
		{"reload", "print-unread"});
	}

	SECTION("--execute reload print-unread") {
		check({"newsboat", "--execute", "reload", "print-unread"},
		{"reload", "print-unread"});
	}

	SECTION("Multiple occurrences of the option") {
		check({"newsboat", "-x", "print-unread", "--execute", "reload"},
		{"print-unread", "reload"});
	}
}

TEST_CASE("Requests silent mode if -q/--quiet is provided", "[CliArgsParser]")
{
	auto check = [](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.silent());
	};

	SECTION("-q") {
		check({"newsboat", "-q"});
	}

	SECTION("--quiet") {
		check({"newsboat", "--quiet"});
	}
}

TEST_CASE(
	"Sets `readinfofile` if -I/--import-from-file is provided",
	"[CliArgsParser]")
{
	const std::string filename("filename");

	auto check = [&filename](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.readinfo_import_file() == filename);
	};

	SECTION("-I") {
		check({"newsboat", "-I", filename});
	}

	SECTION("--import-from-file") {
		check({"newsboat", "--import-from-file=" + filename});
	}
}

TEST_CASE("Resolves tilde to homedir in -I/--import-from-file",
	"[CliArgsParser]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());

	const std::string filename("read.txt");
	const std::string arg = std::string("~/") + filename;

	auto check = [&filename, &tmp](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(Filepath::from_locale_string(args.readinfo_import_file().value()) ==
			tmp.get_path().join(filename));
	};

	SECTION("-I") {
		check({"newsboat", "-I", arg});
	}

	SECTION("--import-from-file") {
		check({"newsboat", "--import-from-file", arg});
	}
}

TEST_CASE(
	"Sets `readinfofile` if -E/--export-to-file is provided",
	"[CliArgsParser]")
{
	const std::string filename("filename");

	auto check = [&filename](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.readinfo_export_file() == filename);
	};

	SECTION("-E") {
		check({"newsboat", "-E", filename});
	}

	SECTION("--export-from-file") {
		check({"newsboat", "--export-to-file=" + filename});
	}
}

TEST_CASE("Resolves tilde to homedir in -E/--export-to-file", "[CliArgsParser]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());

	const std::string filename("read.txt");
	const std::string arg = std::string("~/") + filename;

	auto check = [&filename, &tmp](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(Filepath::from_locale_string(args.readinfo_export_file().value()) ==
			tmp.get_path().join(filename));
	};

	SECTION("-E") {
		check({"newsboat", "-E", arg});
	}

	SECTION("--export-to-file") {
		check({"newsboat", "--export-to-file", arg});
	}
}

TEST_CASE(
	"Asks to print usage info and exit with failure if both "
	"-I/--import-from-file and -E/--export-to-file are provided",
	"[CliArgsParser]")
{
	const std::string importf("import.opml");
	const std::string exportf("export.opml");

	auto check = [](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.should_print_usage());
		REQUIRE(args.return_code() == EXIT_FAILURE);
	};

	SECTION("-I first") {
		check({"newsboat", "-I", importf, "-E", exportf});
	}

	SECTION("-E first") {
		check({"newsboat", "-E", exportf, "-I", importf});
	}
}

TEST_CASE("Sets `log_file` if -d/--log-file is provided",
	"[CliArgsParser]")
{
	const std::string filename("log file.txt");

	auto check = [&filename](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.log_file() == filename);
	};

	SECTION("-d") {
		check({"newsboat", "-d", filename});
	}

	SECTION("--log-file") {
		check({"newsboat", "--log-file=" + filename});
	}
}

TEST_CASE("Resolves tilde to homedir in -d/--log-file", "[CliArgsParser]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());

	const std::string filename("newsboat.log");
	const std::string arg = std::string("~/") + filename;

	auto check = [&filename, &tmp](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(Filepath::from_locale_string(args.log_file().value()) == tmp.get_path().join(
				filename));
	};

	SECTION("-d") {
		check({"newsboat", "-d", arg});
	}

	SECTION("--log-file") {
		check({"newsboat", "--log-file", arg});
	}
}

TEST_CASE(
	"Sets `log_level` if argument to -l/--log-level is in range of [1; 6]",
	"[CliArgsParser]")
{
	auto check = [](test_helpers::Opts opts, Level expected) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.log_level() == expected);
	};

	SECTION("--log-level=1 means USERERROR") {
		check({"newsboat", "--log-level=1"}, Level::USERERROR);
	}

	SECTION("--log-level=2 means CRITICAL") {
		check({"newsboat", "--log-level=2"}, Level::CRITICAL);
	}

	SECTION("-l3 means ERROR") {
		check({"newsboat", "-l3"}, Level::ERROR);
	}

	SECTION("--log-level=4 means WARN") {
		check({"newsboat", "--log-level=4"}, Level::WARN);
	}

	SECTION("-l5 means INFO") {
		check({"newsboat", "-l5"}, Level::INFO);
	}

	SECTION("-l6 means DEBUG") {
		check({"newsboat", "-l6"}, Level::DEBUG);
	}
}

TEST_CASE(
	"Sets `display_msg` and asks to exit with failure if argument to "
	"-l/--log-level is outside of [1; 6]",
	"[CliArgsParser]")
{
	auto check = [](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE_FALSE(args.display_msg() == "");
		REQUIRE(args.return_code() == EXIT_FAILURE);
	};

	SECTION("-l0") {
		check({"newsboat", "-l0"});
	}

	SECTION("--log-level=7") {
		check({"newsboat", "--log-level=7"});
	}

	SECTION("--log-level=9001") {
		check({"newsboat", "--log-level=90001"});
	}
}

TEST_CASE("Sets `program_name` to the first string of the options list",
	"[CliArgsParser]")
{
	auto check = [](test_helpers::Opts opts, std::string expected) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.program_name() == expected);
	};

	check({"newsboat"}, "newsboat");
	check({"podboat", "-h"}, "podboat");
	check({"something else entirely", "--foo", "--bar", "--baz"},
		"something else entirely");
	check({"/usr/local/bin/app-with-a-path"},
		"/usr/local/bin/app-with-a-path");
}

TEST_CASE("Test should fail on equal sign with multiple values",
	"[CliArgsParser]")
{
	auto check = [](test_helpers::Opts opts, const std::vector<std::string>& cmds) {
		CliArgsParser args(opts.argc(), opts.argv());
		REQUIRE(args.should_print_usage());
		REQUIRE(args.cmds_to_execute() != cmds);
	};

	check({"newsboat", "-x=reload", "print-unread" }, { "reload", "print-unread" });
	check({"newsboat", "--execute=reload", "print-unread" }, { "reload", "print-unread" });

}

TEST_CASE("Supports combined short options where last has equal sign",
	"[CliArgsParser]")
{
	test_helpers::Opts opts = {"newsboat", "-rx=reload"};
	CliArgsParser args(opts.argc(), opts.argv());
	std::vector<std::string> commands{"reload"};
	REQUIRE(args.cmds_to_execute() == commands);
}

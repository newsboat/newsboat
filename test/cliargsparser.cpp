#include "3rd-party/catch.hpp"

#include "cliargsparser.h"
#include "test_helpers/envvar.h"
#include "test_helpers/opts.h"
#include "test_helpers/stringmaker/optional.h"
#include "test_helpers/tempdir.h"

using namespace Newsboat;

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
		check({"Newsboat", "--some-unknown-option"});
	}

	SECTION("Example No.2") {
		check({"Newsboat", "-s"});
	}

	SECTION("Example No.3") {
		check({"Newsboat", "-m ix"});
	}

	SECTION("Example No.4") {
		check({"Newsboat", "-wtf"});
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
		check({"Newsboat", "-i", filename});
	}

	SECTION("--import-from-opml") {
		check({"Newsboat", "--import-from-opml=" + filename});
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
		REQUIRE(args.importfile() == tmp.get_path() + filename);
	};

	SECTION("-i") {
		check({"Newsboat", "-i", arg});
	}

	SECTION("--import-from-opml") {
		check({"Newsboat", "--import-from-opml", arg});
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
		check({"Newsboat", "-i", importf, "-e", exportf});
	}

	SECTION("-e first") {
		check({"Newsboat", "-e", exportf, "-i", importf});
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
		check({"Newsboat", "-r"});
	}

	SECTION("--refresh-on-start") {
		check({"Newsboat", "--refresh-on-start"});
	}
}

TEST_CASE("Requests silent mode if -e/--export-to-opml is provided",
	"[CliArgsParser]")
{
	const test_helpers::Opts opts{"Newsboat", "-e"};

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
		check({"Newsboat", "-e"});
	}

	SECTION("--export-to-opml") {
		check({"Newsboat", "--export-to-opml"});
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
		check({"Newsboat", "-h"});
	}

	SECTION("--help") {
		check({"Newsboat", "--help"});
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
		check({"Newsboat", "-u", filename});
	}

	SECTION("--url-file") {
		check({"Newsboat", "--url-file=" + filename});
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

		REQUIRE(args.url_file() == tmp.get_path() + filename);
	};

	SECTION("-u") {
		check({"Newsboat", "-u", arg});
	}

	SECTION("--url-file") {
		check({"Newsboat", "--url-file", arg});
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
		check({"Newsboat", "-c", filename});
	}

	SECTION("--cache-file") {
		check({"Newsboat", "--cache-file=" + filename});
	}
}

TEST_CASE("Supports combined short options", "[CliArgsParser]")
{
	const std::string filename("cache.db");

	test_helpers::Opts opts = {"Newsboat", "-vc", filename};
	CliArgsParser args(opts.argc(), opts.argv());

	REQUIRE(args.cache_file() == filename);
	REQUIRE(args.lock_file() == filename + ".lock");
	REQUIRE(args.using_nonstandard_configs());
	REQUIRE(args.show_version() == 1);
}

TEST_CASE("Supports combined short option and value", "[CliArgsParser]")
{
	const std::string filename("cache.db");

	test_helpers::Opts opts = {"Newsboat", "-c" + filename};
	CliArgsParser args(opts.argc(), opts.argv());

	REQUIRE(args.cache_file() == filename);
	REQUIRE(args.lock_file() == filename + ".lock");
	REQUIRE(args.using_nonstandard_configs());
}

TEST_CASE("Supports `=` between short option and value",
	"[CliArgsParser]")
{
	const std::string filename("cache.db");

	test_helpers::Opts opts = {"Newsboat", "-c=" + filename};
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

		REQUIRE(args.cache_file() == tmp.get_path() + filename);
		REQUIRE(args.lock_file() == tmp.get_path() + filename + ".lock");
	};

	SECTION("-c") {
		check({"Newsboat", "-c", arg});
	}

	SECTION("--cache-file") {
		check({"Newsboat", "--cache-file", arg});
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
		check({"Newsboat", "-C", filename});
	}

	SECTION("--config-file") {
		check({"Newsboat", "--config-file=" + filename});
	}
}

TEST_CASE("Resolves tilde to homedir in -C/--config-file", "[CliArgsParser]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());

	const std::string filename("Newsboat-config");
	const std::string arg = std::string("~/") + filename;

	auto check = [&filename, &tmp](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.config_file() == tmp.get_path() + filename);
		REQUIRE(args.using_nonstandard_configs());
	};

	SECTION("-C") {
		check({"Newsboat", "-C", arg});
	}

	SECTION("--config-file") {
		check({"Newsboat", "--config-file", arg});
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
		check({"Newsboat", "--queue-file=" + filename});
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

		REQUIRE(args.queue_file() == tmp.get_path() + filename);
	};

	SECTION("--queue-file") {
		check({"Newsboat", "--queue-file", arg});
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
		check({"Newsboat", "--search-history-file=" + filename});
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

		REQUIRE(args.search_history_file() == tmp.get_path() + filename);
	};

	SECTION("--search-history-file") {
		check({"Newsboat", "--search-history-file", arg});
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
		check({"Newsboat", "--cmdline-history-file=" + filename});
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

		REQUIRE(args.cmdline_history_file() == tmp.get_path() + filename);
	};

	SECTION("--cmdline-history-file") {
		check({"Newsboat", "--cmdline-history-file", arg});
	}
}

TEST_CASE("Sets `do_vacuum` if -X/--vacuum is provided", "[CliArgsParser]")
{
	auto check = [](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.do_vacuum());
	};

	SECTION("-X") {
		check({"Newsboat", "-X"});
	}

	SECTION("--vacuum") {
		check({"Newsboat", "--vacuum"});
	}
}

TEST_CASE("Sets `do_cleanup` if --cleanup is provided", "[CliArgsParser]")
{
	auto check = [](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.do_cleanup());
	};

	SECTION("--cleanup") {
		check({"Newsboat", "--cleanup"});
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
		check({"Newsboat", "-v"}, 1);
	}

	SECTION("-V => 1") {
		check({"Newsboat", "-V"}, 1);
	}

	SECTION("--version => 1") {
		check({"Newsboat", "--version"}, 1);
	}

	SECTION("-vV => 2") {
		check({"Newsboat", "-vV"}, 2);
	}

	SECTION("--version -v => 2") {
		check({"Newsboat", "--version", "-v"}, 2);
	}

	SECTION("-V --version -v => 3") {
		check({"Newsboat", "-V", "--version", "-v"}, 3);
	}

	SECTION("-VvVVvvvvV => 9") {
		check({"Newsboat", "-VvVVvvvvV"}, 9);
	}
}

TEST_CASE("Requests silent mode if -x/--execute is provided", "[CliArgsParser]")
{
	auto check = [](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.silent());
	};

	SECTION("-x") {
		check({"Newsboat", "-x", "reload"});
	}

	SECTION("--execute") {
		check({"Newsboat", "--execute", "reload"});
	}
}

TEST_CASE("Sets `execute_cmds` if -x/--execute is provided", "[CliArgsParser]")
{
	auto check = [](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE_FALSE(args.cmds_to_execute().empty());
	};

	SECTION("-x") {
		check({"Newsboat", "-x", "reload"});
	}

	SECTION("--execute") {
		check({"Newsboat", "--execute", "reload"});
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
		check({"Newsboat", "-x", "reload"}, {"reload"});
	}

	SECTION("--execute reload") {
		check({"Newsboat", "--execute", "reload"}, {"reload"});
	}

	SECTION("-x reload print-unread") {
		check({"Newsboat", "-x", "reload", "print-unread"},
		{"reload", "print-unread"});
	}

	SECTION("--execute reload print-unread") {
		check({"Newsboat", "--execute", "reload", "print-unread"},
		{"reload", "print-unread"});
	}

	SECTION("Multiple occurrences of the option") {
		check({"Newsboat", "-x", "print-unread", "--execute", "reload"},
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
		check({"Newsboat", "-q"});
	}

	SECTION("--quiet") {
		check({"Newsboat", "--quiet"});
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
		check({"Newsboat", "-I", filename});
	}

	SECTION("--import-from-file") {
		check({"Newsboat", "--import-from-file=" + filename});
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

		REQUIRE(args.readinfo_import_file() == tmp.get_path() + filename);
	};

	SECTION("-I") {
		check({"Newsboat", "-I", arg});
	}

	SECTION("--import-from-file") {
		check({"Newsboat", "--import-from-file", arg});
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
		check({"Newsboat", "-E", filename});
	}

	SECTION("--export-from-file") {
		check({"Newsboat", "--export-to-file=" + filename});
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

		REQUIRE(args.readinfo_export_file() == tmp.get_path() + filename);
	};

	SECTION("-E") {
		check({"Newsboat", "-E", arg});
	}

	SECTION("--export-to-file") {
		check({"Newsboat", "--export-to-file", arg});
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
		check({"Newsboat", "-I", importf, "-E", exportf});
	}

	SECTION("-E first") {
		check({"Newsboat", "-E", exportf, "-I", importf});
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
		check({"Newsboat", "-d", filename});
	}

	SECTION("--log-file") {
		check({"Newsboat", "--log-file=" + filename});
	}
}

TEST_CASE("Resolves tilde to homedir in -d/--log-file", "[CliArgsParser]")
{
	test_helpers::TempDir tmp;

	test_helpers::EnvVar home("HOME");
	home.set(tmp.get_path());

	const std::string filename("Newsboat.log");
	const std::string arg = std::string("~/") + filename;

	auto check = [&filename, &tmp](test_helpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.log_file() == tmp.get_path() + filename);
	};

	SECTION("-d") {
		check({"Newsboat", "-d", arg});
	}

	SECTION("--log-file") {
		check({"Newsboat", "--log-file", arg});
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
		check({"Newsboat", "--log-level=1"}, Level::USERERROR);
	}

	SECTION("--log-level=2 means CRITICAL") {
		check({"Newsboat", "--log-level=2"}, Level::CRITICAL);
	}

	SECTION("-l3 means ERROR") {
		check({"Newsboat", "-l3"}, Level::ERROR);
	}

	SECTION("--log-level=4 means WARN") {
		check({"Newsboat", "--log-level=4"}, Level::WARN);
	}

	SECTION("-l5 means INFO") {
		check({"Newsboat", "-l5"}, Level::INFO);
	}

	SECTION("-l6 means DEBUG") {
		check({"Newsboat", "-l6"}, Level::DEBUG);
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
		check({"Newsboat", "-l0"});
	}

	SECTION("--log-level=7") {
		check({"Newsboat", "--log-level=7"});
	}

	SECTION("--log-level=9001") {
		check({"Newsboat", "--log-level=90001"});
	}
}

TEST_CASE("Sets `program_name` to the first string of the options list",
	"[CliArgsParser]")
{
	auto check = [](test_helpers::Opts opts, std::string expected) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.program_name() == expected);
	};

	check({"Newsboat"}, "Newsboat");
	check({"Podboat", "-h"}, "Podboat");
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

	check({"Newsboat", "-x=reload", "print-unread" }, { "reload", "print-unread" });
	check({"Newsboat", "--execute=reload", "print-unread" }, { "reload", "print-unread" });

}

TEST_CASE("Supports combined short options where last has equal sign",
	"[CliArgsParser]")
{
	test_helpers::Opts opts = {"Newsboat", "-rx=reload"};
	CliArgsParser args(opts.argc(), opts.argv());
	std::vector<std::string> commands{"reload"};
	REQUIRE(args.cmds_to_execute() == commands);
}

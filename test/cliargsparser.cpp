#include "3rd-party/catch.hpp"

#include <cstring>

#include "cliargsparser.h"
#include "test-helpers.h"

using namespace newsboat;

TEST_CASE(
	"Asks to print usage info and exit with failure if unknown option is "
	"provided",
	"[CliArgsParser]")
{
	auto check = [](TestHelpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.should_print_usage());
		REQUIRE(args.should_return());
		REQUIRE(args.return_code() == EXIT_FAILURE);
	};

	SECTION("Example No.1")
	{
		check({"newsboat", "--some-unknown-option"});
	}

	SECTION("Example No.2")
	{
		check({"newsboat", "-s"});
	}

	SECTION("Example No.3")
	{
		check({"newsboat", "-m ix"});
	}

	SECTION("Example No.4")
	{
		check({"newsboat", "-wtf"});
	}
}

TEST_CASE(
	"Sets `do_import` and `importfile` if -i/--import-from-opml is "
	"provided",
	"[CliArgsParser]")
{
	const std::string filename("blogroll.opml");

	auto check = [&filename](TestHelpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.do_import());
		REQUIRE(args.importfile() == filename);
	};

	SECTION("-i")
	{
		check({"newsboat", "-i", filename});
	}

	SECTION("--import-from-opml")
	{
		check({"newsboat", "--import-from-opml=" + filename});
	}
}

TEST_CASE(
	"Asks to print usage and exit with failure if both "
	"-i/--import-from-opml and -e/--export-to-opml are provided",
	"[CliArgsParser]")
{
	const std::string importf("import.opml");
	const std::string exportf("export.opml");

	auto check = [](TestHelpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.should_print_usage());
		REQUIRE(args.should_return());
		REQUIRE(args.return_code() == EXIT_FAILURE);
	};

	SECTION("-i first")
	{
		check({"newsboat", "-i", importf, "-e", exportf});
	}

	SECTION("-e first")
	{
		check({"newsboat", "-e", exportf, "-i", importf});
	}
}

TEST_CASE("Sets `refresh_on_start` if -r/--refresh-on-start is provided",
	"[CliArgsParser]")
{
	auto check = [](TestHelpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.refresh_on_start());
	};

	SECTION("-r")
	{
		check({"newsboat", "-r"});
	}

	SECTION("--refresh-on-start")
	{
		check({"newsboat", "--refresh-on-start"});
	}
}

TEST_CASE("Requests silent mode if -e/--export-to-opml is provided",
	"[CliArgsParser]")
{
	const TestHelpers::Opts opts{"newsboat", "-e"};

	CliArgsParser args(opts.argc(), opts.argv());

	REQUIRE(args.silent());
}

TEST_CASE("Sets `do_export` if -e/--export-to-opml is provided",
	"[CliArgsParser]")
{
	auto check = [](TestHelpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.do_export());
	};

	SECTION("-e")
	{
		check({"newsboat", "-e"});
	}

	SECTION("--export-to-opml")
	{
		check({"newsboat", "--export-to-opml"});
	}
}

TEST_CASE("Asks to print usage and exit with success if -h/--help is provided",
	"[CliArgsParser]")
{
	auto check = [](TestHelpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.should_print_usage());
		REQUIRE(args.should_return());
		REQUIRE(args.return_code() == EXIT_SUCCESS);
	};

	SECTION("-h")
	{
		check({"newsboat", "-h"});
	}

	SECTION("--help")
	{
		check({"newsboat", "--help"});
	}
}

TEST_CASE(
	"Sets `url_file`, `set_url_file`, and "
	"`using_nonstandard_configs` if -u/--url-file is provided",
	"[CliArgsParser]")
{
	const std::string filename("urlfile");

	auto check = [&filename](TestHelpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.set_url_file());
		REQUIRE(args.url_file() == filename);
		REQUIRE(args.using_nonstandard_configs());
	};

	SECTION("-u")
	{
		check({"newsboat", "-u", filename});
	}

	SECTION("--url-file")
	{
		check({"newsboat", "--url-file=" + filename});
	}
}

TEST_CASE(
	"Sets `cache_file`, `lock_file`, `set_cache_file`, `set_lock_file`, "
	"and "
	"`using_nonstandard_configs` if -c/--cache-file is provided",
	"[CliArgsParser]")
{
	const std::string filename("cache.db");

	auto check = [&filename](TestHelpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.set_cache_file());
		REQUIRE(args.cache_file() == filename);
		REQUIRE(args.set_lock_file());
		REQUIRE(args.lock_file() == filename + ".lock");
		REQUIRE(args.using_nonstandard_configs());
	};

	SECTION("-c")
	{
		check({"newsboat", "-c", filename});
	}

	SECTION("--cache-file")
	{
		check({"newsboat", "--cache-file=" + filename});
	}
}

TEST_CASE(
	"Sets `config_file`, `set_config_file`, and "
	"`using_nonstandard_configs` if -C/--config-file is provided",
	"[CliArgsParser]")
{
	const std::string filename("config file");

	auto check = [&filename](TestHelpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.set_config_file());
		REQUIRE(args.config_file() == filename);
		REQUIRE(args.using_nonstandard_configs());
	};

	SECTION("-C")
	{
		check({"newsboat", "-C", filename});
	}

	SECTION("--config-file")
	{
		check({"newsboat", "--config-file=" + filename});
	}
}

TEST_CASE("Sets `do_vacuum` if -X/--vacuum is provided", "[CliArgsParser]")
{
	auto check = [](TestHelpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.do_vacuum());
	};

	SECTION("-X")
	{
		check({"newsboat", "-X"});
	}

	SECTION("--vacuum")
	{
		check({"newsboat", "--vacuum"});
	}
}

TEST_CASE("Increases `show_version` with each -v/-V/--version provided",
	"[CliArgsParser]")
{
	auto check = [](TestHelpers::Opts opts, int expected_version) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.show_version() == expected_version);
	};

	SECTION("-v => 1")
	{
		check({"newsboat", "-v"}, 1);
	}

	SECTION("-V => 1")
	{
		check({"newsboat", "-V"}, 1);
	}

	SECTION("--version => 1")
	{
		check({"newsboat", "--version"}, 1);
	}

	SECTION("-vV => 2")
	{
		check({"newsboat", "-vV"}, 2);
	}

	SECTION("--version -v => 2")
	{
		check({"newsboat", "--version", "-v"}, 2);
	}

	SECTION("-V --version -v => 3")
	{
		check({"newsboat", "-V", "--version", "-v"}, 3);
	}

	SECTION("-VvVVvvvvV => 9")
	{
		check({"newsboat", "-VvVVvvvvV"}, 9);
	}
}

TEST_CASE("Requests silent mode if -x/--execute is provided", "[CliArgsParser]")
{
	auto check = [](TestHelpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.silent());
	};

	SECTION("-x")
	{
		check({"newsboat", "-x", "reload"});
	}

	SECTION("--execute")
	{
		check({"newsboat", "--execute", "reload"});
	}
}

TEST_CASE("Sets `execute_cmds` if -x/--execute is provided", "[CliArgsParser]")
{
	auto check = [](TestHelpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.execute_cmds());
	};

	SECTION("-x")
	{
		check({"newsboat", "-x", "reload"});
	}

	SECTION("--execute")
	{
		check({"newsboat", "--execute", "reload"});
	}
}

TEST_CASE("Inserts commands to cmds_to_execute if -x/--execute is provided",
	"[CliArgsParser]")
{
	auto check = [](TestHelpers::Opts opts,
			     const std::vector<std::string>& cmds) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.cmds_to_execute() == cmds);
	};

	SECTION("-x reload")
	{
		check({"newsboat", "-x", "reload"}, {"reload"});
	}

	SECTION("--execute reload")
	{
		check({"newsboat", "--execute", "reload"}, {"reload"});
	}

	SECTION("-x reload print-unread")
	{
		check({"newsboat", "-x", "reload", "print-unread"},
			{"reload", "print-unread"});
	}

	SECTION("--execute reload print-unread")
	{
		check({"newsboat", "--execute", "reload", "print-unread"},
			{"reload", "print-unread"});
	}

	SECTION("Multiple occurrences of the option")
	{
		check({"newsboat", "-x", "print-unread", "--execute", "reload"},
			{"print-unread", "reload"});
	}
}

TEST_CASE("Requests silent mode if -q/--quiet is provided", "[CliArgsParser]")
{
	auto check = [](TestHelpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.silent());
	};

	SECTION("-q")
	{
		check({"newsboat", "-q"});
	}

	SECTION("--quiet")
	{
		check({"newsboat", "--quiet"});
	}
}

TEST_CASE(
	"Sets `do_read_import` and `readinfofile` if -I/--import-from-file "
	"is provided",
	"[CliArgsParser]")
{
	const std::string filename("filename");

	auto check = [&filename](TestHelpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.do_read_import());
		REQUIRE(args.readinfo_import_file() == filename);
	};

	SECTION("-I")
	{
		check({"newsboat", "-I", filename});
	}

	SECTION("--import-from-file")
	{
		check({"newsboat", "--import-from-file=" + filename});
	}
}

TEST_CASE(
	"Sets `do_read_export` and `readinfofile` if -E/--export-to-file "
	"is provided",
	"[CliArgsParser]")
{
	const std::string filename("filename");

	auto check = [&filename](TestHelpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.do_read_export());
		REQUIRE(args.readinfo_export_file() == filename);
	};

	SECTION("-E")
	{
		check({"newsboat", "-E", filename});
	}

	SECTION("--export-from-file")
	{
		check({"newsboat", "--export-to-file=" + filename});
	}
}

TEST_CASE(
	"Asks to print usage info and exit with failure if both "
	"-I/--import-from-file and -E/--export-to-file are provided",
	"[CliArgsParser]")
{
	const std::string importf("import.opml");
	const std::string exportf("export.opml");

	auto check = [](TestHelpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.should_print_usage());
		REQUIRE(args.should_return());
		REQUIRE(args.return_code() == EXIT_FAILURE);
	};

	SECTION("-I first")
	{
		check({"newsboat", "-I", importf, "-E", exportf});
	}

	SECTION("-E first")
	{
		check({"newsboat", "-E", exportf, "-I", importf});
	}
}

TEST_CASE("Sets `set_log_file` and `log_file` if -d/--log-file is provided",
	"[CliArgsParser]")
{
	const std::string filename("log file.txt");

	auto check = [&filename](TestHelpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.set_log_file());
		REQUIRE(args.log_file() == filename);
	};

	SECTION("-d")
	{
		check({"newsboat", "-d", filename});
	}

	SECTION("--log-file")
	{
		check({"newsboat", "--log-file=" + filename});
	}
}

TEST_CASE(
	"Sets `set_log_level` and `log_level` if argument to "
	"-l/--log-level is in range of [1; 6]",
	"[CliArgsParser]")
{
	auto check = [](TestHelpers::Opts opts, Level expected) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.set_log_level());
		REQUIRE(args.log_level() == expected);
	};

	SECTION("--log-level=1 means USERERROR")
	{
		check({"newsboat", "--log-level=1"}, Level::USERERROR);
	}

	SECTION("--log-level=2 means CRITICAL")
	{
		check({"newsboat", "--log-level=2"}, Level::CRITICAL);
	}

	SECTION("-l3 means ERROR")
	{
		check({"newsboat", "-l3"}, Level::ERROR);
	}

	SECTION("--log-level=4 means WARN")
	{
		check({"newsboat", "--log-level=4"}, Level::WARN);
	}

	SECTION("-l5 means INFO")
	{
		check({"newsboat", "-l5"}, Level::INFO);
	}

	SECTION("-l6 means DEBUG")
	{
		check({"newsboat", "-l6"}, Level::DEBUG);
	}
}

TEST_CASE(
	"Sets `display_msg` and asks to exit with failure if argument to "
	"-l/--log-level is outside of [1; 6]",
	"[CliArgsParser]")
{
	auto check = [](TestHelpers::Opts opts) {
		CliArgsParser args(opts.argc(), opts.argv());

		REQUIRE_FALSE(args.display_msg().empty());
		REQUIRE(args.should_return());
		REQUIRE(args.return_code() == EXIT_FAILURE);
	};

	SECTION("-l0")
	{
		check({"newsboat", "-l0"});
	}

	SECTION("--log-level=7")
	{
		check({"newsboat", "--log-level=7"});
	}

	SECTION("--log-level=9001")
	{
		check({"newsboat", "--log-level=90001"});
	}
}

TEST_CASE("Sets `program_name` to the first string of the options list",
	"[CliArgsParser]")
{
	auto check = [](TestHelpers::Opts opts, std::string expected) {
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

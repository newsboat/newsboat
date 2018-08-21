#include "3rd-party/catch.hpp"

#include <cstring>

#include "cliargsparser.h"

using namespace newsboat;

/// Helper class to create argc and argv arguments for CLIArgsParser
///
/// When testing CLIArgsParser, resource management turned out to be a problem:
/// CLIArgsParser requires char** pointing to arguments, but such a pointer
/// can't be easily obtained from any of standard containers. To overcome that,
/// I wrote Opts, which simply copies elements of initializer_list into
/// separate unique_ptr<char>, and presents useful accessors argc() and argv()
/// whose results can be passed right into CLIArgsParser. Problem solved!
class Opts {
	/// Individual elements of argv.
	std::vector<std::unique_ptr<char[]>> m_opts;
	/// This is argv as main() knows it.
	std::unique_ptr<char* []> m_data;
	/// This is argc as main() knows it.
	std::size_t m_argc = 0;

public:
	/// Turns \a opts into argc and argv.
	Opts(std::initializer_list<std::string> opts)
		: m_argc(opts.size())
	{
		m_opts.reserve(m_argc);

		for (const std::string& option : opts) {
			// Copy string into separate char[], managed by
			// unique_ptr.
			auto ptr = std::unique_ptr<char[]>(
				new char[option.size() + 1]);
			std::copy(option.cbegin(), option.cend(), ptr.get());
			// C and C++ require argv to be NULL-terminated:
			// https://stackoverflow.com/questions/18547114/why-do-we-need-argc-while-there-is-always-a-null-at-the-end-of-argv
			ptr.get()[option.size()] = '\0';

			// Hold onto the smart pointer to keep the entry in argv
			// alive.
			m_opts.emplace_back(std::move(ptr));
		}

		// Copy out intermediate argv vector into its final storage.
		m_data = std::unique_ptr<char* []>(new char*[m_argc + 1]);
		int i = 0;
		for (const auto& ptr : m_opts) {
			m_data.get()[i++] = ptr.get();
		}
		m_data.get()[i] = nullptr;
	}

	std::size_t argc() const
	{
		return m_argc;
	}

	char** argv() const
	{
		return m_data.get();
	}
};

TEST_CASE(
	"Asks to print usage info and exit with failure if unknown option is "
	"provided",
	"[cliargsparser]")
{
	auto check = [](Opts opts) {
		CLIArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.should_print_usage);
		REQUIRE(args.should_return);
		REQUIRE(args.return_code == EXIT_FAILURE);
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
	"[cliargsparser]")
{
	const std::string filename("blogroll.opml");

	auto check = [&filename](Opts opts) {
		CLIArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.do_import);
		REQUIRE(args.importfile == filename);
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
	"[cliargsparser]")
{
	const std::string importf("import.opml");
	const std::string exportf("export.opml");

	auto check = [](Opts opts) {
		CLIArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.should_print_usage);
		REQUIRE(args.should_return);
		REQUIRE(args.return_code == EXIT_FAILURE);
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
	"[cliargsparser]")
{
	auto check = [](Opts opts) {
		CLIArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.refresh_on_start);
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
	"[cliargsparser]")
{
	const Opts opts{"newsboat", "-e"};

	CLIArgsParser args(opts.argc(), opts.argv());

	REQUIRE(args.silent);
}

TEST_CASE("Sets `do_export` if -e/--export-to-opml is provided",
	"[cliargsparser]")
{
	auto check = [](Opts opts) {
		CLIArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.do_export);
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
	"[cliargsparser]")
{
	auto check = [](Opts opts) {
		CLIArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.should_print_usage);
		REQUIRE(args.should_return);
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
	"[cliargsparser]")
{
	const std::string filename("urlfile");

	auto check = [&filename](Opts opts) {
		CLIArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.set_url_file);
		REQUIRE(args.url_file == filename);
		REQUIRE(args.using_nonstandard_configs);
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
	"[cliargsparser]")
{
	const std::string filename("cache.db");

	auto check = [&filename](Opts opts) {
		CLIArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.set_cache_file);
		REQUIRE(args.cache_file == filename);
		REQUIRE(args.set_lock_file);
		REQUIRE_THAT(
			args.lock_file, Catch::Matchers::StartsWith(filename));
		REQUIRE(args.using_nonstandard_configs);
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
	"[cliargsparser]")
{
	const std::string filename("config file");

	auto check = [&filename](Opts opts) {
		CLIArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.set_config_file);
		REQUIRE(args.config_file == filename);
		REQUIRE(args.using_nonstandard_configs);
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

TEST_CASE("Sets `do_vacuum` if -X/--vacuum is provided", "[cliargsparser]")
{
	auto check = [](Opts opts) {
		CLIArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.do_vacuum);
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
	"[cliargsparser]")
{
	auto check = [](Opts opts, int expected_version) {
		CLIArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.show_version == expected_version);
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

TEST_CASE("Requests silent mode if -x/--execute is provided", "[cliargsparser]")
{
	auto check = [](Opts opts) {
		CLIArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.silent);
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

TEST_CASE("Sets `execute_cmds` if -x/--execute is provided", "[cliargsparser]")
{
	auto check = [](Opts opts) {
		CLIArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.execute_cmds);
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
	"[cliargsparser]")
{
	auto check = [](Opts opts, const std::vector<std::string>& cmds) {
		CLIArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.cmds_to_execute == cmds);
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
}

TEST_CASE("Requests silent mode if -q/--quiet is provided", "[cliargsparser]")
{
	auto check = [](Opts opts) {
		CLIArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.silent);
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
	"[cliargsparser]")
{
	const std::string filename("filename");

	auto check = [&filename](Opts opts) {
		CLIArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.do_read_import);
		REQUIRE(args.readinfofile == filename);
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
	"[cliargsparser]")
{
	const std::string filename("filename");

	auto check = [&filename](Opts opts) {
		CLIArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.do_read_export);
		REQUIRE(args.readinfofile == filename);
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
	"[cliargsparser]")
{
	const std::string importf("import.opml");
	const std::string exportf("export.opml");

	auto check = [](Opts opts) {
		CLIArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.should_print_usage);
		REQUIRE(args.should_return);
		REQUIRE(args.return_code == EXIT_FAILURE);
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
	"[cliargsparser]")
{
	const std::string filename("log file.txt");

	auto check = [&filename](Opts opts) {
		CLIArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.set_log_file);
		REQUIRE(args.log_file == filename);
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
	"[cliargsparser]")
{
	auto check = [](Opts opts, level expected) {
		CLIArgsParser args(opts.argc(), opts.argv());

		REQUIRE(args.set_log_level);
		REQUIRE(args.log_level == expected);
	};

	SECTION("--log-level=1 means USERERROR")
	{
		check({"newsboat", "--log-level=1"}, level::USERERROR);
	}

	SECTION("--log-level=2 means CRITICAL")
	{
		check({"newsboat", "--log-level=2"}, level::CRITICAL);
	}

	SECTION("-l3 means ERROR")
	{
		check({"newsboat", "-l3"}, level::ERROR);
	}

	SECTION("--log-level=4 means WARN")
	{
		check({"newsboat", "--log-level=4"}, level::WARN);
	}

	SECTION("-l5 means INFO")
	{
		check({"newsboat", "-l5"}, level::INFO);
	}

	SECTION("-l6 means DEBUG")
	{
		check({"newsboat", "-l6"}, level::DEBUG);
	}
}

TEST_CASE(
	"Sets `display_msg` and asks to exit with failure if argument to "
	"-l/--log-level is outside of [1; 6]",
	"[cliargsparser]")
{
	auto check = [](Opts opts) {
		CLIArgsParser args(opts.argc(), opts.argv());

		REQUIRE_FALSE(args.display_msg.empty());
		REQUIRE(args.should_return);
		REQUIRE(args.return_code == EXIT_FAILURE);
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

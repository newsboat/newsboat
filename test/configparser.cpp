#include "configparser.h"

#include <fstream>
#include <utility>
#include <vector>

#include "3rd-party/catch.hpp"

#include "configexception.h"
#include "keycombination.h"
#include "keymap.h"
#include "test_helpers/envvar.h"
#include "test_helpers/tempfile.h"

using namespace Newsboat;

namespace {

class ConfigHandlerHistoryDummy : public ConfigActionHandler {
public:
	void handle_action(const std::string& action,
		const std::vector<std::string>& params) override
	{
		history.emplace_back(action, params);
	};

	void dump_config(std::vector<std::string>&) const override {};

	std::vector<std::pair<std::string, std::vector<std::string>>> history;
};

} // Anonymous namespace

TEST_CASE("parse_line() handles both lines with and without quoting",
	"[ConfigParser]")
{
	ConfigParser cfgParser;

	ConfigHandlerHistoryDummy handler;
	cfgParser.register_handler("command-name", handler);

	const std::string location = "dummy-location";

	SECTION("unknown command results in exception") {
		REQUIRE_THROWS_AS(cfgParser.parse_line("foo", location), ConfigException);
		REQUIRE_THROWS_AS(cfgParser.parse_line("foo arg1 arg2", location),
			ConfigException);
	}

	SECTION("different combinations of (un)quoted commands/arguments have the same result") {
		const std::vector<std::string> inputs = {
			R"(command-name arg1 arg2)",
			R"("command-name" "arg1" "arg2")",
			R"(command-name "arg1" arg2)",
			R"("command-name" arg1 arg2)",
			R"("command-name""arg1""arg2")", // whitespace can be omitted between quoted arguments
		};

		for (std::string input : inputs) {
			DYNAMIC_SECTION("input: " << input) {
				cfgParser.parse_line(input, location);

				REQUIRE(handler.history.size() == 1);
				REQUIRE(handler.history[0].first == "command-name");
				REQUIRE(handler.history[0].second == std::vector<std::string>({"arg1", "arg2"}));
			}
		}
	}
}

TEST_CASE("parse_line() does not care about whitespace at start or end of line",
	"[ConfigParser]")
{
	ConfigParser cfgParser;

	ConfigHandlerHistoryDummy handler;
	cfgParser.register_handler("command-name", handler);

	const std::string location = "dummy-location";

	SECTION("no whitespace") {
		cfgParser.parse_line("command-name arg1", location);

		REQUIRE(handler.history.size() == 1);
		REQUIRE(handler.history[0].first == "command-name");
		REQUIRE(handler.history[0].second == std::vector<std::string>({"arg1"}));
	}

	SECTION("some whitespace") {
		const std::vector<std::string> inputs = {
			"\r\n\t command-name arg1",
			"command-name arg1\r\n\t ",
			"\r\n\t command-name\t\targ1\r\n\t ",
		};

		for (std::string input : inputs) {
			DYNAMIC_SECTION("input: " << input) {
				cfgParser.parse_line(input, location);

				REQUIRE(handler.history.size() == 1);
				REQUIRE(handler.history[0].first == "command-name");
				REQUIRE(handler.history[0].second == std::vector<std::string>({"arg1"}));
			}
		}
	}
}

TEST_CASE("parse_line() processes backslash escapes in quoted commands and arguments",
	"[ConfigParser]")
{
	ConfigParser cfgParser;

	ConfigHandlerHistoryDummy handler;
	cfgParser.register_handler("command", handler);

	const std::string location = "dummy-location";

	auto check_output = [&](const std::string& input,
			const std::string& expected_command,
	const std::vector<std::string>& expected_arguments) {
		cfgParser.parse_line(input, location);
		REQUIRE(handler.history.size() >= 1);
		REQUIRE(handler.history.back().first == expected_command);
		REQUIRE(handler.history.back().second == expected_arguments);
	};

	SECTION("escapes are handled when inside quoted string") {
		check_output(R"(command "arg")", "command", {"arg"});
		check_output(R"(command "arg\n")", "command", {"arg\n"});
		check_output(R"(command "a\"r\"g")", "command", {R"(a"r"g)"});
	}

	SECTION("no escape handling outside of quoted parts of string") {
		check_output(R"(command arg)", "command", {"arg"});
		check_output(R"(command arg\n)", "command", {R"(arg\n)"});
		check_output(R"(command \"arg)", "command", {R"(\"arg)"});
	}
}

TEST_CASE("evaluate_backticks replaces command in backticks with its output",
	"[ConfigParser]")
{
	SECTION("substitutes command output") {
		REQUIRE(ConfigParser::evaluate_backticks("") == "");
		REQUIRE(ConfigParser::evaluate_backticks("hello world") ==
			"hello world");
		// backtick evaluation with true (empty string)
		REQUIRE(ConfigParser::evaluate_backticks("foo`true`baz") ==
			"foobaz");
		// backtick evaluation with true (2)
		REQUIRE(ConfigParser::evaluate_backticks("foo `true` baz") ==
			"foo  baz");
		REQUIRE(ConfigParser::evaluate_backticks("foo`barbaz") ==
			"foo`barbaz");
		REQUIRE(ConfigParser::evaluate_backticks(
				"foo `true` baz `xxx") == "foo  baz `xxx");
		REQUIRE(ConfigParser::evaluate_backticks(
				"`echo hello world`") == "hello world");
		REQUIRE(ConfigParser::evaluate_backticks("xxx`echo yyy`zzz") ==
			"xxxyyyzzz");
		REQUIRE(ConfigParser::evaluate_backticks(
				"`seq 10 | tail -1`") == "10");
	}

	SECTION("subsistutes multiple shellouts") {
		REQUIRE(ConfigParser::evaluate_backticks("xxx`echo aaa`yyy`echo bbb`zzz") ==
			"xxxaaayyybbbzzz");
	}

	SECTION("backticks can be escaped with backslash") {
		REQUIRE(ConfigParser::evaluate_backticks(
				"hehe \\`two at a time\\`haha") ==
			"hehe `two at a time`haha");
	}

	SECTION("single backticks have to be escaped too") {
		REQUIRE(ConfigParser::evaluate_backticks(
				"a single literal backtick: \\`") ==
			"a single literal backtick: `");
	}
	SECTION("commands with space are evaluated by backticks") {
		ConfigParser cfgparser;
		KeyMap keys(KM_NEWSBOAT);
		cfgparser.register_handler("bind-key", keys);
		REQUIRE_NOTHROW(cfgparser.parse_file("data/config-space-backticks"));
		MultiKeyBindingState binding_state{};
		BindingType binding_type{};
		const auto& cmds = keys.get_operation({KeyCombination("s")}, "feedlist", binding_state,
				binding_type);
		REQUIRE(cmds.size() == 1);
		REQUIRE(cmds.at(0).op == OP_SORT);
	}

	SECTION("Unbalanced backtick does *not* start a command") {
		const auto input1 = std::string("one `echo two three");
		REQUIRE(ConfigParser::evaluate_backticks(input1) == input1);

		const auto input2 = std::string("this `echo is a` test `here");
		const auto expected2 = std::string("this is a test `here");
		REQUIRE(ConfigParser::evaluate_backticks(input2) == expected2);
	}

	// One might think that putting one or both backticks inside a string will
	// "escape" them, the same way as backslash does. But it doesn't, and
	// shouldn't: when parsing a config, we need to evaluate *all* commands
	// there are, no matter where they're placed.
	SECTION("Backticks inside double quotes are not ignored") {
		const auto input1 = std::string(R"#("`echo hello`")#");
		REQUIRE(ConfigParser::evaluate_backticks(input1) == R"#("hello")#");

		const auto input2 = std::string(R"#(a "b `echo c" d e` f)#");
		// The line above asks the shell to run 'echo c" d e', which is an
		// invalid command--the double quotes are not closed. The standard
		// output of that command would be empty, so nothing will be inserted
		// in place of backticks.
		const auto expected2 = std::string(R"#(a "b  f)#");
		REQUIRE(ConfigParser::evaluate_backticks(input2) == expected2);
	}
}

TEST_CASE("\"unbind-key -a\" removes all key bindings", "[ConfigParser]")
{
	ConfigParser cfgparser;

	SECTION("In all contexts by default") {
		KeyMap keys(KM_NEWSBOAT);
		cfgparser.register_handler("unbind-key", keys);
		cfgparser.parse_file("data/config-unbind-all");

		for (int i = OP_QUIT; i < OP_NB_MAX; ++i) {
			REQUIRE(keys.get_keys(static_cast<Operation>(i),
					"feedlist") == std::vector<KeyCombination>());
			REQUIRE(keys.get_keys(static_cast<Operation>(i),
					"Podboat") == std::vector<KeyCombination>());
		}
	}

	SECTION("For a specific context") {
		KeyMap keys(KM_NEWSBOAT);
		cfgparser.register_handler("unbind-key", keys);
		cfgparser.parse_file("data/config-unbind-all-context");

		INFO("it doesn't affect the help dialog");
		KeyMap default_keys(KM_NEWSBOAT);
		for (int i = OP_QUIT; i < OP_NB_MAX; ++i) {
			const auto op = static_cast<Operation>(i);
			REQUIRE(keys.get_keys(op, "help") == default_keys.get_keys(op, "help"));
		}

		for (int i = OP_QUIT; i < OP_NB_MAX; ++i) {
			REQUIRE(keys.get_keys(static_cast<Operation>(i),
					"article") == std::vector<KeyCombination>());
		}
	}
}

TEST_CASE("Concatenates lines that end with a backslash", "[ConfigParser]")
{
	ConfigParser cfgparser;
	KeyMap k(KM_NEWSBOAT);
	cfgparser.register_handler("macro", k);
	REQUIRE_NOTHROW(cfgparser.parse_file("data/config-multi-line"));
	auto p_macro = k.get_macro(KeyCombination("p"));
	REQUIRE(!p_macro.empty());
	REQUIRE(p_macro[0].op == Newsboat::OP_OPEN);
	REQUIRE(p_macro[1].op == Newsboat::OP_RELOAD);
	REQUIRE(p_macro[2].op == Newsboat::OP_QUIT);
	REQUIRE(p_macro[3].op == Newsboat::OP_QUIT);

	auto contrived = k.get_macro(KeyCombination("j"));
	REQUIRE(!contrived.empty());
	REQUIRE(contrived[0].op == Newsboat::OP_QUIT);
	REQUIRE(contrived[1].op == Newsboat::OP_QUIT);
	REQUIRE(contrived[2].op == Newsboat::OP_QUIT);
	REQUIRE(contrived[3].op == Newsboat::OP_QUIT);
}

TEST_CASE("`include` directive includes other config files", "[ConfigParser]")
{
	// TODO: error messages should be more descriptive than "file couldn't be opened"
	ConfigParser cfgparser;
	SECTION("Errors if file is not found") {
		REQUIRE_THROWS_AS(cfgparser.parse_file("data/config-missing-include"),
			ConfigException);
	}
	SECTION("Errors on invalid UTF-8 in file") {
		REQUIRE_THROWS_AS(cfgparser.parse_file("data/config-invalid-utf-8"),
			ConfigException);
	}
	SECTION("Terminates on recursive include") {
		REQUIRE_THROWS_AS(cfgparser.parse_file("data/config-recursive-include"),
			ConfigException);
	}
	SECTION("Successfully includes existing file") {
		REQUIRE_NOTHROW(cfgparser.parse_file("data/config-absolute-include"));
	}
	SECTION("Success on relative includes") {
		REQUIRE_NOTHROW(cfgparser.parse_file("data/config-relative-include"));
	}
	SECTION("Diamond of death includes pass") {
		REQUIRE_NOTHROW(cfgparser.parse_file("data/diamond-of-death/A"));
	}
	SECTION("File including itself only gets evaluated once") {
		test_helpers::TempFile testfile;
		test_helpers::EnvVar tmpfile("TMPFILE"); // $TMPFILE used in conf file
		tmpfile.set(testfile.get_path());

		// recursive includes don't fail
		REQUIRE_NOTHROW(
			cfgparser.parse_file("data/recursive-include-side-effect"));
		// I think it will never get below here and fail? If it recurses, the above fails

		int line_count = 0;
		{
			// from https://stackoverflow.com/a/19140230
			std::ifstream in(testfile.get_path());
			std::string line;
			while (std::getline(in, line)) {
				line_count++;
			}
		}
		REQUIRE(line_count == 1); // only 1 line from date command
	}
}

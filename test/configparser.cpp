#include "configparser.h"

#include "3rd-party/catch.hpp"
#include "keymap.h"
#include "test-helpers.h"

using namespace newsboat;

TEST_CASE("evaluate_backticks replaces command in backticks with its output",
	"[ConfigParser]")
{
	SECTION("substitutes command output")
	{
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

	SECTION("subsistutes multiple shellouts")
	{
		REQUIRE(ConfigParser::evaluate_backticks(
				"xxx`echo aaa`yyy`echo bbb`zzz") ==
			"xxxaaayyybbbzzz");
	}

	SECTION("backticks can be escaped with backslash")
	{
		REQUIRE(ConfigParser::evaluate_backticks(
				"hehe \\`two at a time\\`haha") ==
			"hehe `two at a time`haha");
	}

	SECTION("single backticks have to be escaped too")
	{
		REQUIRE(ConfigParser::evaluate_backticks(
				"a single literal backtick: \\`") ==
			"a single literal backtick: `");
	}
	SECTION("commands with space are evaluated by backticks")
	{
		ConfigParser cfgparser;
		KeyMap keys(KM_NEWSBOAT);
		cfgparser.register_handler("bind-key", &keys);
		REQUIRE_NOTHROW(cfgparser.parse("data/config-space-backticks"));
		REQUIRE_FALSE(keys.get_operation("s", "all") == OP_NIL);
	}

	SECTION("Unbalanced backtick does *not* start a command")
	{
		const auto input1 = std::string("one `echo two three");
		REQUIRE(ConfigParser::evaluate_backticks(input1) == input1);

		const auto input2 = std::string("this `echo is a` test `here");
		const auto expected2 = std::string("this is a test `here");
		REQUIRE(ConfigParser::evaluate_backticks(input2) == expected2);
	}

	// One might think that putting one or both backticks inside a string
	// will "escape" them, the same way as backslash does. But it doesn't,
	// and shouldn't: when parsing a config, we need to evaluate *all*
	// commands there are, no matter where they're placed.
	SECTION("Backticks inside double quotes are not ignored")
	{
		const auto input1 = std::string(R"#("`echo hello`")#");
		REQUIRE(ConfigParser::evaluate_backticks(input1) ==
			R"#("hello")#");

		const auto input2 = std::string(R"#(a "b `echo c" d e` f)#");
		// The line above asks the shell to run 'echo c" d e', which is
		// an invalid command--the double quotes are not closed. The
		// standard output of that command would be empty, so nothing
		// will be inserted in place of backticks.
		const auto expected2 = std::string(R"#(a "b  f)#");
		REQUIRE(ConfigParser::evaluate_backticks(input2) == expected2);
	}
}

TEST_CASE("\"unbind-key -a\" removes all key bindings", "[ConfigParser]")
{
	ConfigParser cfgparser;

	SECTION("In all contexts by default")
	{
		KeyMap keys(KM_NEWSBOAT);
		cfgparser.register_handler("unbind-key", &keys);
		cfgparser.parse("data/config-unbind-all");

		for (int i = OP_QUIT; i < OP_NB_MAX; ++i) {
			REQUIRE(keys.getkey(static_cast<Operation>(i), "all") ==
				"<none>");
		}
	}

	SECTION("For a specific context")
	{
		KeyMap keys(KM_NEWSBOAT);
		cfgparser.register_handler("unbind-key", &keys);
		cfgparser.parse("data/config-unbind-all-context");

		INFO("it doesn't affect the help dialog");
		KeyMap default_keys(KM_NEWSBOAT);
		for (int i = OP_QUIT; i < OP_NB_MAX; ++i) {
			const auto op = static_cast<Operation>(i);
			REQUIRE(keys.getkey(op, "help") ==
				default_keys.getkey(op, "help"));
		}

		for (int i = OP_QUIT; i < OP_NB_MAX; ++i) {
			REQUIRE(keys.getkey(static_cast<Operation>(i),
					"article") == "<none>");
		}
	}
}

TEST_CASE("include directive includes other config files", "[ConfigParser]")
{
	// TODO: error messages should be more descriptive than "file couldn't
	// be opened"
	ConfigParser cfgparser;
	SECTION("Errors on not found file")
	{
		REQUIRE_THROWS(cfgparser.parse("data/config-missing-include"));
	}
	SECTION("Terminates on recursive include")
	{
		REQUIRE_THROWS(
			cfgparser.parse("data/config-recursive-include"));
	}
	SECTION("Successfully includes existing file")
	{
		REQUIRE_NOTHROW(
			cfgparser.parse("data/config-absolute-include"));
	}
	SECTION("Success on relative includes")
	{
		REQUIRE_NOTHROW(
			cfgparser.parse("data/config-relative-include"));
	}
	SECTION("Diamond of death includes pass")
	{
		REQUIRE_NOTHROW(cfgparser.parse("data/diamond-of-death/A"));
	}
	SECTION("File including itself only gets evaluated once")
	{
		TestHelpers::TempFile testfile;
		TestHelpers::EnvVar tmpfile(
			"TMPFILE"); // $TMPFILE used in conf file
		tmpfile.set(testfile.get_path());

		REQUIRE_NOTHROW(cfgparser.parse(
			"data/recursive-include-side-effect")); // recursive
								// includes
								// don't fail
		// I think it will never get below here and fail? If it
		// recurses, the above fails

		int line_count = 0;
		{ // from https://stackoverflow.com/a/19140230
			std::ifstream in(testfile.get_path());
			std::string line;
			while (std::getline(in, line)) {
				line_count++;
			}
		}
		REQUIRE(line_count == 1); // only 1 line from date command
	}
}

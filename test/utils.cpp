#include "utils.h"

#include <cctype>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <memory>
#include <mutex>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

#include "3rd-party/catch.hpp"
#include "links.h"
#include "test_helpers/chdir.h"
#include "test_helpers/envvar.h"
#include "test_helpers/stringmaker/filepath.h"
#include "test_helpers/stringmaker/optional.h"
#include "test_helpers/tempdir.h"
#include "test_helpers/tempfile.h"
#include "test_helpers/misc.h"

using namespace newsboat;

TEST_CASE("tokenize() extracts tokens separated by given delimiters", "[utils]")
{
	std::vector<std::string> tokens;

	SECTION("Default delimiters") {
		tokens = utils::tokenize("as df qqq");
		REQUIRE(tokens.size() == 3);
		REQUIRE(tokens[0] == "as");
		REQUIRE(tokens[1] == "df");
		REQUIRE(tokens[2] == "qqq");

		tokens = utils::tokenize(" aa ");
		REQUIRE(tokens.size() == 1);
		REQUIRE(tokens[0] == "aa");

		tokens = utils::tokenize("	");
		REQUIRE(tokens.size() == 0);

		tokens = utils::tokenize("");
		REQUIRE(tokens.size() == 0);
	}

	SECTION("Splitting by tabulation characters") {
		tokens = utils::tokenize("hello world\thow are you?", "\t");
		REQUIRE(tokens.size() == 2);
		REQUIRE(tokens[0] == "hello world");
		REQUIRE(tokens[1] == "how are you?");
	}
}

TEST_CASE(
	"tokenize_spaced() splits string into runs of delimiter characters "
	"interspersed with runs of non-delimiter chars",
	"[utils]")
{
	std::vector<std::string> tokens;

	SECTION("Default delimiters include space and tab") {
		tokens = utils::tokenize_spaced("a b");
		REQUIRE(tokens.size() == 3);
		REQUIRE(tokens[1] == " ");

		tokens = utils::tokenize_spaced(" a\t b ");
		REQUIRE(tokens.size() == 5);
		REQUIRE(tokens[0] == " ");
		REQUIRE(tokens[1] == "a");
		REQUIRE(tokens[2] == "\t ");
		REQUIRE(tokens[3] == "b");
		REQUIRE(tokens[4] == " ");
	}

	SECTION("Comma-separated values containing spaces and tabs") {
		tokens = utils::tokenize_spaced("123,John Doe,\t\t$8", ",");
		REQUIRE(tokens.size() == 5);
		REQUIRE(tokens[0] == "123");
		REQUIRE(tokens[1] == ",");
		REQUIRE(tokens[2] == "John Doe");
		REQUIRE(tokens[3] == ",");
		REQUIRE(tokens[4] == "\t\t$8");
	}
}

TEST_CASE(
	"tokenize_quoted() splits string on delimiters, treating strings "
	"inside double quotes as single token",
	"[utils]")
{
	std::vector<std::string> tokens;

	SECTION("Default delimiters include spaces, newlines and tabs") {
		tokens = utils::tokenize_quoted(
				"asdf \"foobar bla\" \"foo\\r\\n\\tbar\"");
		REQUIRE(tokens.size() == 3);
		REQUIRE(tokens[0] == "asdf");
		REQUIRE(tokens[1] == "foobar bla");
		REQUIRE(tokens[2] == "foo\r\n\tbar");

		tokens = utils::tokenize_quoted("  \"foo \\\\xxx\"\t\r \" \"");
		REQUIRE(tokens.size() == 2);
		REQUIRE(tokens[0] == "foo \\xxx");
		REQUIRE(tokens[1] == " ");
	}

	SECTION("Closing double quote marks the end of a token") {
		tokens = utils::tokenize_quoted(R"(set browser "mpv %u";)");

		REQUIRE(tokens.size() == 4);
		REQUIRE(tokens[0] == "set");
		REQUIRE(tokens[1] == "browser");
		REQUIRE(tokens[2] == "mpv %u");
		REQUIRE(tokens[3] == ";");
	}
}

TEST_CASE("tokenize_quoted() implicitly closes quotes at the end of the string",
	"[utils]")
{
	std::vector<std::string> tokens;

	tokens = utils::tokenize_quoted("\"\\\\");
	REQUIRE(tokens.size() == 1);
	REQUIRE(tokens[0] == "\\");

	tokens = utils::tokenize_quoted("\"\\\\\" and \"some other stuff");
	REQUIRE(tokens.size() == 3);
	REQUIRE(tokens[0] == "\\");
	REQUIRE(tokens[1] == "and");
	REQUIRE(tokens[2] == "some other stuff");

	tokens = utils::tokenize_quoted(R"("abc\)");
	REQUIRE(tokens.size() == 1);
	REQUIRE(tokens[0] == "abc");
}

TEST_CASE(
	"tokenize_quoted() interprets \"\\\\\" as escaped backslash and puts "
	"single backslash in output",
	"[utils]")
{
	std::vector<std::string> tokens;

	tokens = utils::tokenize_quoted(R"_("")_");
	REQUIRE(tokens.size() == 1);
	REQUIRE(tokens[0] == "");

	tokens = utils::tokenize_quoted(R"_("\\")_");
	REQUIRE(tokens.size() == 1);
	REQUIRE(tokens[0] == R"_(\)_");

	tokens = utils::tokenize_quoted(R"_("#\\")_");
	REQUIRE(tokens.size() == 1);
	REQUIRE(tokens[0] == R"_(#\)_");

	tokens = utils::tokenize_quoted(R"_("'#\\'")_");
	REQUIRE(tokens.size() == 1);
	REQUIRE(tokens[0] == R"_('#\')_");

	tokens = utils::tokenize_quoted(R"_("'#\\ \\'")_");
	REQUIRE(tokens.size() == 1);
	REQUIRE(tokens[0] == R"_('#\ \')_");

	tokens = utils::tokenize_quoted("\"\\\\\\\\");
	REQUIRE(tokens.size() == 1);
	REQUIRE(tokens[0] == "\\\\");

	tokens = utils::tokenize_quoted("\"\\\\\\\\\\\\");
	REQUIRE(tokens.size() == 1);
	REQUIRE(tokens[0] == "\\\\\\");

	tokens = utils::tokenize_quoted("\"\\\\\\\\\"");
	REQUIRE(tokens.size() == 1);
	REQUIRE(tokens[0] == "\\\\");

	tokens = utils::tokenize_quoted("\"\\\\\\\\\\\\\"");
	REQUIRE(tokens.size() == 1);
	REQUIRE(tokens[0] == "\\\\\\");

	// https://github.com/newsboat/newsboat/issues/642
	tokens = utils::tokenize_quoted(R"("\\bgit\\b")");
	REQUIRE(tokens.size() == 1);
	REQUIRE(tokens[0] == R"(\bgit\b)");

	// https://github.com/newsboat/newsboat/issues/536
	tokens = utils::tokenize_quoted(
			R"(browser "/Applications/Google\\ Chrome.app/Contents/MacOS/Google\\ Chrome --app %u")");
	REQUIRE(tokens.size() == 2);
	REQUIRE(tokens[0] == "browser");
	REQUIRE(tokens[1] ==
		R"(/Applications/Google\ Chrome.app/Contents/MacOS/Google\ Chrome --app %u)");
}

TEST_CASE("tokenize_quoted() doesn't un-escape escaped backticks", "[utils]")
{
	std::vector<std::string> tokens;

	tokens = utils::tokenize_quoted("asdf \"\\`foobar `bla`\\`\"");

	REQUIRE(tokens.size() == 2);
	REQUIRE(tokens[0] == "asdf");
	REQUIRE(tokens[1] == "\\`foobar `bla`\\`");
}

TEST_CASE("tokenize_quoted stops tokenizing once it found a # character "
	"(outside of double quotes)",
	"[utils]")
{
	std::vector<std::string> tokens;

	SECTION("A string consisting of just a comment") {
		tokens = utils::tokenize_quoted("# just a comment");
		REQUIRE(tokens.empty());
	}

	SECTION("A string with one quoted substring") {
		tokens = utils::tokenize_quoted(R"#("a test substring" # !!!)#");
		REQUIRE(tokens.size() == 1);
		REQUIRE(tokens[0] == "a test substring");
	}

	SECTION("A string with two quoted substrings") {
		tokens = utils::tokenize_quoted(R"#("first sub" "snd" # comment)#");
		REQUIRE(tokens.size() == 2);
		REQUIRE(tokens[0] == "first sub");
		REQUIRE(tokens[1] == "snd");
	}

	SECTION("A comment containing # character") {
		tokens = utils::tokenize_quoted(R"#(one # a comment with # char)#");
		REQUIRE(tokens.size() == 1);
		REQUIRE(tokens[0] == "one");
	}

	SECTION("A # character inside quoted substring is ignored") {
		tokens = utils::tokenize_quoted(R"#(this "will # be" ignored)#");
		REQUIRE(tokens.size() == 3);
		REQUIRE(tokens[0] == "this");
		REQUIRE(tokens[1] == "will # be");
		REQUIRE(tokens[2] == "ignored");
	}
}

TEST_CASE("tokenize_quoted does not consider escaped pound sign (\\#) "
	"a beginning of a comment",
	"[utils]")
{
	const auto tokens = utils::tokenize_quoted(R"#(one \# two three # ???)#");
	REQUIRE(tokens.size() == 4);
	REQUIRE(tokens[0] == "one");
	REQUIRE(tokens[1] == "\\#");
	REQUIRE(tokens[2] == "two");
	REQUIRE(tokens[3] == "three");
}

TEST_CASE("extract_token_quoted() returns no result if it finds a comment (signaled by '#')",
	"[utils]")
{
	SECTION("actual comment") {
		std::string str = "\t\t  # commented out";
		auto token = utils::extract_token_quoted(str);
		REQUIRE_FALSE(token.has_value());
		REQUIRE(str == "");
	}

	SECTION("ignores '#' if it is inside quotes") {
		std::string str = R"("# in quoted token" other tokens)";
		auto token = utils::extract_token_quoted(str);
		REQUIRE(token.has_value());
		REQUIRE(token.value() == "# in quoted token");
		REQUIRE(str == " other tokens");
	}

	SECTION("token before start of comment") {
		std::string str = "some-token # some comment";
		auto token = utils::extract_token_quoted(str);
		REQUIRE(token.has_value());
		REQUIRE(token.value() == "some-token");
		REQUIRE(str == " # some comment");
	}
}

TEST_CASE("extract_token_quoted() ignores configured `delimiters` in front of tokens",
	"[utils]")
{
	SECTION("default delimiters") {
		std::string str = "\n\r\t \n\t\r token-name";
		auto token = utils::extract_token_quoted(str);
		REQUIRE(token.has_value());
		REQUIRE(token.value() == "token-name");
		REQUIRE(str == "");
	}

	SECTION("empty delimiters") {
		std::string str = "\n\r\t \n\t\r token-name";
		auto token = utils::extract_token_quoted(str, "");
		REQUIRE(token.has_value());
		REQUIRE(token.value() == "\n\r\t \n\t\r token-name");
		REQUIRE(str == "");
	}

	SECTION("single delimiter") {
		std::string str = "--token name--";
		auto token = utils::extract_token_quoted(str, "-");
		REQUIRE(token.has_value());
		REQUIRE(token.value() == "token name");
		REQUIRE(str == "--");
	}

	SECTION("two delimiters") {
		std::string str = "--token name--";
		auto token = utils::extract_token_quoted(str, " -");
		REQUIRE(token.has_value());
		REQUIRE(token.value() == "token");
		REQUIRE(str == " name--");
	}
}

TEST_CASE("extract_token_quoted() ignores delimiter characters within quoted strings",
	"[utils]")
{
	SECTION("default delimiters") {
		std::string str = R"(  "token name"  )";
		auto token = utils::extract_token_quoted(str);
		REQUIRE(token.has_value());
		REQUIRE(token.value() == "token name");
		REQUIRE(str == "  ");
	}

	SECTION("two delimiters") {
		std::string str = R"(--"token name"--)";
		auto token = utils::extract_token_quoted(str, " -");
		REQUIRE(token.has_value());
		REQUIRE(token.value() == "token name");
		REQUIRE(str == "--");
	}
}

TEST_CASE("extract_token_quoted() processes escape sequences within quoted strings",
	"[utils]")
{
	std::string str = R"(  "\n \r \t \" \` \\ " remainder)";
	auto token = utils::extract_token_quoted(str);
	REQUIRE(token.has_value());
	REQUIRE(token.value() == "\n \r \t \" \\` \\ ");
	REQUIRE(str == " remainder");
}

TEST_CASE("extract_token_quoted() works with Unicode strings too", "[utils]")
{
	std::string str = R"("привет мир" Юникода)";
	const auto token = utils::extract_token_quoted(str);
	REQUIRE(token.has_value());
	REQUIRE(token.value() == "привет мир");
	REQUIRE(str == " Юникода");
}

TEST_CASE("tokenize_nl() split a string into delimiters and fields", "[utils]")
{
	std::vector<std::string> tokens;

	SECTION("a few words separated by newlines") {
		tokens = utils::tokenize_nl("first\nsecond\nthird");

		REQUIRE(tokens.size() == 5);
		REQUIRE(tokens[0] == "first");
		REQUIRE(tokens[2] == "second");
		REQUIRE(tokens[4] == "third");
	}

	SECTION("several preceding delimiters") {
		tokens = utils::tokenize_nl("\n\n\nonly");

		REQUIRE(tokens.size() == 4);
		REQUIRE(tokens[3] == "only");
	}

	SECTION("redundant internal delimiters") {
		tokens = utils::tokenize_nl("first\nsecond\n\nthird");

		REQUIRE(tokens.size() == 6);
		REQUIRE(tokens[0] == "first");
		REQUIRE(tokens[2] == "second");
		REQUIRE(tokens[5] == "third");
	}

	SECTION("custom delimiter") {
		tokens = utils::tokenize_nl("first\nsecond\nthird", "i");

		REQUIRE(tokens.size() == 5);
		REQUIRE(tokens[0] == "f");
		REQUIRE(tokens[2] == "rst\nsecond\nth");
		REQUIRE(tokens[4] == "rd");
	}

	SECTION("no non-delimiter text") {
		SECTION("single newline") {
			tokens = utils::tokenize_nl("\n");

			REQUIRE(tokens.size() == 1);
			REQUIRE(tokens[0] == "\n");
		}

		SECTION("multiple newlines") {
			tokens = utils::tokenize_nl("\n\n\n");

			REQUIRE(tokens.size() == 3);
			REQUIRE(tokens[0] == "\n");
			REQUIRE(tokens[1] == "\n");
			REQUIRE(tokens[2] == "\n");
		}
	}
}

TEST_CASE(
	"strip_comments returns only the part of the line before first # character",
	"[utils]")
{
	SECTION("no comments in line") {
		REQUIRE(utils::strip_comments("") == "");
		REQUIRE(utils::strip_comments("\t\n") == "\t\n");
		REQUIRE(utils::strip_comments("some directive ") == "some directive ");
	}

	SECTION("fully commented line") {
		REQUIRE(utils::strip_comments("#") == "");
		REQUIRE(utils::strip_comments("# #") == "");
		REQUIRE(utils::strip_comments("# comment") == "");
	}

	SECTION("partially commented line") {
		REQUIRE(utils::strip_comments("directive # comment") == "directive ");
		REQUIRE(utils::strip_comments("directive # comment # another") == "directive ");
		REQUIRE(utils::strip_comments("directive#comment") == "directive");
	}
}

TEST_CASE("strip_comments ignores escaped # characters (\\#)")
{
	const auto expected =
		std::string(R"#(one two \# three four)#");
	const auto input = expected + "# and a comment";
	REQUIRE(utils::strip_comments(input) == expected);
}

TEST_CASE("strip_comments ignores # characters inside double quotes",
	"[utils][issue652]")
{
	SECTION("Real-world cases from issue 652") {
		const auto expected1 =
			std::string(R"#(highlight article "[-=+#_*~]{3,}.*" green default)#");
		const auto input1 = expected1 + "# this is a comment";
		REQUIRE(utils::strip_comments(input1) == expected1);

		const auto expected2 =
			std::string(
				R"#(highlight all "(https?|ftp)://[\-\.,/%~_:?&=\#a-zA-Z0-9]+" blue default bold)#");
		const auto input2 = expected2 + "#heresacomment";
		REQUIRE(utils::strip_comments(input2) == expected2);
	}

	SECTION("Escaped double quote inside double quotes is not treated "
		"as closing quote") {
		const auto expected =
			std::string(R"#(test "here \"goes # nothing\" etc" hehe)#");
		const auto input = expected + "# and here is a comment";
		REQUIRE(utils::strip_comments(input) == expected);
	}
}

TEST_CASE("strip_comments ignores # characters inside backticks", "[utils]")
{
	SECTION("Simple case") {
		const auto expected = std::string(R"#(one `two # three` four)#");
		const auto input = expected + "# and a comment, of course";
		REQUIRE(utils::strip_comments(input) == expected);
	}

	SECTION("Escaped backtick inside backticks is not treated as closing") {
		const auto expected =
			std::string(R"#(some `other \` tricky # test` hehe)#");
		const auto input = expected + "#here goescomment";
		REQUIRE(utils::strip_comments(input) == expected);
	}
}

TEST_CASE("strip_comments is not confused by nested double quotes and backticks",
	"[utils]")
{
	{
		const auto expected = std::string(R"#("`" ... ` `"` ")#");
		const auto input = expected + "#comment";
		REQUIRE(utils::strip_comments(input) == expected);
	}

	{
		const auto expected = std::string(R"#(aaa ` bbb "ccc ddd" e` dd)#");
		const auto input = expected + "# a comment string";
		REQUIRE(utils::strip_comments(input) == expected);
	}

	{
		const auto expected =
			std::string(R"#(option "this `weird " command` for value")#");
		const auto input = expected + "#and a comment";
		REQUIRE(utils::strip_comments(input) == expected);
	}
}

TEST_CASE(
	"consolidate_whitespace replaces multiple consecutive"
	"whitespace with a single space",
	"[utils]")
{
	REQUIRE(utils::consolidate_whitespace("LoremIpsum") == "LoremIpsum");
	REQUIRE(utils::consolidate_whitespace("Lorem Ipsum") == "Lorem Ipsum");
	REQUIRE(utils::consolidate_whitespace(" Lorem \t\tIpsum \t ") ==
		" Lorem Ipsum ");
	REQUIRE(utils::consolidate_whitespace(" Lorem \r\n\r\n\tIpsum") ==
		" Lorem Ipsum");

	REQUIRE(utils::consolidate_whitespace("") == "");
}

TEST_CASE("consolidate_whitespace preserves leading whitespace", "[utils]")
{
	REQUIRE(utils::consolidate_whitespace("    Lorem \t\tIpsum \t ") ==
		"    Lorem Ipsum ");
	REQUIRE(utils::consolidate_whitespace("   Lorem \r\n\r\n\tIpsum") ==
		"   Lorem Ipsum");
}

TEST_CASE("get_command_output()", "[utils]")
{
	REQUIRE(utils::get_command_output("ls /dev/null") == "/dev/null\n");
	REQUIRE_NOTHROW(utils::get_command_output(
			"a-program-that-is-guaranteed-to-not-exists"));
	REQUIRE(utils::get_command_output(
			"a-program-that-is-guaranteed-to-not-exists") == "");
	REQUIRE(utils::get_command_output("echo c\" d e") == "");
}

TEST_CASE("extract_filter()", "[utils]")
{
	{
		const auto parts =
			utils::extract_filter("filter:~/bin/script.sh:https://newsboat.org");
		REQUIRE(std::string(parts.script_name) == "~/bin/script.sh");
		REQUIRE(std::string(parts.url) == "https://newsboat.org");
	}

	{
		const auto parts = utils::extract_filter("filter::https://newsboat.org");
		REQUIRE(std::string(parts.script_name) == "");
		REQUIRE(std::string(parts.url) == "https://newsboat.org");
	}

	{
		const auto parts = utils::extract_filter("filter:https://newsboat.org");
		REQUIRE(std::string(parts.script_name) == "https");
		REQUIRE(std::string(parts.url) == "//newsboat.org");
	}

	{
		const auto parts = utils::extract_filter("filter:foo:");
		REQUIRE(std::string(parts.script_name) == "foo");
		REQUIRE(std::string(parts.url) == "");
	}

	{
		const auto parts = utils::extract_filter("filter:");
		REQUIRE(std::string(parts.script_name) == "");
		REQUIRE(std::string(parts.url) == "");
	}
}

TEST_CASE("run_program()", "[utils]")
{
	const char* argv[4];
	argv[0] = "cat";
	argv[1] = nullptr;
	REQUIRE(utils::run_program(
			argv, "this is a multine-line\ntest string") ==
		"this is a multine-line\ntest string");

	argv[0] = "echo";
	argv[1] = "-n";
	argv[2] = "hello world";
	argv[3] = nullptr;
	REQUIRE(utils::run_program(argv, "") == "hello world");
}

TEST_CASE("run_program() works for large inputs", "[utils]")
{
	const auto large_input = std::make_shared<std::string>(1000000, 'a');

	struct Sync {
		bool thread_finished = false;
		std::mutex mtx;
		std::condition_variable condvar;
		std::string output;
	};
	auto sync = std::make_shared<Sync>();

	std::thread([large_input, sync]() {
		const char* argv[4];
		argv[0] = "sh";
		argv[1] = "-c";
		argv[2] = "cat";
		argv[3] = nullptr;
		sync->output = utils::run_program(argv, *large_input);

		{
			std::lock_guard<std::mutex> g(sync->mtx);
			sync->thread_finished = true;
		}
		sync->condvar.notify_one();
	}).detach();

	std::unique_lock<std::mutex> g(sync->mtx);
	// cat should be able to process 1MB of input in under a second
	sync->condvar.wait_for(g, std::chrono::seconds(1), [&]() {
		return sync->thread_finished;
	});

	REQUIRE(sync->thread_finished);
	REQUIRE(sync->output == *large_input);
}

TEST_CASE("run_command() executes the given command with a given argument",
	"[utils]")
{
	test_helpers::TempFile sentry;
	const auto argument = sentry.get_path();

	{
		INFO("File shouldn't exist, because TempFile doesn't create it");

		struct stat sb;
		const int result = ::stat(argument.c_str(), &sb);
		const int saved_errno = errno;

		REQUIRE(result == -1);
		REQUIRE(saved_errno == ENOENT);
	}

	utils::run_command("touch", argument);

	struct stat sb;
	int result = 0;

	// Busy-wait for 100 tries of 10 milliseconds each, waiting for `touch` to
	// create the file. Usually it happens quickly, and the loop exists on the
	// first try; but sometimes on CI it takes longer for `touch` to finish, so
	// we need a slightly longer wait.
	int tries = 100;
	while (tries-- > 0) {
		::usleep(10 * 1000);

		result = ::stat(argument.c_str(), &sb);
		if (result == 0) {
			break;
		}
	}

	INFO("File should have been created by the `touch`");
	REQUIRE(result == 0);
}

TEST_CASE("run_command() doesn't wait for the command to finish",
	"[utils]")
{
	using namespace std::chrono;

	const auto start = steady_clock::now();

	// Using a big timeout value of 60 seconds to overcome any slowdowns that
	// Cirrus CI sometimes exhibits, e.g. here waiting for `sleep 5` took 19
	// seconds: https://cirrus-ci.com/task/6641382309756928?command=test#L64
	utils::run_command("sleep", "60");

	const auto finish = steady_clock::now();
	const auto runtime = duration_cast<milliseconds>(finish - start);

	REQUIRE(runtime.count() < 60000);
}

TEST_CASE("resolve_tilde() replaces ~ with the path to the home directory",
	"[utils]")
{
	test_helpers::EnvVar envVar("HOME");
	envVar.set("test");
	REQUIRE(utils::resolve_tilde("~") == "test/");
	REQUIRE(utils::resolve_tilde("~/") == "test/");
	REQUIRE(utils::resolve_tilde("~/dir") == "test/dir");
	REQUIRE(utils::resolve_tilde("/home/~") == "/home/~");
	REQUIRE(
		utils::resolve_tilde("~/foo/bar") ==
		"test/foo/bar"
	);
	REQUIRE(utils::resolve_tilde("/foo/bar") == "/foo/bar");
}

TEST_CASE("resolve_relative() returns an absolute file path relative to another",
	"[utils]")
{
	SECTION("Nothing - absolute path") {
		REQUIRE(utils::resolve_relative("/foo/bar", "/baz") == "/baz");
		REQUIRE(utils::resolve_relative("/config", "/config/baz") == "/config/baz");
	}
	SECTION("Reference path") {
		REQUIRE(utils::resolve_relative("/foo/bar", "baz") == "/foo/baz");
		REQUIRE(utils::resolve_relative("/config", "baz") == "/baz");
	}
}

TEST_CASE("replace_all()", "[utils]")
{
	REQUIRE(utils::replace_all("aaa", "a", "b") == "bbb");
	REQUIRE(utils::replace_all("aaa", "aa", "ba") == "baa");
	REQUIRE(utils::replace_all("aaaaaa", "aa", "ba") == "bababa");
	REQUIRE(utils::replace_all("", "a", "b") == "");
	REQUIRE(utils::replace_all("aaaa", "b", "c") == "aaaa");
	REQUIRE(utils::replace_all("this is a normal test text", " t", " T") ==
		"this is a normal Test Text");
	REQUIRE(utils::replace_all("o o o", "o", "<o>") == "<o> <o> <o>");
}

TEST_CASE("replace_all() with from/to pairs", "[utils]")
{
	SECTION("output == input if no from/to pairs are specified") {
		const std::string input = "lorem ipsum";
		REQUIRE(utils::replace_all("", {}) == "");
		REQUIRE(utils::replace_all(input, {}) == input);
	}

	SECTION("supports multiple from/to pairs") {
		const std::vector<std::pair<std::string, std::string>> rules = {
			{"a", "x"},
			{"bb", "y"},
			{"c", "zz"}
		};

		SECTION("empty input results in empty output") {
			REQUIRE(utils::replace_all("", rules) == "");
		}

		SECTION("a from/to pair can be matched multiple times") {
			REQUIRE(utils::replace_all("aaa", rules) == "xxx");
			REQUIRE(utils::replace_all("bbb", rules) == "yb");
			REQUIRE(utils::replace_all("ccc", rules) == "zzzzzz");
		}

		SECTION("multiple from/to pairs can be matched in the same input") {
			REQUIRE(utils::replace_all("abc cccbbbaaa", rules) == "xbzz zzzzzzybxxx");
			REQUIRE(utils::replace_all("begin abc cccbbbaaa end",
					rules) == "begin xbzz zzzzzzybxxx end");
		}
	}

	SECTION("multiple from/to pairs do not create loop") {
		const std::vector<std::pair<std::string, std::string>> rules = {
			{"a", "b"},
			{"b", "cc"},
			{"cc", "z"}
		};

		REQUIRE(utils::replace_all("a", rules) == "b");
		REQUIRE(utils::replace_all("aaa a", rules) == "bbb b");
		REQUIRE(utils::replace_all("abc", rules) == "bccc");
		REQUIRE(utils::replace_all("ccab", rules) == "zbcc");
	}

	SECTION("from/to pairs can start with common substring") {
		const std::vector<std::pair<std::string, std::string>> rules = {
			{"%u", "lorem"},
			{"%F", "ipsum"},
			{"%%", "%"}
		};

		REQUIRE(utils::replace_all("%u", rules) == "lorem");
		REQUIRE(utils::replace_all("%F", rules) == "ipsum");
		REQUIRE(utils::replace_all("%%", rules) == "%");
		REQUIRE(utils::replace_all("%% %u %F", rules) == "% lorem ipsum");
	}

	SECTION("if multiple pairs match, first one is used") {
		const std::vector<std::pair<std::string, std::string>> rules = {
			{"test", "lorem"},
			{"t", "ipsum"},
			{"tester", "dolar"}
		};

		REQUIRE(utils::replace_all("tester", rules) == "loremer");
		REQUIRE(utils::replace_all("ts", rules) == "ipsums");
	}
}

TEST_CASE("to_string()", "[utils]")
{
	REQUIRE(std::to_string(0) == "0");
	REQUIRE(std::to_string(100) == "100");
	REQUIRE(std::to_string(65536) == "65536");
	REQUIRE(std::to_string(65537) == "65537");
}

TEST_CASE("partition_index()", "[utils]")
{
	std::vector<std::pair<unsigned int, unsigned int>> partitions;

	SECTION("[0, 9] into 2") {
		partitions = utils::partition_indexes(0, 9, 2);
		REQUIRE(partitions.size() == 2);
		REQUIRE(partitions[0].first == 0);
		REQUIRE(partitions[0].second == 4);
		REQUIRE(partitions[1].first == 5);
		REQUIRE(partitions[1].second == 9);
	}

	SECTION("[0, 10] into 3") {
		partitions = utils::partition_indexes(0, 10, 3);
		REQUIRE(partitions.size() == 3);
		REQUIRE(partitions[0].first == 0);
		REQUIRE(partitions[0].second == 2);
		REQUIRE(partitions[1].first == 3);
		REQUIRE(partitions[1].second == 5);
		REQUIRE(partitions[2].first == 6);
		REQUIRE(partitions[2].second == 10);
	}

	SECTION("[0, 11] into 3") {
		partitions = utils::partition_indexes(0, 11, 3);
		REQUIRE(partitions.size() == 3);
		REQUIRE(partitions[0].first == 0);
		REQUIRE(partitions[0].second == 3);
		REQUIRE(partitions[1].first == 4);
		REQUIRE(partitions[1].second == 7);
		REQUIRE(partitions[2].first == 8);
		REQUIRE(partitions[2].second == 11);
	}

	SECTION("[0, 199] into 200") {
		partitions = utils::partition_indexes(0, 199, 200);
		REQUIRE(partitions.size() == 200);
	}

	SECTION("[0, 103] into 1") {
		partitions = utils::partition_indexes(0, 103, 1);
		REQUIRE(partitions.size() == 1);
		REQUIRE(partitions[0].first == 0);
		REQUIRE(partitions[0].second == 103);
	}
}

TEST_CASE("censor_url()", "[utils]")
{
	REQUIRE(utils::censor_url("") == "");
	REQUIRE(utils::censor_url("foobar") == "foobar");
	REQUIRE(utils::censor_url("foobar://xyz/") == "foobar://xyz/");

	REQUIRE(utils::censor_url("http://newsbeuter.org/") ==
		"http://newsbeuter.org/");
	REQUIRE(utils::censor_url("https://newsbeuter.org/") ==
		"https://newsbeuter.org/");

	REQUIRE(utils::censor_url("http://@newsbeuter.org/") ==
		"http://newsbeuter.org/");
	REQUIRE(utils::censor_url("https://@newsbeuter.org/") ==
		"https://newsbeuter.org/");

	REQUIRE(utils::censor_url("http://foo:bar@newsbeuter.org/") ==
		"http://*:*@newsbeuter.org/");
	REQUIRE(utils::censor_url("https://foo:bar@newsbeuter.org/") ==
		"https://*:*@newsbeuter.org/");

	REQUIRE(utils::censor_url("http://aschas@newsbeuter.org/") ==
		"http://*:*@newsbeuter.org/");
	REQUIRE(utils::censor_url("https://aschas@newsbeuter.org/") ==
		"https://*:*@newsbeuter.org/");

	REQUIRE(utils::censor_url("xxx://aschas@newsbeuter.org/") ==
		"xxx://*:*@newsbeuter.org/");

	REQUIRE(utils::censor_url("http://foobar") == "http://foobar/");
	REQUIRE(utils::censor_url("https://foobar") == "https://foobar/");

	REQUIRE(utils::censor_url("http://aschas@host") == "http://*:*@host/");
	REQUIRE(utils::censor_url("https://aschas@host") == "https://*:*@host/");

	REQUIRE(utils::censor_url("query:name:age between 1:10") ==
		"query:name:age between 1:10");
}

TEST_CASE("absolute_url()", "[utils]")
{
	REQUIRE(utils::absolute_url("http://foobar/hello/crook/", "bar.html") ==
		"http://foobar/hello/crook/bar.html");
	REQUIRE(utils::absolute_url("https://foobar/foo/", "/bar.html") ==
		"https://foobar/bar.html");
	REQUIRE(utils::absolute_url("https://foobar/foo/",
			"http://quux/bar.html") == "http://quux/bar.html");
	REQUIRE(utils::absolute_url("http://foobar", "bla.html") ==
		"http://foobar/bla.html");
	REQUIRE(utils::absolute_url("http://test:test@foobar:33",
			"bla2.html") == "http://test:test@foobar:33/bla2.html");
}

TEST_CASE("absolute_url() strips ASCII whitespace", "[utils]")
{
	// ASCII whitespace characters are: tab, line feed, form feed, carriage
	// return, and space

	REQUIRE(utils::absolute_url(" \t https://example.com/page1",
			"page2") == "https://example.com/page2");
	REQUIRE(utils::absolute_url("https://example.com/page3  \n ",
			"page4") == "https://example.com/page4");
	REQUIRE(utils::absolute_url("  \rhttps://example.com/page5  \f ",
			"page6") == "https://example.com/page6");
	REQUIRE(utils::absolute_url("https://example.com/base",
			" \n replacement") == "https://example.com/replacement");
	REQUIRE(utils::absolute_url("https://example.com/different",
			"   another\t") == "https://example.com/another");
	REQUIRE(utils::absolute_url("https://example.com/~joe/",
			" \rhello\f") == "https://example.com/~joe/hello");
	REQUIRE(utils::absolute_url( "\f\n\rhttps://example.com/misc/\t    ",
			"   \t\feverything_at_once\n\r") == "https://example.com/misc/everything_at_once");
}

TEST_CASE("quote_for_stfl() adds a \'>\' after every \'<\'", "[utils]")
{
	REQUIRE(utils::quote_for_stfl("<<><><><") == "<><>><>><>><>");
	REQUIRE(utils::quote_for_stfl("test") == "test");
}

TEST_CASE("quote()", "[utils]")
{
	REQUIRE(utils::quote("") == "\"\"");
	REQUIRE(utils::quote("hello world") == "\"hello world\"");
	REQUIRE(utils::quote("\"hello world\"") == "\"\\\"hello world\\\"\"");
}

TEST_CASE("to_u()", "[utils]")
{
	REQUIRE(utils::to_u("0") == 0);
	REQUIRE(utils::to_u("23") == 23);
	REQUIRE(utils::to_u("") == 0);
}

TEST_CASE("strwidth()", "[utils]")
{
	REQUIRE(utils::strwidth("") == 0);

	REQUIRE(utils::strwidth("xx") == 2);

	const auto input1 = utils::wstr2str(L"\uF91F");
	REQUIRE(utils::strwidth(input1) == 2);
	REQUIRE(utils::strwidth("\07") == 1);
}

TEST_CASE("strwidth_stfl()", "[utils]")
{
	REQUIRE(utils::strwidth_stfl("") == 0);

	REQUIRE(utils::strwidth_stfl("x<hi>x") == 2);

	REQUIRE(utils::strwidth_stfl("x<longtag>x</>") == 2);

	REQUIRE(utils::strwidth_stfl("x<>x") == 3);

	REQUIRE(utils::strwidth_stfl("x<>hi>x") == 6);

	REQUIRE(utils::strwidth_stfl("x<>y<>z") == 5);

	const auto input1 = utils::wstr2str(L"\uF91F");
	REQUIRE(utils::strwidth_stfl(input1) == 2);
	REQUIRE(utils::strwidth_stfl("\07") == 1);

	REQUIRE(utils::strwidth_stfl("<a") == 0); // #415
	REQUIRE(utils::strwidth_stfl("a") == 1);
	REQUIRE(utils::strwidth_stfl("abc<tag>def") == 6);
	REQUIRE(utils::strwidth_stfl("less-than: <>") ==  12);
	REQUIRE(utils::strwidth_stfl("ＡＢＣＤＥＦ") == 12);
}

TEST_CASE("is_http_url()", "[utils]")
{
	REQUIRE(utils::is_http_url("http://example.com"));
	REQUIRE(utils::is_http_url("http://foo.bar"));
	REQUIRE(utils::is_http_url("https://foo.bar"));
	REQUIRE(utils::is_http_url("http://"));
	REQUIRE(utils::is_http_url("https://"));

	REQUIRE_FALSE(utils::is_http_url("htt://foo.bar"));
	REQUIRE_FALSE(utils::is_http_url("http:/"));
	REQUIRE_FALSE(utils::is_http_url("foo://bar"));
}

TEST_CASE("join()", "[utils]")
{
	std::vector<std::string> str;
	REQUIRE(utils::join(str, "") == "");
	REQUIRE(utils::join(str, "-") == "");

	SECTION("Join of one element") {
		str.push_back("foobar");
		REQUIRE(utils::join(str, "") == "foobar");
		REQUIRE(utils::join(str, "-") == "foobar");

		SECTION("Join of two elements") {
			str.push_back("quux");
			REQUIRE(utils::join(str, "") == "foobarquux");
			REQUIRE(utils::join(str, "-") == "foobar-quux");
		}
	}
}

TEST_CASE("trim() removes whitespace at start and end of string", "[utils]")
{
	std::string str = "  xxx\r\n";
	utils::trim(str);
	REQUIRE(str == "xxx");

	str = "\n\n \tabc  foobar\n";
	utils::trim(str);
	REQUIRE(str == "abc  foobar");

	str = "";
	utils::trim(str);
	REQUIRE(str == "");

	str = "     \n";
	utils::trim(str);
	REQUIRE(str == "");
}

TEST_CASE("trim_end()", "[utils]")
{
	std::string str = "quux\n";
	utils::trim_end(str);
	REQUIRE(str == "quux");
}

TEST_CASE("utils::make_title extracts possible title from URL", "[utils]")
{
	SECTION("Uses last part of URL as title") {
		auto input = "http://example.com/Item";
		REQUIRE(utils::make_title(input) == "Item");
	}

	SECTION("Replaces dashes and underscores with spaces") {
		std::string input;

		SECTION("Dashes") {
			input = "http://example.com/This-is-the-title";
		}

		SECTION("Underscores") {
			input = "http://example.com/This_is_the_title";
		}

		SECTION("Mix of dashes and underscores") {
			input = "http://example.com/This_is-the_title";
		}

		SECTION("Eliminate .php extension") {
			input = "http://example.com/This_is-the_title.php";
		}

		SECTION("Eliminate .html extension") {
			input = "http://example.com/This_is-the_title.html";
		}

		SECTION("Eliminate .htm extension") {
			input = "http://example.com/This_is-the_title.htm";
		}

		SECTION("Eliminate .aspx extension") {
			input = "http://example.com/This_is-the_title.aspx";
		}

		REQUIRE(utils::make_title(input) == "This is the title");
	}

	SECTION("Capitalizes first letter of extracted title") {
		auto input = "http://example.com/this-is-the-title";
		REQUIRE(utils::make_title(input) == "This is the title");
	}

	SECTION("Only cares about last component of the URL") {
		auto input = "http://example.com/items/misc/this-is-the-title";
		REQUIRE(utils::make_title(input) == "This is the title");
	}

	SECTION("Strips out trailing slashes") {
		std::string input;

		SECTION("One slash") {
			input = "http://example.com/item/";
		}

		SECTION("Numerous slashes") {
			input = "http://example.com/item/////////////";
		}

		REQUIRE(utils::make_title(input) == "Item");
	}

	SECTION("Doesn't mind invalid URL scheme") {
		auto input = "blahscheme://example.com/this-is-the-title";
		REQUIRE(utils::make_title(input) == "This is the title");
	}

	SECTION("Strips out URL query parameters") {
		SECTION("Single parameter") {
			auto input =
				"http://example.com/story/aug/"
				"title-with-dashes?a=b";
			REQUIRE(utils::make_title(input) ==
				"Title with dashes");
		}

		SECTION("Multiple parameters") {
			auto input =
				"http://example.com/"
				"title-with-dashes?a=b&x=y&utf8=✓";
			REQUIRE(utils::make_title(input) ==
				"Title with dashes");
		}
	}

	SECTION("Decodes percent-encoded characters") {
		auto input = "https://example.com/It%27s%202017%21";
		REQUIRE(utils::make_title(input) == "It's 2017!");
	}

	SECTION("Deal with an empty last component") {
		auto input = "https://example.com/?format=rss";
		REQUIRE(utils::make_title(input) == "");
	}

	SECTION("Deal with an empty input") {
		auto input = "";
		REQUIRE(utils::make_title(input) == "");
	}
}

TEST_CASE("run_interactively runs a command with inherited I/O", "[utils]")
{
	SECTION("echo hello should return 0") {
		const auto result = utils::run_interactively("echo hello", "test");
		REQUIRE(result == 0);
	}
	SECTION("exit 1 should return 1") {
		const auto result = utils::run_interactively("exit 1", "test");
		REQUIRE(result == 1);
	}

	// Unfortunately, there is no easy way to provoke this function to return
	// `std::nullopt`, nor to test that it returns just the lower 8 bits.
}

TEST_CASE("run_non_interactively runs a command without I/O", "[utils]")
{
	SECTION("echo hello should return 0") {
		const auto result = utils::run_non_interactively("echo hello", "test");
		REQUIRE(result == 0);
	}
	SECTION("exit 1 should return 1") {
		const auto result = utils::run_non_interactively("exit 1", "test");
		REQUIRE(result == 1);
	}

	// Unfortunately, there is no easy way to provoke this function to return
	// `std::nullopt`, nor to test that it returns just the lower 8 bits.
}

TEST_CASE("run_non_interactively waits for program to finish", "[utils]")
{
	using namespace std::chrono;

	SECTION("when sleeping, we use at least as much time as specified") {
		const auto start = steady_clock::now();

		const auto result = utils::run_non_interactively("sleep 1", "test");
		REQUIRE(result == 0);

		const auto finish = steady_clock::now();
		const auto runtime = duration_cast<milliseconds>(finish - start);
		REQUIRE(runtime.count() >= 1000);
	}

	SECTION("if program is put in background, using &, we return immediately") {
		const auto start = steady_clock::now();

		const auto result = utils::run_non_interactively("sleep 5 &", "test");
		REQUIRE(result == 0);

		const auto finish = steady_clock::now();
		const auto runtime = duration_cast<milliseconds>(finish - start);
		REQUIRE(runtime.count() < 5000);
	}
}

TEST_CASE("remove_soft_hyphens remove all U+00AD characters from a string",
	"[utils]")
{
	SECTION("doesn't do anything if input has no soft hyphens in it") {
		std::string data = "hello world!";
		REQUIRE_NOTHROW(utils::remove_soft_hyphens(data));
		REQUIRE(data == "hello world!");
	}

	SECTION("removes *all* soft hyphens") {
		std::string data = "hy\u00ADphen\u00ADa\u00ADtion";
		REQUIRE_NOTHROW(utils::remove_soft_hyphens(data));
		REQUIRE(data == "hyphenation");
	}

	SECTION("removes consequtive soft hyphens") {
		std::string data =
			"don't know why any\u00AD\u00ADone would do that";
		REQUIRE_NOTHROW(utils::remove_soft_hyphens(data));
		REQUIRE(data == "don't know why anyone would do that");
	}

	SECTION("removes soft hyphen at the beginning of the line") {
		std::string data = "\u00ADtion";
		REQUIRE_NOTHROW(utils::remove_soft_hyphens(data));
		REQUIRE(data == "tion");
	}

	SECTION("removes soft hyphen at the end of the line") {
		std::string data = "over\u00AD";
		REQUIRE_NOTHROW(utils::remove_soft_hyphens(data));
		REQUIRE(data == "over");
	}
}

TEST_CASE(
	"substr_with_width() returns a longest substring fits to the given "
	"width",
	"[utils]")
{
	REQUIRE(utils::substr_with_width("a", 1) == "a");
	REQUIRE(utils::substr_with_width("a", 2) == "a");
	REQUIRE(utils::substr_with_width("ab", 1) == "a");
	REQUIRE(utils::substr_with_width("abc", 1) == "a");
	REQUIRE(utils::substr_with_width("A\u3042B\u3044C\u3046", 5) ==
		"A\u3042B");

	SECTION("returns an empty string if the given string is empty") {
		REQUIRE(utils::substr_with_width("", 0) == "");
		REQUIRE(utils::substr_with_width("", 1) == "");
	}

	SECTION("returns an empty string if the given width is zero") {
		REQUIRE(utils::substr_with_width("world", 0) == "");
		REQUIRE(utils::substr_with_width("", 0) == "");
	}

	SECTION("doesn't split single codepoint in two") {
		std::string data = "\u3042\u3044\u3046";
		REQUIRE(utils::substr_with_width(data, 1) == "");
		REQUIRE(utils::substr_with_width(data, 3) == "\u3042");
		REQUIRE(utils::substr_with_width(data, 5) == "\u3042\u3044");
	}

	SECTION("handles angular brackets as regular characters") {
		REQUIRE(utils::substr_with_width("ＡＢＣ<b>ＤＥ</b>Ｆ", 9) ==
			"ＡＢＣ<b>");
		REQUIRE(utils::substr_with_width("<foobar>ＡＢＣ", 4) ==
			"<foo");
		REQUIRE(utils::substr_with_width("a<<xyz>>bcd", 3) ==
			"a<<");
		REQUIRE(utils::substr_with_width("ＡＢＣ<b>ＤＥ", 10) ==
			"ＡＢＣ<b>");
		REQUIRE(utils::substr_with_width("a</>b</>c</>", 2) ==
			"a<");
		REQUIRE(utils::substr_with_width("<><><>", 2) == "<>");
		REQUIRE(utils::substr_with_width("a<>b<>c", 3) == "a<>");
	}

	SECTION("treats non-printable characters as 1 wide") {
		REQUIRE(utils::substr_with_width("\x01\x02" "abc", 1) == "\x01");
	}
}

TEST_CASE(
	"substr_with_width_stfl() returns a longest substring fits to the given "
	"width",
	"[utils]")
{
	REQUIRE(utils::substr_with_width_stfl("a", 1) == "a");
	REQUIRE(utils::substr_with_width_stfl("a", 2) == "a");
	REQUIRE(utils::substr_with_width_stfl("ab", 1) == "a");
	REQUIRE(utils::substr_with_width_stfl("abc", 1) == "a");
	REQUIRE(utils::substr_with_width_stfl("A\u3042B\u3044C\u3046", 5) ==
		"A\u3042B");

	SECTION("returns an empty string if the given string is empty") {
		REQUIRE(utils::substr_with_width_stfl("", 0) == "");
		REQUIRE(utils::substr_with_width_stfl("", 1) == "");
	}

	SECTION("returns an empty string if the given width is zero") {
		REQUIRE(utils::substr_with_width_stfl("world", 0) == "");
		REQUIRE(utils::substr_with_width_stfl("", 0) == "");
	}

	SECTION("doesn't split single codepoint in two") {
		std::string data = "\u3042\u3044\u3046";
		REQUIRE(utils::substr_with_width_stfl(data, 1) == "");
		REQUIRE(utils::substr_with_width_stfl(data, 3) == "\u3042");
		REQUIRE(utils::substr_with_width_stfl(data, 5) == "\u3042\u3044");
	}

	SECTION("doesn't count a width of STFL tag") {
		REQUIRE(utils::substr_with_width_stfl("ＡＢＣ<b>ＤＥ</b>Ｆ", 9) ==
			"ＡＢＣ<b>Ｄ");
		REQUIRE(utils::substr_with_width_stfl("<foobar>ＡＢＣ", 4) ==
			"<foobar>ＡＢ");
		REQUIRE(utils::substr_with_width_stfl("a<<xyz>>bcd", 3) ==
			"a<<xyz>>b"); // tag: "<<xyz>"
		REQUIRE(utils::substr_with_width_stfl("ＡＢＣ<b>ＤＥ", 10) ==
			"ＡＢＣ<b>ＤＥ");
		REQUIRE(utils::substr_with_width_stfl("a</>b</>c</>", 2) ==
			"a</>b</>");
	}

	SECTION("count a width of escaped less-than mark") {
		REQUIRE(utils::substr_with_width_stfl("<><><>", 2) == "<><>");
		REQUIRE(utils::substr_with_width_stfl("a<>b<>c", 3) == "a<>b");
	}

	SECTION("treat non-printable has zero width") {
		REQUIRE(utils::substr_with_width_stfl("\x01\x02"
				"abc",
				1) ==
			"\x01\x02"
			"a");
	}
}

TEST_CASE("getcwd() returns current directory of the process", "[utils]")
{
	SECTION("Returns non-empty string") {
		REQUIRE(utils::getcwd() != Filepath());
	}

	SECTION("Value depends on current directory") {
		const std::string maindir = utils::getcwd();
		// Other tests already rely on the presense of "data" directory
		// next to the executable, so it's okay to use that dependency
		// here, too
		const std::string subdir = "data";
		REQUIRE(0 == ::chdir(subdir.c_str()));
		const std::string datadir = utils::getcwd();
		REQUIRE(0 == ::chdir(".."));
		const std::string backdir = utils::getcwd();

		INFO("maindir = " << maindir);
		INFO("backdir = " << backdir);

		REQUIRE(maindir == backdir);
		REQUIRE_FALSE(maindir == datadir);

		// Datadir path starts with path to maindir
		REQUIRE(datadir.find(maindir) == 0);
		// Datadir path ends with "data" string
		REQUIRE(datadir.substr(datadir.length() - subdir.length()) ==
			subdir);
	}

	SECTION("Returns empty string if current directory doesn't exist") {
		// Create a temporary directory, change to it and delete it. This leads
		// getcwd to fail.
		test_helpers::TempDir tempdir;

		const std::string tempdir_path = tempdir.get_path();
		INFO("tempdir = " << tempdir_path);

		test_helpers::Chdir chdir(tempdir_path);

		REQUIRE(0 == ::rmdir(tempdir_path.c_str()));

		REQUIRE(Filepath() == utils::getcwd());
	}
}

TEST_CASE("read_text_file() returns file contents line by line", "[utils]")
{
	test_helpers::TempFile tempfile;

	SECTION("succesful if test file contains only valid UTF-8") {
		{
			std::ofstream f(tempfile.get_path());
			f << "lorem ipsum\ntest1\ntest2";
		}

		const auto result = utils::read_text_file(tempfile.get_path());
		REQUIRE(result);
		const auto content = result.value();
		REQUIRE(content.size() == 3);
		REQUIRE(content[0] == "lorem ipsum");
		REQUIRE(content[1] == "test1");
		REQUIRE(content[2] == "test2");
	}

	SECTION("fails with error message if file contains invalid UTF-8") {
		{
			std::ofstream f(tempfile.get_path());
			f << "test1\nt\xffst2"; // \xff is an invalid UTF-8 codepoint
		}
		std::vector<std::string> content;
		std::string error_message;

		const auto result = utils::read_text_file(tempfile.get_path());
		REQUIRE_FALSE(result);
		REQUIRE(result.error().message.size() > 0);
	}
}

TEST_CASE("strnaturalcmp() compares strings using natural numeric ordering",
	"[utils]")
{
	// Tests copied over from 3rd-party/alphanum.hpp
	REQUIRE(utils::strnaturalcmp("", "") == 0);
	REQUIRE(utils::strnaturalcmp("", "a") < 0);
	REQUIRE(utils::strnaturalcmp("a", "") > 0);
	REQUIRE(utils::strnaturalcmp("a", "a") == 0);
	REQUIRE(utils::strnaturalcmp("", "9") < 0);
	REQUIRE(utils::strnaturalcmp("9", "") > 0);
	REQUIRE(utils::strnaturalcmp("1", "1") == 0);
	REQUIRE(utils::strnaturalcmp("1", "2") < 0);
	REQUIRE(utils::strnaturalcmp("3", "2") > 0);
	REQUIRE(utils::strnaturalcmp("a1", "a1") == 0);
	REQUIRE(utils::strnaturalcmp("a1", "a2") < 0);
	REQUIRE(utils::strnaturalcmp("a2", "a1") > 0);
	REQUIRE(utils::strnaturalcmp("a1a2", "a1a3") < 0);
	REQUIRE(utils::strnaturalcmp("a1a2", "a1a0") > 0);
	REQUIRE(utils::strnaturalcmp("134", "122") > 0);
	REQUIRE(utils::strnaturalcmp("12a3", "12a3") == 0);
	REQUIRE(utils::strnaturalcmp("12a1", "12a0") > 0);
	REQUIRE(utils::strnaturalcmp("12a1", "12a2") < 0);
	REQUIRE(utils::strnaturalcmp("a", "aa") < 0);
	REQUIRE(utils::strnaturalcmp("aaa", "aa") > 0);
	REQUIRE(utils::strnaturalcmp("Alpha 2", "Alpha 2") == 0);
	REQUIRE(utils::strnaturalcmp("Alpha 2", "Alpha 2A") < 0);
	REQUIRE(utils::strnaturalcmp("Alpha 2 B", "Alpha 2") > 0);

	REQUIRE(utils::strnaturalcmp("aa10", "aa2") > 0);
}

TEST_CASE(
	"is_valid_podcast_type() returns true if supplied MIME type "
	"is audio, video, or a container",
	"[utils]")
{
	REQUIRE(utils::is_valid_podcast_type("audio/mpeg"));
	REQUIRE(utils::is_valid_podcast_type("audio/mp3"));
	REQUIRE(utils::is_valid_podcast_type("audio/x-mp3"));
	REQUIRE(utils::is_valid_podcast_type("audio/ogg"));
	REQUIRE(utils::is_valid_podcast_type("video/x-matroska"));
	REQUIRE(utils::is_valid_podcast_type("video/webm"));
	REQUIRE(utils::is_valid_podcast_type("application/ogg"));

	REQUIRE_FALSE(utils::is_valid_podcast_type("image/jpeg"));
	REQUIRE_FALSE(utils::is_valid_podcast_type("image/png"));
	REQUIRE_FALSE(utils::is_valid_podcast_type("text/plain"));
	REQUIRE_FALSE(utils::is_valid_podcast_type("application/zip"));
}

TEST_CASE("podcast_mime_to_link_type() returns HtmlRenderer's LinkType that "
	"corresponds to the given podcast MIME type",
	"[utils]")
{
	SECTION("Valid podcast MIME types") {
		const auto check = [](std::string mime, LinkType expected) {
			REQUIRE(utils::podcast_mime_to_link_type(mime) == expected);
		};

		check("audio/mpeg", LinkType::AUDIO);
		check("audio/mp3", LinkType::AUDIO);
		check("audio/x-mp3", LinkType::AUDIO);
		check("audio/ogg", LinkType::AUDIO);
		check("video/x-matroska", LinkType::VIDEO);
		check("video/webm", LinkType::VIDEO);
		check("application/ogg", LinkType::AUDIO);
	}

	SECTION("Sets `ok` to `false` if given MIME type is not a podcast type") {
		const auto check = [](std::string mime) {
			REQUIRE(utils::podcast_mime_to_link_type(mime) == std::nullopt);
		};

		check("image/jpeg");
		check("image/png");
		check("text/plain");
		check("application/zip");
	}
}

TEST_CASE("string_from_utf8_lossy() replaces invalid utf-8 code units with replacement characters",
	"[utils]")
{
	const std::string input_str = "abc" "\x81" "def";
	const std::vector<std::uint8_t> input(input_str.begin(), input_str.end());
	const auto output = utils::string_from_utf8_lossy(input);
	REQUIRE(output == "abc�def");
}

TEST_CASE("parse_rss_author_email() extracts name and email from string",
	"[utils]")
{
	const std::string input_str = "author@example.com (Example Author)";
	const std::vector<std::uint8_t> input(input_str.begin(), input_str.end());
	std::string name;
	std::string email;
	utils::parse_rss_author_email(input, name, email);
	REQUIRE(name == "Example Author");
	REQUIRE(email == "author@example.com");
}

TEST_CASE(
	"is_valid_color() returns false for things that aren't valid STFL "
	"colors",
	"[utils]")
{
	const std::vector<std::string> non_colors{
		"awesome", "list", "of", "things", "that", "aren't", "colors"};
	for (const auto& input : non_colors) {
		REQUIRE_FALSE(utils::is_valid_color(input));
	}
}

TEST_CASE(
	"is_query_url() returns true if given URL is a query URL, i.e. it "
	"starts with \"query:\" string",
	"[utils]")
{
	REQUIRE(utils::is_query_url("query:"));
	REQUIRE(utils::is_query_url("query: example"));
	REQUIRE_FALSE(utils::is_query_url("query"));
	REQUIRE_FALSE(utils::is_query_url("   query:"));
}

TEST_CASE(
	"is_filter_url() returns true if given URL is a filter URL, i.e. it "
	"starts with \"filter:\" string",
	"[utils]")
{
	REQUIRE(utils::is_filter_url("filter:"));
	REQUIRE(utils::is_filter_url("filter: example"));
	REQUIRE_FALSE(utils::is_filter_url("filter"));
	REQUIRE_FALSE(utils::is_filter_url("   filter:"));
}

TEST_CASE(
	"is_exec_url() returns true if given URL is a exec URL, i.e. it "
	"starts with \"exec:\" string",
	"[utils]")
{
	REQUIRE(utils::is_exec_url("exec:"));
	REQUIRE(utils::is_exec_url("exec: example"));
	REQUIRE_FALSE(utils::is_exec_url("exec"));
	REQUIRE_FALSE(utils::is_exec_url("   exec:"));
}

TEST_CASE(
	"is_special_url() return true if given URL is a query, filter or exec "
	"URL, i.e. it "
	"starts with \"query:\", \"filter:\" or \"exec:\" string",
	"[utils]")
{
	REQUIRE(utils::is_special_url("query:"));
	REQUIRE(utils::is_special_url("query: example"));
	REQUIRE_FALSE(utils::is_special_url("query"));
	REQUIRE_FALSE(utils::is_special_url("   query:"));

	REQUIRE(utils::is_special_url("filter:"));
	REQUIRE(utils::is_special_url("filter: example"));
	REQUIRE_FALSE(utils::is_special_url("filter"));
	REQUIRE_FALSE(utils::is_special_url("   filter:"));

	REQUIRE(utils::is_special_url("exec:"));
	REQUIRE(utils::is_special_url("exec: example"));
	REQUIRE_FALSE(utils::is_special_url("exec"));
	REQUIRE_FALSE(utils::is_special_url("   exec:"));
}

TEST_CASE(
	"get_default_browser() returns BROWSER environment variable or "
	"\"lynx\" if the variable is not set",
	"[utils]")
{
	test_helpers::EnvVar browserEnv("BROWSER");

	// If BROWSER is not set, default browser is lynx(1)
	browserEnv.unset();
	REQUIRE(utils::get_default_browser() == Filepath::from_locale_string("lynx"));

	browserEnv.set("firefox");
	REQUIRE(utils::get_default_browser() == Filepath::from_locale_string("firefox"));

	browserEnv.set("opera");
	REQUIRE(utils::get_default_browser() == Filepath::from_locale_string("opera"));
}

TEST_CASE(
	"get_basename() returns basename from a URL if available, if not"
	" then an empty string",
	"[utils]")
{
	SECTION("return basename in the presence of GET parameters") {
		REQUIRE(utils::get_basename("https://example.org/path/to/file.mp3?param=value#fragment")
			== "file.mp3");
		REQUIRE(utils::get_basename("https://example.org/file.mp3") == "file.mp3");
	}

	SECTION("return empty string when basename is unavailable") {
		REQUIRE(utils::get_basename("https://example.org/?param=value#fragment")
			== "");
		REQUIRE(utils::get_basename("https://example.org/path/to/?param=value#fragment")
			== "");
	}
}

TEST_CASE(
	"get_auth_method() returns enumerated constant "
	"on defined values and undefined values",
	"[utils]")
{
	REQUIRE(utils::get_auth_method("any") == CURLAUTH_ANY);
	REQUIRE(utils::get_auth_method("ntlm") == CURLAUTH_NTLM);
	REQUIRE(utils::get_auth_method("basic") == CURLAUTH_BASIC);
	REQUIRE(utils::get_auth_method("digest") == CURLAUTH_DIGEST);
	REQUIRE(utils::get_auth_method("digest_ie") == CURLAUTH_DIGEST_IE);
	REQUIRE(utils::get_auth_method("gssnegotiate") == CURLAUTH_GSSNEGOTIATE);
	REQUIRE(utils::get_auth_method("anysafe") == CURLAUTH_ANYSAFE);

	REQUIRE(utils::get_auth_method("") == CURLAUTH_ANY);
	REQUIRE(utils::get_auth_method("test") == CURLAUTH_ANY);
}

TEST_CASE(
	"get_proxy_type() returns enumerated constant "
	"on defined values and undefined values",
	"[utils]")
{
	REQUIRE(utils::get_proxy_type("http") == CURLPROXY_HTTP);
	REQUIRE(utils::get_proxy_type("socks4") == CURLPROXY_SOCKS4);
	REQUIRE(utils::get_proxy_type("socks5") == CURLPROXY_SOCKS5);
	REQUIRE(utils::get_proxy_type("socks5h") == CURLPROXY_SOCKS5_HOSTNAME);

	REQUIRE(utils::get_proxy_type("") == CURLPROXY_HTTP);
	REQUIRE(utils::get_proxy_type("test") == CURLPROXY_HTTP);
}

TEST_CASE("is_valid_attribute returns true if given string is an STFL attribute",
	"[utils]")
{
	const std::vector<std::string> invalid = {
		"foo",
		"bar",
		"baz",
		"quux"
	};
	for (const auto& attr : invalid) {
		REQUIRE_FALSE(utils::is_valid_attribute(attr));
	}

	const std::vector<std::string> valid = {
		"standout",
		"underline",
		"reverse",
		"blink",
		"dim",
		"bold",
		"protect",
		"invis",
		"default"
	};
	for (const auto& attr : valid) {
		REQUIRE(utils::is_valid_attribute(attr));
	}
}

TEST_CASE("unescape_url() takes a percent-encoded string and returns the string "
	"with a precent escaped string",
	"[utils]")
{
	REQUIRE(utils::unescape_url("foo%20bar") == "foo bar");
	REQUIRE(utils::unescape_url(
			"%21%23%24%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D") ==
		"!#$&'()*+,/:;=?@[]");
	REQUIRE(utils::unescape_url("%00") == "");

}

TEST_CASE("preserve_quotes() preserves single quotes and double quotes, if any",
	"[utils]")
{
	REQUIRE(utils::preserve_quotes("") == "''");
	REQUIRE(utils::preserve_quotes("sss") == "'sss'");

	SECTION("Preserves double quotes") {
		REQUIRE(utils::preserve_quotes("\"sss\"") == "'\"sss\"'");
		REQUIRE(utils::preserve_quotes("sss\"") == "'sss\"'");
		REQUIRE(utils::preserve_quotes("\"sss") == "'\"sss'");
		REQUIRE(utils::preserve_quotes("ss\"s") == "'ss\"s'");
		REQUIRE(utils::preserve_quotes("ss\"\"s") == "'ss\"\"s'");
	}

	SECTION("Preserves single quotes") {
		REQUIRE(utils::preserve_quotes("'sss'") == "\\\''sss'\\\'");
		REQUIRE(utils::preserve_quotes("'sss") == "\\\''sss'");
		REQUIRE(utils::preserve_quotes("sss'") == "'sss'\\\'");
		REQUIRE(utils::preserve_quotes("ss's") == "'ss'\\\''s'");
		REQUIRE(utils::preserve_quotes("ss''s") == "'ss'\\\'\\\''s'");
	}

	SECTION("Preserves a combination of single and double quotes") {
		REQUIRE(utils::preserve_quotes("'\"sss\"'") == "\\\''\"sss\"'\\\'");
		REQUIRE(utils::preserve_quotes("\"'sss'\"") == "'\"'\\\''sss'\\\''\"'");
		REQUIRE(utils::preserve_quotes("\"sss'") == "'\"sss'\\\'");
		REQUIRE(utils::preserve_quotes("'sss\"") == "\\\''sss\"'");
		REQUIRE(utils::preserve_quotes("ss\"'s") == "'ss\"'\\\''s'");
	}
}

TEST_CASE("mkdir_parents() creates all paths components and returns 0 if "
	"the path now exists",
	"[utils]")
{
	test_helpers::TempDir tmp;

	const auto require_return_zero = [](const std::string& path) {
		REQUIRE(utils::mkdir_parents(path, 0700) == 0);
		REQUIRE(::access(path.c_str(), R_OK | X_OK) == 0);
	};

	SECTION("Simple test on temporary dir itself") {
		const auto path = tmp.get_path();
		INFO("Path is " << path);
		require_return_zero(path);
	}

	SECTION("Zero intermediate directories") {
		const auto path = tmp.get_path().join(std::to_string(rand()));
		INFO("Path is " << path);

		SECTION("Target doesn't yet exist") {
			require_return_zero(path);
		}

		SECTION("Target already exists") {
			REQUIRE(test_helpers::mkdir(path, 0700) == 0);
			require_return_zero(path);
		}
	}

	SECTION("One intermediate directory") {
		const auto intermediate_path = tmp.get_path().join(std::to_string(rand()));
		const auto path = intermediate_path.join(std::to_string(rand()));
		INFO("Path is " << path);

		SECTION("Which doesn't exist") {
			require_return_zero(path);
		}

		SECTION("Which exists") {
			REQUIRE(test_helpers::mkdir(intermediate_path, 0700) == 0);

			SECTION("Target doesn't exist") {
				require_return_zero(path);
			}

			SECTION("Target exists") {
				REQUIRE(test_helpers::mkdir(path, 0700) == 0);
				require_return_zero(path);
			}
		}
	}

	SECTION("Two intermediate directories") {
		const auto intermediate_path1 = tmp.get_path().join(std::to_string(rand()));
		const auto intermediate_path2 = intermediate_path1.join(std::to_string(rand()));
		const auto path = intermediate_path2.join(std::to_string(rand()));
		INFO("Path is " << path);

		SECTION("Which don't exist") {
			require_return_zero(path);
		}

		SECTION("First one exists") {
			REQUIRE(test_helpers::mkdir(intermediate_path1, 0700) == 0);

			SECTION("Second one exists") {
				REQUIRE(test_helpers::mkdir(intermediate_path2, 0700) == 0);

				SECTION("Target exists") {
					REQUIRE(test_helpers::mkdir(path, 0700) == 0);
					require_return_zero(path);
				}

				SECTION("Target doesn't exist") {
					require_return_zero(path);
				}
			}

			SECTION("Second one doesn't exist") {
				require_return_zero(path);
			}
		}
	}
}

TEST_CASE("mkdir_parents() doesn't care if the path ends in a slash or not",
	"[utils]")
{
	test_helpers::TempDir tmp;

	const auto path = tmp.get_path().join(std::to_string(rand()));

	const auto check = [](const Filepath& path) {
		REQUIRE(utils::mkdir_parents(path, 0700) == 0);
		const auto path_str = path.to_locale_string();
		REQUIRE(::access(path_str.c_str(), R_OK | X_OK) == 0);
	};

	auto path_as_string = path.to_locale_string();
	SECTION("Path doesn't end in slash => directory created") {
		REQUIRE(path_as_string.back() != '/');
		check(path);
	}

	SECTION("Path ends in slash => directory created") {
		REQUIRE(path_as_string.back() != '/');
		path_as_string.push_back('/');
		check(Filepath::from_locale_string(path_as_string));
	}
}

TEST_CASE("utf8_to_locale() converts text from UTF-8 to the encoding specified "
	"by locale in LC_CTYPE class",
	"[utils]")
{
	test_helpers::LcCtypeEnvVar lc_ctype;
	const auto set_locale = [&lc_ctype](std::string new_locale) -> bool {
		if (::setlocale(LC_CTYPE, new_locale.c_str()) == nullptr)
		{
			WARN("Couldn't set locale " + new_locale + "; test skipped.");
			return false;
		}
		lc_ctype.set(new_locale);
		return true;
	};

	SECTION("UTF-8") {
		if (!set_locale("en_US.UTF-8")) {
			return;
		}

		REQUIRE(utils::utf8_to_locale("") == "");

		// "Просто проверяю", "Just testing" in Russian.
		const std::string text("\xd0\x9f\xd1\x80\xd0\xbe\xd1\x81\xd1\x82\xd0"
			"\xbe\x20\xd0\xbf\xd1\x80\xd0\xbe\xd0\xb2\xd0\xb5\xd1\x80\xd1\x8f\xd1\x8e");
		REQUIRE(utils::utf8_to_locale(text) == text);
	}

	SECTION("KOI8-R") {
		if (!set_locale("ru_RU.KOI8-R")) {
			return;
		}

		REQUIRE(utils::utf8_to_locale("") == "");

		// "ещё один тест", "another test" in Russian.
		const std::string input("\xd0\xb5\xd1\x89\xd1\x91\x20\xd0\xbe\xd0\xb4"
			"\xd0\xb8\xd0\xbd\x20\xd1\x82\xd0\xb5\xd1\x81\xd1\x82");
		const std::string expected("\xc5\xdd\xa3\x20\xcf\xc4\xc9\xce\x20\xd4"
			"\xc5\xd3\xd4");
		REQUIRE(utils::utf8_to_locale(input) == expected);
	}

	SECTION("CP1251") {
		if (!set_locale("ru_RU.CP1251")) {
			return;
		}

		REQUIRE(utils::utf8_to_locale("") == "");

		// "Приветствую!", "Greetings!" in Russian.
		const std::string input("\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1"
			"\x82\xd1\x81\xd1\x82\xd0\xb2\xd1\x83\xd1\x8e\x21");
		const std::string expected("\xcf\xf0\xe8\xe2\xe5\xf2\xf1\xf2\xe2\xf3"
			"\xfe\x21");
		REQUIRE(utils::utf8_to_locale(input) == expected);
	}
}

TEST_CASE("utf8_to_locale() transliterates characters unsupported by the locale's encoding",
	"[utils]")
{
	test_helpers::LcCtypeEnvVar lc_ctype;
	const auto set_locale = [&lc_ctype](std::string new_locale) -> bool {
		if (::setlocale(LC_CTYPE, new_locale.c_str()) == nullptr)
		{
			WARN("Couldn't set locale " + new_locale + "; test skipped.");
			return false;
		}
		lc_ctype.set(new_locale);
		return true;
	};

	// Tests below don't spell out the expected result because different
	// platforms might follow different transliteration rules. The best we can
	// do is check that 1) the output is non-empty, i.e. there was no error in
	// conversion; 2) the output is not the same as the input, i.e. the
	// conversion did something.

	SECTION("KOI8-R doesn't support Ukrainian characters") {
		if (!set_locale("ru_RU.KOI8-R")) {
			return;
		}

		// "Пісня", "Song" in Ukrainian.
		const std::string input("\xd0\x9f\xd1\x96\xd1\x81\xd0\xbd\xd1\x8f");

		const auto result = utils::utf8_to_locale(input);
		REQUIRE(result != "");
		REQUIRE(result != input);
	}

	SECTION("CP1251") {
		if (!set_locale("ru_RU.CP1251")) {
			return;
		}

		// "日本", "Japan" in Japanese
		const std::string input("\xe6\x97\xa5\xe6\x9c\xac");

		const auto result = utils::utf8_to_locale(input);
		REQUIRE(result != "");
		REQUIRE(result != input);
	}
}

TEST_CASE("locale_to_utf8() converts text from the encoding specified by locale "
	"in LC_CTYPE class to UTF-8",
	"[utils]")
{
	test_helpers::LcCtypeEnvVar lc_ctype;
	const auto set_locale = [&lc_ctype](std::string new_locale) -> bool {
		if (::setlocale(LC_CTYPE, new_locale.c_str()) == nullptr)
		{
			WARN("Couldn't set locale " + new_locale + "; test skipped.");
			return false;
		}
		lc_ctype.set(new_locale);
		return true;
	};

	SECTION("UTF-8") {
		if (!set_locale("en_US.UTF-8")) {
			return;
		}

		REQUIRE(utils::locale_to_utf8("") == "");

		// "Newsboat мне нравится", "I like Newsboat" in Russian.
		const std::string text("\x4e\x65\x77\x73\x62\x6f\x61\x74\x20\xd0\xbc"
			"\xd0\xbd\xd0\xb5\x20\xd0\xbd\xd1\x80\xd0\xb0\xd0\xb2\xd0\xb8"
			"\xd1\x82\xd1\x81\xd1\x8f");
		REQUIRE(utils::locale_to_utf8(text) == text);
	}

	SECTION("KOI8-R") {
		if (!set_locale("ru_RU.KOI8-R")) {
			return;
		}

		REQUIRE(utils::locale_to_utf8("") == "");

		// "великолепная проверка", "excellent check" in Russian.
		const std::string input("\xd7\xc5\xcc\xc9\xcb\xcf\xcc\xc5\xd0\xce\xc1"
			"\xd1\x20\xd0\xd2\xcf\xd7\xc5\xd2\xcb\xc1");
		const std::string expected("\xd0\xb2\xd0\xb5\xd0\xbb\xd0\xb8\xd0\xba\xd0"
			"\xbe\xd0\xbb\xd0\xb5\xd0\xbf\xd0\xbd\xd0\xb0\xd1\x8f\x20\xd0"
			"\xbf\xd1\x80\xd0\xbe\xd0\xb2\xd0\xb5\xd1\x80\xd0\xba\xd0\xb0");
		REQUIRE(utils::locale_to_utf8(input) == expected);
	}

	SECTION("CP1251") {
		if (!set_locale("ru_RU.CP1251")) {
			return;
		}

		REQUIRE(utils::locale_to_utf8("") == "");

		// "Все тесты зелёные!", "All tests green!" in Russian.
		const std::string input("\xc2\xf1\xe5\x20\xf2\xe5\xf1\xf2\xfb\x20\xe7"
			"\xe5\xeb\xb8\xed\xfb\xe5\x21");
		const std::string expected("\xd0\x92\xd1\x81\xd0\xb5\x20\xd1\x82\xd0\xb5"
			"\xd1\x81\xd1\x82\xd1\x8b\x20\xd0\xb7\xd0\xb5\xd0\xbb\xd1\x91\xd0"
			"\xbd\xd1\x8b\xd0\xb5\x21");
		REQUIRE(utils::locale_to_utf8(input) == expected);
	}
}

TEST_CASE("translit() returns the value of `tocode`, maybe with \"//TRANSLIT\" appended",
	"[utils]")
{
	// The behaviour of translit() is inherently platform-dependent: some
	// platforms support transliteration for some encodings, while others
	// support it for different encodings, or don't support it at all. This
	// tests checks just the bare minimum: the function doesn't crash, doesn't
	// throw exceptions, and returns something resembling a correct value.

	const auto check = [&](std::string fromcode, std::string tocode) {
		DYNAMIC_SECTION(fromcode << " → " << tocode) {
			const auto expected1(tocode);
			const auto expected2(tocode + "//TRANSLIT");

			const auto actual = utils::translit(tocode, fromcode);

			INFO("Expected " << expected1);
			INFO("      or " << expected2);
			INFO("But got: " << actual);

			// Extra parentheses required by Catch2 -- otherwise it doesn't let us use operator||.
			REQUIRE((actual == expected1 || actual == expected2));
		}
	};

	check("ISO-8859-1", "UTF-8");
	check("KOI8-R", "UTF-8");
	check("UTF-16", "UTF-8");
	check("UTF-8", "UTF-8");
	check("UTF-8", "UTF-16");
	check("UTF-8", "KOI8-R");
	check("UTF-8", "ISO-8859-1");
}

TEST_CASE("translit() always returns the same value for the same inputs",
	"[utils]")
{
	const std::vector<std::string> encodings {"UTF-8", "UTF-16", "KOI8-R", "ISO-8859-1"};
	// (fromcode, tocode) -> result
	std::map<std::pair<std::string, std::string>, std::string> results;

	for (const auto& fromcode : encodings) {
		for (const auto& tocode : encodings) {
			results[std::make_pair(fromcode, tocode)] = utils::translit(tocode, fromcode);
		}
	}

	for (const auto& fromcode : encodings) {
		for (const auto& tocode : encodings) {
			INFO(fromcode << " → " << tocode);
			REQUIRE(utils::translit(tocode, fromcode) == results.at(std::make_pair(fromcode, tocode)));
		}
	}
}

void verify_convert_text(const std::vector<unsigned char>& input,
	const std::string& tocode, const std::string& fromcode,
	const std::vector<unsigned char>& expected_output)
{
	const std::string input_str = std::string(reinterpret_cast<const char*>(input.data()),
			input.size());
	const std::string expected_str = std::string(reinterpret_cast<const char*>
			(expected_output.data()), expected_output.size());

	REQUIRE(utils::convert_text(input_str, tocode, fromcode) == expected_str);
}

TEST_CASE("convert_text() returns input string if fromcode and tocode are the same",
	"[utils]")
{
	std::vector<unsigned char> input = {
		// \x81 is not valid UTF-8
		0x81, 0x13, 0x41,
	};

	std::vector<unsigned char> expected = {
		0x3f, 0x13, 0x41,
	};

	verify_convert_text(input, "UTF-8", "UTF-8", expected);
}

TEST_CASE("convert_text() replaces incomplete multibyte sequences with a question mark: utf8 to utf16le",
	"[utils]")
{
	std::vector<unsigned char> input = {
		// "ой", "oops" in Russian, but the last byte is missing
		0xd0, 0xbe, 0xd0,
	};

	std::vector<unsigned char> expected = {
		0x3e, 0x04, 0x3f, 0x00,
	};

	verify_convert_text(input, "UTF-16LE", "UTF-8", expected);
}

TEST_CASE("convert_text() replaces incomplete multibyte sequences with a question mark: utf16le to utf8",
	"[utils]")
{
	SECTION("input includes zero byte") {
		std::vector<unsigned char> input = {
			// "hi", but the last byte is missing
			0x68, 0x00, 0x69,
		};

		std::vector<unsigned char> expected = {
			0x68, 0x3f,
		};

		verify_convert_text(input, "UTF-8", "UTF-16LE", expected);
	}

	SECTION("input does not include zero byte") {
		std::vector<unsigned char> input = {
			// "эй", "hey" in Russian, but the last byte is missing
			0x4d, 0x04, 0x39,
		};

		std::vector<unsigned char> expected = {
			0xd1, 0x8d, 0x3f,
		};

		verify_convert_text(input, "UTF-8", "UTF-16LE", expected);
	}
}

TEST_CASE("convert_text() replaces invalid multibyte sequences with a question mark: utf8 to utf16le",
	"[utils]")
{
	std::vector<unsigned char> input = {
		// "日本", "Japan", but the third byte of the first character (0xa5) is
		// missing, making the whole first character an illegal sequence.
		0xe6, 0x97, 0xe6, 0x9c, 0xac,
	};

	std::vector<unsigned char> expected = {
		0x3f, 0x00, 0x3f, 0x00, 0x2c, 0x67,
	};

	verify_convert_text(input, "UTF-16LE", "UTF-8", expected);
}

TEST_CASE("convert_text() replaces invalid multibyte sequences with a question mark: utf16le to utf8",
	"[utils]")
{
	std::vector<unsigned char> input = {
		// The first two bytes here are part of a surrogate pair, i.e. they
		// imply that the next two bytes encode additional info. However, the
		// next two bytes are an ordinary character. This breaks the decoding
		// process, so some things get turned into a question mark while others
		// are decoded incorrectly.
		0x01, 0xd8, 0xd7, 0x03,
	};

	std::vector<unsigned char> expected = {
		0x3f, 0xed, 0x9f, 0x98, 0x3f,
	};

	verify_convert_text(input, "UTF-8", "UTF-16LE", expected);
}

TEST_CASE("convert_text() converts text between encodings: utf8 to utf16le", "[utils]")
{
	std::vector<unsigned char> input = {
		// "Тестирую", "Testing" in Russian.
		0xd0, 0xa2, 0xd0, 0xb5, 0xd1, 0x81, 0xd1, 0x82, 0xd0, 0xb8, 0xd1, 0x80, 0xd1, 0x83,
		0xd1, 0x8e,
	};

	std::vector<unsigned char> expected = {
		0x22, 0x04, 0x35, 0x04, 0x41, 0x04, 0x42, 0x04, 0x38, 0x04, 0x40, 0x04, 0x43, 0x04,
		0x4e, 0x04,
	};

	verify_convert_text(input, "UTF-16LE", "UTF-8", expected);
}

TEST_CASE("convert_text() converts text between encodings: utf8 to koi8r", "[utils]")
{
	std::vector<unsigned char> input = {
		// "Проверка", "Check" in Russian.
		0xd0, 0x9f, 0xd1, 0x80, 0xd0, 0xbe, 0xd0, 0xb2, 0xd0, 0xb5, 0xd1, 0x80, 0xd0, 0xba,
		0xd0, 0xb0,
	};

	std::vector<unsigned char> expected = {
		0xf0, 0xd2, 0xcf, 0xd7, 0xc5, 0xd2, 0xcb, 0xc1,
	};

	verify_convert_text(input, "KOI8-R", "UTF-8", expected);
}

TEST_CASE("convert_text() converts text between encodings: utf8 to ISO-8859-1", "[utils]")
{
	// Some symbols in the result will be transliterated.

	std::vector<unsigned char> input = {
		// "вау °±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃ": a mix of Cyrillic (unsupported by
		// ISO-8859-1) and ISO-8859-1 characters.
		0xd0, 0xb2, 0xd0, 0xb0, 0xd1, 0x83, 0x20, 0xc2, 0xb0, 0xc2, 0xb1, 0xc2, 0xb2, 0xc2,
		0xb3, 0xc2, 0xb4, 0xc2, 0xb5, 0xc2, 0xb6, 0xc2, 0xb7, 0xc2, 0xb8, 0xc2, 0xb9, 0xc2,
		0xba, 0xc2, 0xbb, 0xc2, 0xbc, 0xc2, 0xbd, 0xc2, 0xbe, 0xc2, 0xbf, 0xc3, 0x80, 0xc3,
		0x81, 0xc3, 0x82, 0xc3, 0x83,
	};

	const std::string input_str = std::string(reinterpret_cast<const char*>(input.data()),
			input.size());

	const auto result = utils::convert_text(input_str, "ISO-8859-1", "UTF-8");

	// We can't spell out an expected result because different platforms
	// might follow different transliteration rules.
	REQUIRE(result != "");
	REQUIRE(result != input_str);
}

TEST_CASE("convert_text() converts text between encodings: utf16le to utf8", "[utils]")
{
	std::vector<unsigned char> input = {
		// "Успех", "Success" in Russian.
		0xff, 0xfe, 0x23, 0x04, 0x41, 0x04, 0x3f, 0x04, 0x35, 0x04, 0x45, 0x04,
	};

	std::vector<unsigned char> expected = {
		0xef, 0xbb, 0xbf, 0xd0, 0xa3, 0xd1, 0x81, 0xd0, 0xbf, 0xd0, 0xb5, 0xd1, 0x85,
	};

	verify_convert_text(input, "UTF-8", "UTF-16LE", expected);
}

TEST_CASE("convert_text() converts text between encodings: koi8r to utf8", "[utils]")
{
	std::vector<unsigned char> input = {
		// "История", "History" in Russian.
		0xe9, 0xd3, 0xd4, 0xcf, 0xd2, 0xc9, 0xd1,
	};

	std::vector<unsigned char> expected = {
		0xd0, 0x98, 0xd1, 0x81, 0xd1, 0x82, 0xd0, 0xbe, 0xd1, 0x80, 0xd0, 0xb8, 0xd1, 0x8f,
	};

	verify_convert_text(input, "UTF-8", "KOI8-R", expected);
}

TEST_CASE("convert_text() converts text between encodings: ISO-8859-1 to utf8", "[utils]")
{
	std::vector<unsigned char> input = {
		// "ÄÅÆÇÈÉÊËÌÍÎÏ": some umlauts and Latin letters.
		0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	};

	std::vector<unsigned char> expected = {
		0xc3, 0x84, 0xc3, 0x85, 0xc3, 0x86, 0xc3, 0x87, 0xc3, 0x88, 0xc3, 0x89, 0xc3, 0x8a,
		0xc3, 0x8b, 0xc3, 0x8c, 0xc3, 0x8d, 0xc3, 0x8e, 0xc3, 0x8f,
	};

	verify_convert_text(input, "UTF-8", "ISO-8859-1", expected);
}

#include "utils.h"

#include <unistd.h> // chdir()

#include "3rd-party/catch.hpp"

using namespace newsboat;

TEST_CASE("tokenize() extracts tokens separated by given delimiters", "[utils]")
{
	std::vector<std::string> tokens;

	SECTION("Default delimiters")
	{
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

	SECTION("Splitting by tabulation characters")
	{
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

	SECTION("Default delimiters include space and tab")
	{
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

	SECTION("Comma-separated values containing spaces and tabs")
	{
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

	SECTION("Default delimiters include spaces, newlines and tabs")
	{
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
}

TEST_CASE(
	"tokenize_quoted() implicitly closes quotes at the end of the string",
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
}

TEST_CASE(
	"tokenize_quoted() interprets \"\\\\\" as escaped backslash and puts "
	"single backslash in output",
	"[utils]")
{
	std::vector<std::string> tokens;

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
}

TEST_CASE("tokenize_quoted() doesn't un-escape escaped backticks", "[utils]")
{
	std::vector<std::string> tokens;

	tokens = utils::tokenize_quoted("asdf \"\\`foobar `bla`\\`\"");

	REQUIRE(tokens.size() == 2);
	REQUIRE(tokens[0] == "asdf");
	REQUIRE(tokens[1] == "\\`foobar `bla`\\`");
}

TEST_CASE(
	"consolidate_whitespace replaces multiple consecutive"
	"whitespace with a single space",
	"[utils]")
{
	REQUIRE(utils::consolidate_whitespace("LoremIpsum") == "LoremIpsum");
	REQUIRE(utils::consolidate_whitespace("Lorem Ipsum") == "Lorem Ipsum");
	REQUIRE(utils::consolidate_whitespace(" Lorem \t\tIpsum \t ")
		== " Lorem Ipsum ");
	REQUIRE(utils::consolidate_whitespace(" Lorem \r\n\r\n\tIpsum")
		== " Lorem Ipsum");

	REQUIRE(utils::consolidate_whitespace("") == "");
}

TEST_CASE("consolidate_whitespace preserves leading whitespace", "[utils]")
{
	REQUIRE(utils::consolidate_whitespace("    Lorem \t\tIpsum \t ")
		== "    Lorem Ipsum ");
	REQUIRE(utils::consolidate_whitespace("   Lorem \r\n\r\n\tIpsum")
		== "   Lorem Ipsum");
}

TEST_CASE("get_command_output()", "[utils]")
{
	REQUIRE(utils::get_command_output("ls /dev/null") == "/dev/null\n");
	REQUIRE_NOTHROW(utils::get_command_output(
		"a-program-that-is-guaranteed-to-not-exists"));
	REQUIRE(utils::get_command_output(
			"a-program-that-is-guaranteed-to-not-exists")
		== "");
}

TEST_CASE("run_program()", "[utils]")
{
	char* argv[4];
	char cat[] = "cat";
	argv[0] = cat;
	argv[1] = nullptr;
	REQUIRE(utils::run_program(argv, "this is a multine-line\ntest string")
		== "this is a multine-line\ntest string");

	char echo[] = "echo";
	char dashn[] = "-n";
	char helloworld[] = "hello world";
	argv[0] = echo;
	argv[1] = dashn;
	argv[2] = helloworld;
	argv[3] = nullptr;
	REQUIRE(utils::run_program(argv, "") == "hello world");
}

TEST_CASE("replace_all()", "[utils]")
{
	REQUIRE(utils::replace_all("aaa", "a", "b") == "bbb");
	REQUIRE(utils::replace_all("aaa", "aa", "ba") == "baa");
	REQUIRE(utils::replace_all("aaaaaa", "aa", "ba") == "bababa");
	REQUIRE(utils::replace_all("", "a", "b") == "");
	REQUIRE(utils::replace_all("aaaa", "b", "c") == "aaaa");
	REQUIRE(utils::replace_all("this is a normal test text", " t", " T")
		== "this is a normal Test Text");
	REQUIRE(utils::replace_all("o o o", "o", "<o>") == "<o> <o> <o>");
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

	SECTION("[0, 9] into 2")
	{
		partitions = utils::partition_indexes(0, 9, 2);
		REQUIRE(partitions.size() == 2);
		REQUIRE(partitions[0].first == 0);
		REQUIRE(partitions[0].second == 4);
		REQUIRE(partitions[1].first == 5);
		REQUIRE(partitions[1].second == 9);
	}

	SECTION("[0, 10] into 3")
	{
		partitions = utils::partition_indexes(0, 10, 3);
		REQUIRE(partitions.size() == 3);
		REQUIRE(partitions[0].first == 0);
		REQUIRE(partitions[0].second == 2);
		REQUIRE(partitions[1].first == 3);
		REQUIRE(partitions[1].second == 5);
		REQUIRE(partitions[2].first == 6);
		REQUIRE(partitions[2].second == 10);
	}

	SECTION("[0, 11] into 3")
	{
		partitions = utils::partition_indexes(0, 11, 3);
		REQUIRE(partitions.size() == 3);
		REQUIRE(partitions[0].first == 0);
		REQUIRE(partitions[0].second == 3);
		REQUIRE(partitions[1].first == 4);
		REQUIRE(partitions[1].second == 7);
		REQUIRE(partitions[2].first == 8);
		REQUIRE(partitions[2].second == 11);
	}

	SECTION("[0, 199] into 200")
	{
		partitions = utils::partition_indexes(0, 199, 200);
		REQUIRE(partitions.size() == 200);
	}

	SECTION("[0, 103] into 1")
	{
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

	REQUIRE(utils::censor_url("http://newsbeuter.org/")
		== "http://newsbeuter.org/");
	REQUIRE(utils::censor_url("https://newsbeuter.org/")
		== "https://newsbeuter.org/");

	REQUIRE(utils::censor_url("http://@newsbeuter.org/")
		== "http://*:*@newsbeuter.org/");
	REQUIRE(utils::censor_url("https://@newsbeuter.org/")
		== "https://*:*@newsbeuter.org/");

	REQUIRE(utils::censor_url("http://foo:bar@newsbeuter.org/")
		== "http://*:*@newsbeuter.org/");
	REQUIRE(utils::censor_url("https://foo:bar@newsbeuter.org/")
		== "https://*:*@newsbeuter.org/");

	REQUIRE(utils::censor_url("http://aschas@newsbeuter.org/")
		== "http://*:*@newsbeuter.org/");
	REQUIRE(utils::censor_url("https://aschas@newsbeuter.org/")
		== "https://*:*@newsbeuter.org/");

	REQUIRE(utils::censor_url("xxx://aschas@newsbeuter.org/")
		== "xxx://*:*@newsbeuter.org/");

	REQUIRE(utils::censor_url("http://foobar") == "http://foobar");
	REQUIRE(utils::censor_url("https://foobar") == "https://foobar");

	REQUIRE(utils::censor_url("http://aschas@host") == "http://*:*@host");
	REQUIRE(utils::censor_url("https://aschas@host") == "https://*:*@host");

	REQUIRE(utils::censor_url("query:name:age between 1:10")
		== "query:name:age between 1:10");
}

TEST_CASE("absolute_url()", "[utils]")
{
	REQUIRE(utils::absolute_url("http://foobar/hello/crook/", "bar.html")
		== "http://foobar/hello/crook/bar.html");
	REQUIRE(utils::absolute_url("https://foobar/foo/", "/bar.html")
		== "https://foobar/bar.html");
	REQUIRE(utils::absolute_url(
			"https://foobar/foo/", "http://quux/bar.html")
		== "http://quux/bar.html");
	REQUIRE(utils::absolute_url("http://foobar", "bla.html")
		== "http://foobar/bla.html");
	REQUIRE(utils::absolute_url("http://test:test@foobar:33", "bla2.html")
		== "http://test:test@foobar:33/bla2.html");
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

	REQUIRE(utils::strwidth(utils::wstr2str(L"\uF91F")) == 2);
}

TEST_CASE("join()", "[utils]")
{
	std::vector<std::string> str;
	REQUIRE(utils::join(str, "") == "");
	REQUIRE(utils::join(str, "-") == "");

	SECTION("Join of one element")
	{
		str.push_back("foobar");
		REQUIRE(utils::join(str, "") == "foobar");
		REQUIRE(utils::join(str, "-") == "foobar");

		SECTION("Join of two elements")
		{
			str.push_back("quux");
			REQUIRE(utils::join(str, "") == "foobarquux");
			REQUIRE(utils::join(str, "-") == "foobar-quux");
		}
	}
}

TEST_CASE("trim()", "[utils]")
{
	std::string str = "  xxx\r\n";
	utils::trim(str);
	REQUIRE(str == "xxx");

	str = "\n\n abc  foobar\n";
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

TEST_CASE("utils::make_title extracts possible title from URL")
{
	SECTION("Uses last part of URL as title")
	{
		auto input = "http://example.com/Item";
		REQUIRE(utils::make_title(input) == "Item");
	}

	SECTION("Replaces dashes and underscores with spaces")
	{
		std::string input;

		SECTION("Dashes")
		{
			input = "http://example.com/This-is-the-title";
		}

		SECTION("Underscores")
		{
			input = "http://example.com/This_is_the_title";
		}

		SECTION("Mix of dashes and underscores")
		{
			input = "http://example.com/This_is-the_title";
		}

		SECTION("Eliminate .php extension")
		{
			input = "http://example.com/This_is-the_title.php";
		}

		SECTION("Eliminate .html extension")
		{
			input = "http://example.com/This_is-the_title.html";
		}

		SECTION("Eliminate .htm extension")
		{
			input = "http://example.com/This_is-the_title.htm";
		}

		SECTION("Eliminate .aspx extension")
		{
			input = "http://example.com/This_is-the_title.aspx";
		}

		REQUIRE(utils::make_title(input) == "This is the title");
	}

	SECTION("Capitalizes first letter of extracted title")
	{
		auto input = "http://example.com/this-is-the-title";
		REQUIRE(utils::make_title(input) == "This is the title");
	}

	SECTION("Only cares about last component of the URL")
	{
		auto input = "http://example.com/items/misc/this-is-the-title";
		REQUIRE(utils::make_title(input) == "This is the title");
	}

	SECTION("Strips out trailing slashes")
	{
		std::string input;

		SECTION("One slash")
		{
			input = "http://example.com/item/";
		}

		SECTION("Numerous slashes")
		{
			input = "http://example.com/item/////////////";
		}

		REQUIRE(utils::make_title(input) == "Item");
	}

	SECTION("Doesn't mind invalid URL scheme")
	{
		auto input = "blahscheme://example.com/this-is-the-title";
		REQUIRE(utils::make_title(input) == "This is the title");
	}

	SECTION("Strips out URL query parameters")
	{
		SECTION("Single parameter")
		{
			auto input =
				"http://example.com/story/aug/"
				"title-with-dashes?a=b";
			REQUIRE(utils::make_title(input)
				== "Title with dashes");
		}

		SECTION("Multiple parameters")
		{
			auto input =
				"http://example.com/"
				"title-with-dashes?a=b&x=y&utf8=✓";
			REQUIRE(utils::make_title(input)
				== "Title with dashes");
		}
	}

	SECTION("Decodes percent-encoded characters")
	{
		auto input = "https://example.com/It%27s%202017%21";
		REQUIRE(utils::make_title(input) == "It's 2017!");
	}

	SECTION("Deal with an empty last component")
	{
		auto input = "https://example.com/?format=rss";
		REQUIRE(utils::make_title(input) == "");
	}
}

TEST_CASE(
	"remove_soft_hyphens remove all U+00AD characters from a string",
	"[utils]")
{
	SECTION("doesn't do anything if input has no soft hyphens in it")
	{
		std::string data = "hello world!";
		REQUIRE_NOTHROW(utils::remove_soft_hyphens(data));
		REQUIRE(data == "hello world!");
	}

	SECTION("removes *all* soft hyphens")
	{
		std::string data = "hy\u00ADphen\u00ADa\u00ADtion";
		REQUIRE_NOTHROW(utils::remove_soft_hyphens(data));
		REQUIRE(data == "hyphenation");
	}

	SECTION("removes consequtive soft hyphens")
	{
		std::string data =
			"don't know why any\u00AD\u00ADone would do that";
		REQUIRE_NOTHROW(utils::remove_soft_hyphens(data));
		REQUIRE(data == "don't know why anyone would do that");
	}

	SECTION("removes soft hyphen at the beginning of the line")
	{
		std::string data = "\u00ADtion";
		REQUIRE_NOTHROW(utils::remove_soft_hyphens(data));
		REQUIRE(data == "tion");
	}

	SECTION("removes soft hyphen at the end of the line")
	{
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
	REQUIRE(utils::substr_with_width("A\u3042B\u3044C\u3046", 5)
		== "A\u3042B");

	SECTION("returns an empty string if the given string is empty")
	{
		REQUIRE(utils::substr_with_width("", 0).empty());
		REQUIRE(utils::substr_with_width("", 1).empty());
	}

	SECTION("returns an empty string if the given width is zero")
	{
		REQUIRE(utils::substr_with_width("world", 0).empty());
		REQUIRE(utils::substr_with_width("", 0).empty());
	}

	SECTION("doesn't split single codepoint in two")
	{
		std::string data = "\u3042\u3044\u3046";
		REQUIRE(utils::substr_with_width(data, 1) == "");
		REQUIRE(utils::substr_with_width(data, 3) == "\u3042");
		REQUIRE(utils::substr_with_width(data, 5) == "\u3042\u3044");
	}

	SECTION("doesn't count a width of STFL tag")
	{
		REQUIRE(utils::substr_with_width("ＡＢＣ<b>ＤＥ</b>Ｆ", 9)
			== "ＡＢＣ<b>Ｄ");
		REQUIRE(utils::substr_with_width("<foobar>ＡＢＣ", 4)
			== "<foobar>ＡＢ");
		REQUIRE(utils::substr_with_width("a<<xyz>>bcd", 3)
			== "a<<xyz>>b"); // tag: "<<xyz>"
		REQUIRE(utils::substr_with_width("ＡＢＣ<b>ＤＥ", 10)
			== "ＡＢＣ<b>ＤＥ");
		REQUIRE(utils::substr_with_width("a</>b</>c</>", 2)
			== "a</>b</>");
	}

	SECTION("count a width of escaped less-than mark")
	{
		REQUIRE(utils::substr_with_width("<><><>", 2) == "<><>");
		REQUIRE(utils::substr_with_width("a<>b<>c", 3) == "a<>b");
	}

	SECTION("treat non-printable has zero width")
	{
		REQUIRE(utils::substr_with_width("\x01\x02""abc", 1) == "\x01\x02""a");
	}
}

TEST_CASE("getcwd() returns current directory of the process", "[utils]")
{
	SECTION("Returns non-empty string")
	{
		REQUIRE(utils::getcwd().length() > 0);
	}

	SECTION("Value depends on current directory")
	{
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
		REQUIRE(datadir.substr(datadir.length() - subdir.length())
			== subdir);
	}
}

TEST_CASE(
	"is_valid_podcast_type() returns true if supplied MIME type "
	"is audio or a container",
	"[utils]")
{
	REQUIRE(utils::is_valid_podcast_type("audio/mpeg"));
	REQUIRE(utils::is_valid_podcast_type("audio/mp3"));
	REQUIRE(utils::is_valid_podcast_type("audio/x-mp3"));
	REQUIRE(utils::is_valid_podcast_type("audio/ogg"));
	REQUIRE(utils::is_valid_podcast_type("application/ogg"));

	REQUIRE_FALSE(utils::is_valid_podcast_type("image/jpeg"));
	REQUIRE_FALSE(utils::is_valid_podcast_type("image/png"));
	REQUIRE_FALSE(utils::is_valid_podcast_type("text/plain"));
	REQUIRE_FALSE(utils::is_valid_podcast_type("application/zip"));
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

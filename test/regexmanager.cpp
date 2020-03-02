#include "regexmanager.h"

#include "3rd-party/catch.hpp"

#include "confighandlerexception.h"
#include "matchable.h"

using namespace newsboat;

TEST_CASE("RegexManager throws on invalid command", "[RegexManager]")
{
	RegexManager rxman;
	std::vector<std::string> params;

	SECTION("on invalid command") {
		REQUIRE_THROWS_AS(
			rxman.handle_action("an-invalid-command", params),
			ConfigHandlerException);
	}
}

TEST_CASE("RegexManager throws on invalid `highlight' definition",
	"[RegexManager]")
{
	RegexManager rxman;
	std::vector<std::string> params;

	SECTION("on `highlight' without parameters") {
		REQUIRE_THROWS_AS(rxman.handle_action("highlight", params),
			ConfigHandlerException);
	}

	SECTION("on invalid location") {
		params = {"invalidloc", "foo", "blue", "red"};
		REQUIRE_THROWS_AS(rxman.handle_action("highlight", params),
			ConfigHandlerException);
	}

	SECTION("on invalid regex") {
		params = {"feedlist", "*", "blue", "red"};
		REQUIRE_THROWS_AS(rxman.handle_action("highlight", params),
			ConfigHandlerException);
	}
}

TEST_CASE("RegexManager doesn't throw on valid `highlight' definition",
	"[RegexManager]")
{
	RegexManager rxman;
	std::vector<std::string> params;

	params = {"articlelist", "foo", "blue", "red"};
	REQUIRE_NOTHROW(rxman.handle_action("highlight", params));

	params = {"feedlist", "foo", "blue", "red"};
	REQUIRE_NOTHROW(rxman.handle_action("highlight", params));

	params = {"feedlist", "fbo", "blue", "red", "bold", "underline"};
	REQUIRE_NOTHROW(rxman.handle_action("highlight", params));

	params = {"all", "fba", "blue", "red", "bold", "underline"};
	REQUIRE_NOTHROW(rxman.handle_action("highlight", params));
}

TEST_CASE("RegexManager highlights according to definition", "[RegexManager]")
{
	RegexManager rxman;
	std::string input;

	SECTION("In articlelist") {
		rxman.handle_action("highlight", {"articlelist", "foo", "blue", "red"});
		input = "xfoox";
		rxman.quote_and_highlight(input, "articlelist");
		REQUIRE(input == "x<0>foo</>x");
	}

	SECTION("In feedlist") {
		rxman.handle_action("highlight", {"feedlist", "foo", "blue", "red"});
		input = "yfooy";
		rxman.quote_and_highlight(input, "feedlist");
		REQUIRE(input == "y<0>foo</>y");
	}
}

// This test might seem totally out of the blue, but we do need it: as of this
// writing, `highlight-article` add a nullptr for a regex to "articlelist"
// context, which would've crashed the program if it were to be dereferenced.
// This test checks that those nullptrs are skipped.
TEST_CASE("RegexManager::quote_and_highlight works fine even if there were "
	"`highlight-article` commands",
	"[RegexManager]")
{
	RegexManager rxman;

	rxman.handle_action("highlight", {"articlelist", "foo", "blue", "red"});
	rxman.handle_action("highlight-article", {"title==\"\"", "blue", "red"});

	std::string input = "xfoox";
	rxman.quote_and_highlight(input, "articlelist");
	REQUIRE(input == "x<0>foo</>x");
}

TEST_CASE("RegexManager preserves text when there's nothing to highlight",
	"[RegexManager]")
{
	RegexManager rxman;
	std::string input = "xbarx";
	rxman.quote_and_highlight(input, "feedlist");
	REQUIRE(input == "xbarx");

	input = "<";
	rxman.quote_and_highlight(input, "feedlist");
	REQUIRE(input == "<");

	input = "a<b>";
	rxman.quote_and_highlight(input, "feedlist");
	REQUIRE(input == "a<b>");
}

TEST_CASE("`highlight all` adds rules for all locations", "[RegexManager]")
{
	RegexManager rxman;
	std::vector<std::string> params = {"all", "foo", "red"};
	REQUIRE_NOTHROW(rxman.handle_action("highlight", params));
	std::string input = "xxfooyy";

	for (auto location : {
			"article", "articlelist", "feedlist"
		}) {
		SECTION(location) {
			rxman.quote_and_highlight(input, location);
			REQUIRE(input == "xx<0>foo</>yy");
		}
	}
}

TEST_CASE("RegexManager does not hang on regexes that can match empty strings",
	"[RegexManager]")
{
	RegexManager rxman;
	std::string input = "The quick brown fox jumps over the lazy dog";

	rxman.handle_action("highlight", {"feedlist", "w*", "blue", "red"});
	rxman.quote_and_highlight(input, "feedlist");
	REQUIRE(input == "The quick bro<0>w</>n fox jumps over the lazy dog");
}

TEST_CASE("RegexManager does not hang on regexes that match empty strings",
	"[RegexManager]")
{
	RegexManager rxman;
	std::string input = "The quick brown fox jumps over the lazy dog";
	const std::string compare = input;

	SECTION("testing end of line empty.") {
		rxman.handle_action("highlight", {"feedlist", "$", "blue", "red"});
		rxman.quote_and_highlight(input, "feedlist");
		REQUIRE(input == compare);
	}

	SECTION("testing beginning of line empty") {
		rxman.handle_action("highlight", {"feedlist", "^", "blue", "red"});
		rxman.quote_and_highlight(input, "feedlist");
		REQUIRE(input == compare);
	}

	SECTION("testing empty line") {
		rxman.handle_action("highlight", {"feedlist", "^$", "blue", "red"});
		rxman.quote_and_highlight(input, "feedlist");
		REQUIRE(input == compare);
	}
}

TEST_CASE("quote_and_highlight wraps highlighted text in numbered tags",
	"[RegexManager]")
{
	RegexManager rxman;
	std::string input =  "The quick brown fox jumps over the lazy dog";

	SECTION("Beginning of line match first") {
		const std::string output =
			"<0>The</> quick <1>brown</> fox jumps over <0>the</> lazy dog";
		rxman.handle_action("highlight", {"article", "the", "red"});
		rxman.handle_action("highlight", {"article", "brown", "blue"});
		rxman.quote_and_highlight(input, "article");
		REQUIRE(input == output);
	}

	SECTION("Beginning of line match second") {
		const std::string output =
			"<1>The</> quick <0>brown</> fox jumps over <1>the</> lazy dog";
		rxman.handle_action("highlight", {"article", "brown", "blue"});
		rxman.handle_action("highlight", {"article", "the", "red"});
		rxman.quote_and_highlight(input, "article");
		REQUIRE(input == output);
	}

	SECTION("2 non-overlapping highlights") {
		const std::string output =
			"The <0>quick</> <1>brown</> fox jumps over the lazy dog";
		rxman.handle_action("highlight", {"article", "quick", "red"});
		rxman.handle_action("highlight", {"article", "brown", "blue"});
		rxman.quote_and_highlight(input, "article");
		REQUIRE(input == output);
	}
}

TEST_CASE("RegexManager::extract_outer_marker returns empty string if input "
	"string is empty",
	"[RegexManager]")
{
	RegexManager rxman;
	REQUIRE(rxman.extract_outer_marker("", 0) == "");
	REQUIRE(rxman.extract_outer_marker("", 10) == "");
}

TEST_CASE("RegexManager::extract_outer_marker finds the innermost tag "
	"relative to given position in the text",
	"[RegexManager]")
{
	RegexManager rxman;
	std::string out;

	SECTION("Find outer tag basic") {
		std::string input = "<1>TestString</>";
		out = rxman.extract_outer_marker(input, 7);

		REQUIRE(out == "<1>");
	}

	SECTION("Find nested tag") {
		std::string input = "<1>Nested<2>Test</>String</>";
		out = rxman.extract_outer_marker(input, 14);

		REQUIRE(out == "<2>");
	}

	SECTION("Find outer tag with second set") {
		std::string input = "<1>Nested<2>Test</>String</>";
		out = rxman.extract_outer_marker(input, 21);

		REQUIRE(out == "<1>");
	}

	SECTION("Find unclosed nested tag") {
		std::string input = "<1>Nested<2>Test</>String";
		out = rxman.extract_outer_marker(input, 21);

		REQUIRE(out == "<1>");
	}

}

TEST_CASE("RegexManager::extract_outer_marker returns empty string if input "
	"string contains closing tag without a pairing opening one, before "
	"the position we're looking at",
	"[RegexManager]")
{
	RegexManager rxman;
	REQUIRE(rxman.extract_outer_marker("hello</>world", 10) == "");
}

TEST_CASE("RegexManager::dump_config turns each `highlight` rule into a string, "
	"and appends them onto a vector",
	"[RegexManager]")
{
	RegexManager rxman;
	std::vector<std::string> result;

	SECTION("Empty object returns empty vector") {
		REQUIRE_NOTHROW(rxman.dump_config(result));
		REQUIRE(result.empty());
	}

	SECTION("One rule") {
		rxman.handle_action("highlight", {"article", "this test is", "green"});
		REQUIRE_NOTHROW(rxman.dump_config(result));
		REQUIRE(result.size() == 1);
		REQUIRE(result[0] == R"#(highlight "article" "this test is" "green")#");
	}

	SECTION("Two rules, one of them a `highlight-article`") {
		rxman.handle_action("highlight", {"all", "keywords", "red", "blue"});
		rxman.handle_action(
			"highlight-article",
		{"title==\"\"", "green", "black"});
		REQUIRE_NOTHROW(rxman.dump_config(result));
		REQUIRE(result.size() == 1);
		REQUIRE(result[0] == R"#(highlight "all" "keywords" "red" "blue")#");
	}
}

TEST_CASE("RegexManager::dump_config appends to given vector", "[RegexManager]")
{
	RegexManager rxman;
	std::vector<std::string> result;

	result.emplace_back("sentinel");

	rxman.handle_action("highlight", {"all", "this", "black"});

	REQUIRE_NOTHROW(rxman.dump_config(result));
	REQUIRE(result.size() == 2);
	REQUIRE(result[0] == "sentinel");
	REQUIRE(result[1] == R"#(highlight "all" "this" "black")#");
}

TEST_CASE("RegexManager::handle_action throws ConfigHandlerException "
	"on invalid foreground color",
	"[RegexManager]")
{
	RegexManager rxman;

	REQUIRE_THROWS_AS(
		rxman.handle_action("highlight", {"all", "keyword", "whatever"}),
		ConfigHandlerException);

	REQUIRE_THROWS_AS(
		rxman.handle_action("highlight", {"feedlist", "keyword", ""}),
		ConfigHandlerException);

	REQUIRE_THROWS_AS(
		rxman.handle_action(
			"highlight-article",
	{"author == \"\"", "whatever", "white"}),
	ConfigHandlerException);

	REQUIRE_THROWS_AS(
		rxman.handle_action(
			"highlight-article",
	{"title =~ \"k\"", "", "white"}),
	ConfigHandlerException);
}

TEST_CASE("RegexManager::handle_action throws ConfigHandlerException "
	"on invalid background color",
	"[RegexManager]")
{
	RegexManager rxman;

	REQUIRE_THROWS_AS(
		rxman.handle_action(
			"highlight",
	{"all", "keyword", "red", "whatever"}),
	ConfigHandlerException);

	REQUIRE_THROWS_AS(
		rxman.handle_action(
			"highlight",
	{"feedlist", "keyword", "green", ""}),
	ConfigHandlerException);

	REQUIRE_THROWS_AS(
		rxman.handle_action(
			"highlight-article",
	{"title == \"keyword\"", "red", "whatever"}),
	ConfigHandlerException);

	REQUIRE_THROWS_AS(
		rxman.handle_action(
			"highlight-article",
	{"content == \"\"", "green", ""}),
	ConfigHandlerException);
}

TEST_CASE("RegexManager::handle_action throws ConfigHandlerException "
	"on invalid attribute",
	"[RegexManager]")
{
	RegexManager rxman;

	REQUIRE_THROWS_AS(
		rxman.handle_action(
			"highlight",
	{"all", "keyword", "red", "green", "sparkles"}),
	ConfigHandlerException);

	REQUIRE_THROWS_AS(
		rxman.handle_action(
			"highlight",
	{"feedlist", "keyword", "green", "red", ""}),
	ConfigHandlerException);

	REQUIRE_THROWS_AS(
		rxman.handle_action(
			"highlight-article",
	{"title==\"\"", "red", "green", "sparkles"}),
	ConfigHandlerException);

	REQUIRE_THROWS_AS(
		rxman.handle_action(
			"highlight-article",
	{"title==\"\"", "green", "red", ""}),
	ConfigHandlerException);
}

TEST_CASE("RegexManager throws on invalid `highlight-article' definition",
	"[RegexManager]")
{
	RegexManager rxman;
	std::vector<std::string> params;

	SECTION("on `highlight-article' without parameters") {
		REQUIRE_THROWS_AS(rxman.handle_action("highlight-article", params),
			ConfigHandlerException);
	}

	SECTION("on invalid filter expression") {
		params = {"a = b", "red", "green"};
		REQUIRE_THROWS_AS(rxman.handle_action("highlight-article", params),
			ConfigHandlerException);
	}

	SECTION("on missing colors") {
		params = {"title==\"\""};
		REQUIRE_THROWS_AS(rxman.handle_action("highlight-article", params),
			ConfigHandlerException);
	}

	SECTION("on missing background color") {
		params = {"title==\"\"", "white"};
		REQUIRE_THROWS_AS(rxman.handle_action("highlight-article", params),
			ConfigHandlerException);
	}
}

TEST_CASE("RegexManager doesn't throw on valid `highlight-article' definition",
	"[RegexManager]")
{
	RegexManager rxman;
	std::vector<std::string> params;

	params = {"title == \"\"", "blue", "red"};
	REQUIRE_NOTHROW(rxman.handle_action("highlight-article", params));

	params = {"content =~ \"keyword\"", "blue", "red"};
	REQUIRE_NOTHROW(rxman.handle_action("highlight-article", params));

	params = {"unread == \"yes\"", "blue", "red", "bold", "underline"};
	REQUIRE_NOTHROW(rxman.handle_action("highlight-article", params));

	params = {"age > 3", "blue", "red", "bold", "underline"};
	REQUIRE_NOTHROW(rxman.handle_action("highlight-article", params));
}

struct RegexManagerMockMatchable : public Matchable {
public:
	bool has_attribute(const std::string& attribname) override
	{
		return attribname == "attr";
	}

	std::string get_attribute(const std::string& attribname) override
	{
		if (attribname == "attr") {
			return "val";
		}
		return "";
	}
};

TEST_CASE("RegexManager::article_matches returns position of the Matcher "
	"that matches a given Matchable",
	"[RegexManager]")
{
	RegexManager rxman;
	RegexManagerMockMatchable mock;

	const auto cmd = std::string("highlight-article");

	SECTION("Just one rule") {
		rxman.handle_action(cmd, {"attr != \"hello\"", "red", "green"});

		REQUIRE(rxman.article_matches(&mock) == 0);
	}

	SECTION("Couple rules") {
		rxman.handle_action(cmd, {"attr != \"val\"", "green", "white"});
		rxman.handle_action(cmd, {"attr # \"entry\"", "green", "white"});
		rxman.handle_action(cmd, {"attr == \"val\"", "green", "white"});
		rxman.handle_action(cmd, {"attr =~ \"hello\"", "green", "white"});

		REQUIRE(rxman.article_matches(&mock) == 2);
	}
}

TEST_CASE("RegexManager::article_matches returns -1 if there are no Matcher "
	"to match a given Matchable",
	"[RegexManager]")
{
	RegexManager rxman;
	RegexManagerMockMatchable mock;

	const auto cmd = std::string("highlight-article");

	SECTION("No rules") {
		REQUIRE(rxman.article_matches(&mock) == -1);
	}

	SECTION("Just one rule") {
		rxman.handle_action(cmd, {"attr == \"hello\"", "red", "green"});

		REQUIRE(rxman.article_matches(&mock) == -1);
	}

	SECTION("Couple rules") {
		rxman.handle_action(cmd, {"attr != \"val\"", "green", "white"});
		rxman.handle_action(cmd, {"attr # \"entry\"", "green", "white"});
		rxman.handle_action(cmd, {"attr =~ \"hello\"", "green", "white"});

		REQUIRE(rxman.article_matches(&mock) == -1);
	}
}

TEST_CASE("RegexManager::remove_last_regex removes last added `highlight` rule",
	"[RegexManager]")
{
	RegexManager rxman;

	rxman.handle_action("highlight", {"articlelist", "foo", "blue", "red"});
	rxman.handle_action("highlight", {"articlelist", "bar", "blue", "red"});

	const auto INPUT = std::string("xfoobarx");

	auto input = INPUT;
	rxman.quote_and_highlight(input, "articlelist");
	REQUIRE(input == "x<0>foo</><1>bar</>x");

	input = INPUT;
	rxman.quote_and_highlight(input, "feedlist");
	REQUIRE(input == INPUT);

	REQUIRE_NOTHROW(rxman.remove_last_regex("articlelist"));

	input = INPUT;
	rxman.quote_and_highlight(input, "articlelist");
	REQUIRE(input == "x<0>foo</>barx");

	input = INPUT;
	rxman.quote_and_highlight(input, "feedlist");
	REQUIRE(input == INPUT);
}

TEST_CASE("RegexManager::remove_last_regex does not crash if there are "
	"no regexes to remove",
	"[RegexManager]")
{
	RegexManager rxman;

	SECTION("No rules existed") {
		rxman.remove_last_regex("articlelist");

		SECTION("Repeated calls don't crash either") {
			rxman.remove_last_regex("articlelist");
			rxman.remove_last_regex("articlelist");
			rxman.remove_last_regex("articlelist");

			rxman.remove_last_regex("feedlist");
		}
	}

	SECTION("A few rules were added and then deleted") {
		rxman.handle_action("highlight", {"articlelist", "test test", "red"});
		rxman.handle_action("highlight", {"articlelist", "another test", "red"});
		rxman.handle_action("highlight", {"articlelist", "more", "green", "blue"});

		rxman.remove_last_regex("articlelist");
		rxman.remove_last_regex("articlelist");
		rxman.remove_last_regex("articlelist");
		// At this point, all the rules are removed
		rxman.remove_last_regex("articlelist");

		SECTION("Repeated calls don't crash either") {
			rxman.remove_last_regex("articlelist");
			rxman.remove_last_regex("articlelist");
			rxman.remove_last_regex("articlelist");

			rxman.remove_last_regex("feedlist");
		}
	}
}

TEST_CASE("RegexManager uses POSIX extended regex syntax",
	"[RegexManager]")
{
	// This syntax is documented in The Open Group Base Specifications Issue 7,
	// IEEE Std 1003.1-2008 Section 9, "Regular Expressions":
	// https://pubs.opengroup.org/onlinepubs/9699919799.2008edition/basedefs/V1_chap09.html

	// Since POSIX extended regular expressions are pretty basic, it's hard to
	// find stuff that they support but other engines don't. So in order to
	// ensure that we're using EREs, these tests try stuff that's *not*
	// supported by EREs.
	//
	// Ideas gleaned from https://www.regular-expressions.info/refcharacters.html

	RegexManager rxman;

	// Supported by Perl, PCRE, PHP and others
	SECTION("No support for escape sequence") {
		rxman.handle_action("highlight", {"articlelist", R"#(\Q*]+\E)#", "red"});

		std::string input = "*]+";
		rxman.quote_and_highlight(input, "articlelist");
		REQUIRE(input == "*]+");
	}

	SECTION("No support for hexadecimal escape") {
		rxman.handle_action("highlight", {"articlelist", R"#(^va\x6Cue)#", "red"});

		std::string input = "value";
		rxman.quote_and_highlight(input, "articlelist");
		REQUIRE(input == "value");
	}

	SECTION("No support for \\a as alert/bell control character") {
		rxman.handle_action("highlight", {"articlelist", R"#(\a)#", "red"});

		std::string input = "\x07";
		rxman.quote_and_highlight(input, "articlelist");
		REQUIRE(input == "\x07");
	}

	SECTION("No support for \\b as backspace control character") {
		rxman.handle_action("highlight", {"articlelist", R"#(\b)#", "red"});

		std::string input = "\x08";
		rxman.quote_and_highlight(input, "articlelist");
		REQUIRE(input == "\x08");
	}

	// If you add more checks to this test, consider adding the same to Matcher tests
}

TEST_CASE("quote_and_highlight() does not break existing tags like <unread>",
	"[RegexManager]")
{
	RegexManager rxman;
	std::string input =  "<unread>This entry is unread now</>";

	WHEN("matching `read`") {
		const std::string output = "<unread>This entry is un<0>read</> now</>";
		rxman.handle_action("highlight", {"article", "read", "red"});
		rxman.quote_and_highlight(input, "article");
		REQUIRE(input == output);
	}

	WHEN("matching `unread`") {
		const std::string output = "<unread>This entry is <0>unread</> now</>";
		rxman.handle_action("highlight", {"article", "unread", "red"});
		rxman.quote_and_highlight(input, "article");
		REQUIRE(input == output);
	}

	WHEN("matching the full line") {
		const std::string output = "<unread><0>This entry is unread now</></>";
		rxman.handle_action("highlight", {"article", "^.*$", "red"});
		rxman.quote_and_highlight(input, "article");
		REQUIRE(input == output);
	}
}

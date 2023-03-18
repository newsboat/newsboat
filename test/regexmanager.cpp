#include "regexmanager.h"

#include "3rd-party/catch.hpp"

#include "confighandlerexception.h"
#include "matchable.h"
#include "matcherexception.h"

using namespace newsboat;

TEST_CASE("RegexManager throws on invalid command", "[RegexManager]")
{
	RegexManager rxman;

	SECTION("on invalid command") {
		std::vector<std::string> params;
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

TEST_CASE("quote_and_highlight() only matches `^` at the start of the line",
	"[RegexManager]")
{
	RegexManager rxman;
	std::string input = "This is a test";

	SECTION("^.") {
		rxman.handle_action("highlight", {"article", "^.", "blue", "red"});
		rxman.quote_and_highlight(input, "article");
		REQUIRE(input == "<0>T</>his is a test");
	}

	SECTION("(^Th|^is)") {
		rxman.handle_action("highlight", {"article", "(^Th|^is)", "blue", "red"});
		rxman.quote_and_highlight(input, "article");
		REQUIRE(input == "<0>Th</>is is a test");
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

	input = "a<b>";
	rxman.quote_and_highlight(input, "feedlist");
	REQUIRE(input == "a<b>");

	SECTION("encode `<` as `<>` for stfl") {
		// Construct the string explicitly to work around GCC 12 bug:
		// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105329
		input = std::string("<");
		rxman.quote_and_highlight(input, "feedlist");
		REQUIRE(input == "<>");
	}
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

TEST_CASE("RegexManager::dump_config turns each `highlight`, "
	"`highlight-article` and `highlight-feed` rule into a string, and appends "
	"them onto a vector",
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
		REQUIRE(result.size() == 2);
		REQUIRE(result[0] == R"#(highlight "all" "keywords" "red" "blue")#");
		REQUIRE(result[1] == R"#(highlight-article "title==\"\"" "green" "black")#");
	}

	SECTION("Three rules") {
		rxman.handle_action("highlight", {"all", "keywords", "red", "blue"});
		rxman.handle_action(
			"highlight-article",
		{"title==\"\"", "green", "black"});
		rxman.handle_action(
			"highlight-feed",
		{"title==\"\"", "red", "black"});
		REQUIRE_NOTHROW(rxman.dump_config(result));
		REQUIRE(result.size() == 3);
		REQUIRE(result[0] == R"#(highlight "all" "keywords" "red" "blue")#");
		REQUIRE(result[1] == R"#(highlight-article "title==\"\"" "green" "black")#");
		REQUIRE(result[2] == R"#(highlight-feed "title==\"\"" "red" "black")#");
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

TEST_CASE("RegexManager throws on invalid `highlight-feed' definition",
	"[RegexManager]")
{
	RegexManager rxman;
	std::vector<std::string> params;

	SECTION("on `highlight-feed' without parameters") {
		REQUIRE_THROWS_AS(rxman.handle_action("highlight-feed", params),
			ConfigHandlerException);
	}

	SECTION("on invalid filter expression") {
		params = {"a = b", "red", "green"};
		REQUIRE_THROWS_AS(rxman.handle_action("highlight-feed", params),
			ConfigHandlerException);
	}

	SECTION("on missing colors") {
		params = {"title==\"\""};
		REQUIRE_THROWS_AS(rxman.handle_action("highlight-feed", params),
			ConfigHandlerException);
	}

	SECTION("on missing background color") {
		params = {"title==\"\"", "white"};
		REQUIRE_THROWS_AS(rxman.handle_action("highlight-feed", params),
			ConfigHandlerException);
	}
}

TEST_CASE("RegexManager doesn't throw on valid `highlight-feed' definition",
	"[RegexManager]")
{
	RegexManager rxman;
	std::vector<std::string> params;

	params = {"title == \"\"", "blue", "red"};
	REQUIRE_NOTHROW(rxman.handle_action("highlight-feed", params));

	params = {"content =~ \"keyword\"", "blue", "red"};
	REQUIRE_NOTHROW(rxman.handle_action("highlight-feed", params));

	params = {"unread == \"yes\"", "blue", "red", "bold", "underline"};
	REQUIRE_NOTHROW(rxman.handle_action("highlight-feed", params));

	params = {"age > 3", "blue", "red", "bold", "underline"};
	REQUIRE_NOTHROW(rxman.handle_action("highlight-feed", params));
}

struct RegexManagerMockMatchable : public Matchable {
public:
	nonstd::optional<std::string> attribute_value(const std::string& attribname)
	const override
	{
		if (attribname == "attr") {
			return "val";
		}
		return nonstd::nullopt;
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

TEST_CASE("RegexManager::feed_matches returns position of the Matcher "
	"that matches a given Matchable",
	"[RegexManager]")
{
	RegexManager rxman;
	RegexManagerMockMatchable mock;

	const auto cmd = std::string("highlight-feed");

	SECTION("Just one rule") {
		rxman.handle_action(cmd, {"attr != \"hello\"", "red", "green"});

		REQUIRE(rxman.feed_matches(&mock) == 0);
	}

	SECTION("Couple rules") {
		rxman.handle_action(cmd, {"attr != \"val\"", "green", "white"});
		rxman.handle_action(cmd, {"attr # \"entry\"", "green", "white"});
		rxman.handle_action(cmd, {"attr == \"val\"", "green", "white"});
		rxman.handle_action(cmd, {"attr =~ \"hello\"", "green", "white"});

		REQUIRE(rxman.feed_matches(&mock) == 2);
	}
}

TEST_CASE("RegexManager::feed_matches returns -1 if there are no Matcher "
	"to match a given Matchable",
	"[RegexManager]")
{
	RegexManager rxman;
	RegexManagerMockMatchable mock;

	const auto cmd = std::string("highlight-feed");

	SECTION("No rules") {
		REQUIRE(rxman.feed_matches(&mock) == -1);
	}

	SECTION("Just one rule") {
		rxman.handle_action(cmd, {"attr == \"hello\"", "red", "green"});

		REQUIRE(rxman.feed_matches(&mock) == -1);
	}

	SECTION("Couple rules") {
		rxman.handle_action(cmd, {"attr != \"val\"", "green", "white"});
		rxman.handle_action(cmd, {"attr # \"entry\"", "green", "white"});
		rxman.handle_action(cmd, {"attr =~ \"hello\"", "green", "white"});

		REQUIRE(rxman.feed_matches(&mock) == -1);
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
	REQUIRE(input == "x<0>foo<1>bar</>x");

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
	// FreeBSD 13 returns an error from regexec() instead of failing the match
	// like other OSes do. That's why we wrap our tests in a try-catch.
	//
	// Ideas gleaned from https://www.regular-expressions.info/refcharacters.html

	RegexManager rxman;

	// Supported by Perl, PCRE, PHP and others
	SECTION("No support for escape sequence") {
		try {
			rxman.handle_action("highlight", {"articlelist", R"#(\Q*]+\E)#", "red"});
		} catch (const ConfigHandlerException& e) {
			const std::string expected(
				R"#(`\Q*]+\E' is not a valid regular expression: trailing backslash (\))#");
			REQUIRE(e.what() == expected);
		}

		std::string input = "*]+";
		rxman.quote_and_highlight(input, "articlelist");
		REQUIRE(input == "*]+");
	}

	SECTION("No support for hexadecimal escape") {
		try {
			rxman.handle_action("highlight", {"articlelist", R"#(^va\x6Cue)#", "red"});
		} catch (const ConfigHandlerException& e) {
			const std::string expected(
				R"#(`^va\x6Cue' is not a valid regular expression: trailing backslash (\))#");
			REQUIRE(e.what() == expected);
		}

		std::string input = "value";
		rxman.quote_and_highlight(input, "articlelist");
		REQUIRE(input == "value");
	}

	SECTION("No support for \\a as alert/bell control character") {
		try {
			rxman.handle_action("highlight", {"articlelist", R"#(\a)#", "red"});
		} catch (const ConfigHandlerException& e) {
			const std::string expected(
				R"#(`\a' is not a valid regular expression: trailing backslash (\))#");
			REQUIRE(e.what() == expected);
		}

		std::string input = "\x07";
		rxman.quote_and_highlight(input, "articlelist");
		REQUIRE(input == "\x07");
	}

	SECTION("No support for \\b as backspace control character") {
		try {
			rxman.handle_action("highlight", {"articlelist", R"#(\b)#", "red"});
		} catch (const ConfigHandlerException& e) {
			const std::string expected(
				R"#(`\b' is not a valid regular expression: trailing backslash (\))#");
			REQUIRE(e.what() == expected);
		}

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
	std::string input =  "<unread>This entry is unread</>";

	WHEN("matching `read`") {
		const std::string output = "<unread>This entry is un<0>read</>";
		rxman.handle_action("highlight", {"article", "read", "red"});
		rxman.quote_and_highlight(input, "article");
		REQUIRE(input == output);
	}

	WHEN("matching `unread`") {
		const std::string output = "<unread>This entry is <0>unread</>";
		rxman.handle_action("highlight", {"article", "unread", "red"});
		rxman.quote_and_highlight(input, "article");
		REQUIRE(input == output);
	}

	WHEN("matching the full line") {
		rxman.handle_action("highlight", {"article", "^.*$", "red"});

		THEN("the <unread> tag is overwritten") {
			const std::string output = "<0>This entry is unread</>";
			rxman.quote_and_highlight(input, "article");
			REQUIRE(input == output);
		}
	}
}

TEST_CASE("quote_and_highlight() ignores tags when matching the regular expressions",
	"[RegexManager]")
{
	RegexManager rxman;
	std::string input =  "<unread>This entry is unread</>";

	WHEN("matching text at the start of the line") {
		rxman.handle_action("highlight", {"article", "^This", "red"});

		THEN("the <unread> tag should be ignored") {
			const std::string output = "<0>This<unread> entry is unread</>";
			rxman.quote_and_highlight(input, "article");
			REQUIRE(input == output);
		}
	}

	WHEN("matching text at the end of the line") {
		rxman.handle_action("highlight", {"article", "unread$", "red"});

		THEN("the closing tag `</>` (related to <unread>) should be ignored") {
			const std::string output = "<unread>This entry is <0>unread</>";
			rxman.quote_and_highlight(input, "article");
			REQUIRE(input == output);
		}
	}

	WHEN("matching text which contains `<u>...</>` markers (added by HTML renderer)") {
		input = "Some<u>thing</> is underlined";
		rxman.handle_action("highlight", {"article", "Something", "red"});

		THEN("the tags should be ignored when matching text with a regular expression") {
			const std::string output = "<0>Something</> is underlined";
			rxman.quote_and_highlight(input, "article");
			REQUIRE(input == output);
		}
	}
}

TEST_CASE("quote_and_highlight() generates a sensible output when multiple matches overlap",
	"[RegexManager]")
{
	RegexManager rxman;
	std::string input = "The quick brown fox jumps over the lazy dog";

	WHEN("a second match is completely inside of the first match") {
		rxman.handle_action("highlight", {"article", "The quick brown", "red"});
		rxman.handle_action("highlight", {"article", "quick", "red"});

		THEN("the first style should be restored at the end of the second match") {
			const std::string output =
				"<0>The <1>quick<0> brown</> fox jumps over the lazy dog";
			rxman.quote_and_highlight(input, "article");
			REQUIRE(input == output);
		}
	}

	WHEN("a second match completely encloses the first match") {
		rxman.handle_action("highlight", {"article", "quick", "red"});
		rxman.handle_action("highlight", {"article", "The quick brown", "red"});

		THEN("the first style should not be present anymore") {
			const std::string output =
				"<1>The quick brown</> fox jumps over the lazy dog";
			rxman.quote_and_highlight(input, "article");
			REQUIRE(input == output);
		}
	}

	WHEN("a second match starts somewhere in the first match") {
		rxman.handle_action("highlight", {"article", "quick brown", "red"});
		rxman.handle_action("highlight", {"article", "brown fox", "red"});

		THEN("the first style should only be overwritten for the part where the matches intersect") {
			const std::string output =
				"The <0>quick <1>brown fox</> jumps over the lazy dog";
			rxman.quote_and_highlight(input, "article");
			REQUIRE(input == output);
		}
	}

	WHEN("a second match ends somewhere in the first match") {
		rxman.handle_action("highlight", {"article", "brown fox", "red"});
		rxman.handle_action("highlight", {"article", "quick brown", "red"});

		THEN("the first style should only be overwritten for the part where the matches intersect") {
			const std::string output =
				"The <1>quick brown<0> fox</> jumps over the lazy dog";
			rxman.quote_and_highlight(input, "article");
			REQUIRE(input == output);
		}
	}

	WHEN("there are three overlapping matches") {
		rxman.handle_action("highlight", {"article", "quick.*dog", "red"});
		rxman.handle_action("highlight", {"article", "fox jumps over", "red"});
		rxman.handle_action("highlight", {"article", "brown fox", "red"});

		THEN("newer matches overwrite older matches") {
			const std::string output =
				"The <0>quick <2>brown fox<1> jumps over<0> the lazy dog</>";
			rxman.quote_and_highlight(input, "article");
			REQUIRE(input == output);
		}
	}
}

TEST_CASE("quote_and_highlight() keeps stfl-encoded angle brackets and allows matching them directly",
	"[RegexManager]")
{
	RegexManager rxman;
	std::string input = "<unread>title with <>literal> angle brackets</>";

	SECTION("stfl-encoded angle brackets are kept/restored") {
		const std::string output = "<unread>title with <>literal> angle brackets</>";
		rxman.quote_and_highlight(input, "article");
		REQUIRE(input == output);
	}

	SECTION("angle brackets can be matched directly") {
		const std::string output =
			"<unread>title with <0><>literal><unread> angle brackets</>";
		rxman.handle_action("highlight", {"article", "<literal>", "red"});
		rxman.quote_and_highlight(input, "article");
		REQUIRE(input == output);
	}
}

TEST_CASE("extract_style_tags() returns map which links locations to tags from input string",
	"[RegexManager]")
{
	RegexManager rxman;

	SECTION("input with no tags at all") {
		std::string input = "The quick brown fox jumps over the lazy dog";
		const std::string output = "The quick brown fox jumps over the lazy dog";
		auto tags = rxman.extract_style_tags(input);
		REQUIRE(input == output);
		REQUIRE(tags.size() == 0);
	}

	SECTION("input with various tags") {
		std::string input = "<unread>title <0>with <u>underline<0> and style</>";
		const std::string output = "title with underline and style";
		auto tags = rxman.extract_style_tags(input);
		REQUIRE(input == output);
		REQUIRE(tags[0]  == "<unread>");
		REQUIRE(tags[6]  == "<0>");
		REQUIRE(tags[11] == "<u>");
		REQUIRE(tags[20] == "<0>");
		REQUIRE(tags[30] == "</>");
		REQUIRE(tags.size() == 5);
	}
}

TEST_CASE("extract_style_tags() keeps stfl-encoded angle brackets in string",
	"[RegexManager]")
{
	RegexManager rxman;

	SECTION("stfl-encoded angle brackets should be preserved") {
		std::string input = "<unread>title with <>literal> angle brackets</>";
		const std::string output = "title with <literal> angle brackets";
		auto tags = rxman.extract_style_tags(input);
		REQUIRE(input == output);
		REQUIRE(tags[0]  == "<unread>");
		REQUIRE(tags[35] == "</>");
		REQUIRE(tags.size() == 2);
	}
}

TEST_CASE("extract_style_tags() ignores invalid characters",
	"[RegexManager]")
{
	RegexManager rxman;
	// Different output might be acceptable as this input would/should be
	// invalid but it makes sense to keep a consistent result when refactoring.

	SECTION("double tag open") {
		std::string input = "<unread>title <with <u>repeated tag opening bracket</>";
		const std::string output = "title <with repeated tag opening bracket";
		auto tags = rxman.extract_style_tags(input);
		REQUIRE(input == output);
		REQUIRE(tags[0]  == "<unread>");
		REQUIRE(tags[12] == "<u>");
		REQUIRE(tags[40] == "</>");
		REQUIRE(tags.size() == 3);
	}

	SECTION("unmatched '<' at end of line") {
		std::string input = "some <u>underlining</>, nothing else<";
		const std::string output = "some underlining, nothing else<";
		auto tags = rxman.extract_style_tags(input);
		REQUIRE(input == output);
		REQUIRE(tags[5]  == "<u>");
		REQUIRE(tags[16]  == "</>");
		REQUIRE(tags.size() == 2);
	}
}

TEST_CASE("insert_style_tags() adds tags into string at correct positions",
	"[RegexManager]")
{
	RegexManager rxman;
	std::string input = "This is a sentence";

	SECTION("string does not change if no tags are specified") {
		std::map<size_t, std::string> tags;
		const std::string output = "This is a sentence";
		rxman.insert_style_tags(input, tags);
		REQUIRE(input == output);
	}

	SECTION("tags are added at the correct location") {
		std::map<size_t, std::string> tags = {
			{0, "<0>"},
			{1, "<1>"},
			{10, "<hello>"},
			{18, "</>"},
		};
		const std::string output = "<0>T<1>his is a <hello>sentence</>";
		rxman.insert_style_tags(input, tags);
		REQUIRE(input == output);
	}
}

TEST_CASE("insert_style_tags() stfl-encodes angle brackets existing in input string",
	"[RegexManager]")
{
	RegexManager rxman;
	std::string input = ">>This <is> a sentence with brackets<<";

	SECTION("brackets are stfl-encoded") {
		std::map<size_t, std::string> tags;
		const std::string output = ">>This <>is> a sentence with brackets<><>";
		rxman.insert_style_tags(input, tags);
		REQUIRE(input == output);
	}

	SECTION("brackets are stfl-encoded and tags are inserted") {
		std::map<size_t, std::string> tags = {
			{0, "<0>"},
			{1, "<1>"},
			{6, "<hello>"},
			{38, "</>"},
		};
		const std::string output =
			"<0>><1>>This<hello> <>is> a sentence with brackets<><></>";
		rxman.insert_style_tags(input, tags);
		REQUIRE(input == output);
	}
}

TEST_CASE("insert_style_tags() does not crash on invalid input",
	"[RegexManager]")
{
	RegexManager rxman;

	SECTION("tags with a position outside  of the string are ignored") {
		std::map<size_t, std::string> tags = {
			{0, "<test>"},
			{9, "<in>"},
			{10, "<out>"},
		};
		std::string input = "test this";
		const std::string output = "<test>test this<in>";
		rxman.insert_style_tags(input, tags);
		REQUIRE(input == output);
	}
}

TEST_CASE("merge_style_tag() removes tags between start and end positions",
	"[RegexManager]")
{
	RegexManager rxman;
	const std::string new_tag = "<test>";
	std::map<size_t, std::string> tags = {
		{0, "<0>"},
		{5, "<1>"},
		{7, "<2>"},
		{10, "<3>"},
		{15, "</>"},
	};

	rxman.merge_style_tag(tags, new_tag, 2, 10);
	REQUIRE(tags[0] == "<0>");
	REQUIRE(tags[2] == new_tag);
	REQUIRE(tags[10] == "<3>");
	REQUIRE(tags[15] == "</>");
	REQUIRE(tags.size() == 4);
}

TEST_CASE("merge_style_tag() restores previous tag after the inserted tag if necessary",
	"[RegexManager]")
{
	RegexManager rxman;
	const std::string new_tag = "<test>";
	std::map<size_t, std::string> tags = {
		{0, "<0>"},
		{5, "<1>"},
		{10, "</>"},
	};

	SECTION("end before tag switch so restore first tag") {
		rxman.merge_style_tag(tags, new_tag, 0, 4);
		REQUIRE(tags[0] == new_tag);
		REQUIRE(tags[4] == "<0>");
		REQUIRE(tags[5] == "<1>");
		REQUIRE(tags[10] == "</>");
		REQUIRE(tags.size() == 4);
	}

	SECTION("end on tag switch so no need to restore anything") {
		rxman.merge_style_tag(tags, new_tag, 0, 5);
		REQUIRE(tags[0] == new_tag);
		REQUIRE(tags[5] == "<1>");
		REQUIRE(tags[10] == "</>");
		REQUIRE(tags.size() == 3);
	}

	SECTION("end after a closing tag (</>) so close at the end") {
		SECTION("on the closing tag") {
			rxman.merge_style_tag(tags, new_tag, 0, 10);
			REQUIRE(tags[0] == new_tag);
			REQUIRE(tags[10] == "</>");
			REQUIRE(tags.size() == 2);
		}

		SECTION("after the closing tag") {
			rxman.merge_style_tag(tags, new_tag, 0, 11);
			REQUIRE(tags[0] == new_tag);
			REQUIRE(tags[11] == "</>");
			REQUIRE(tags.size() == 2);
		}
	}
}

TEST_CASE("merge_style_tag() does not crash on invalid input",
	"[RegexManager]")
{
	RegexManager rxman;

	SECTION("ignore tag merging if `end <= start`") {
		std::map<size_t, std::string> tags;
		const std::string new_tag = "<test>";
		rxman.merge_style_tag(tags, new_tag, 0, 0);
		rxman.merge_style_tag(tags, new_tag, 1, 0);
		rxman.merge_style_tag(tags, new_tag, 5, 4);
		REQUIRE(tags.size() == 0);
	}
}

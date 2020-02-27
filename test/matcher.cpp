#include "matcher.h"

#include "3rd-party/catch.hpp"

#include <map>

#include "matchable.h"
#include "matcherexception.h"

using namespace newsboat;

class MatcherMockMatchable : public Matchable {
public:
	MatcherMockMatchable() = default;

	MatcherMockMatchable(
		std::initializer_list<std::pair<const std::string, std::string>>
		data)
		: m_data(data)
	{}

	virtual bool has_attribute(const std::string& attribname)
	{
		return m_data.find(attribname) != m_data.cend();
	}

	virtual std::string get_attribute(const std::string& attribname)
	{
		const auto it = m_data.find(attribname);
		if (it != m_data.cend()) {
			return it->second;
		}

		return "";
	}

private:
	std::map<std::string, std::string> m_data;
};

TEST_CASE("Operator `=` checks if field has given value", "[Matcher]")
{
	MatcherMockMatchable mock({{"abcd", "xyz"}});
	Matcher m;

	REQUIRE(m.parse("abcd = \"xyz\""));
	REQUIRE(m.matches(&mock));

	REQUIRE(m.parse("abcd = \"uiop\""));
	REQUIRE_FALSE(m.matches(&mock));
}

TEST_CASE("Operator `!=` checks if field doesn't have given value", "[Matcher]")
{
	MatcherMockMatchable mock({{"abcd", "xyz"}});
	Matcher m;

	REQUIRE(m.parse("abcd != \"uiop\""));
	REQUIRE(m.matches(&mock));

	REQUIRE(m.parse("abcd != \"xyz\""));
	REQUIRE_FALSE(m.matches(&mock));
}

TEST_CASE("Operator `=~` checks if field matches given regex", "[Matcher]")
{
	MatcherMockMatchable mock({{"AAAA", "12345"}});
	Matcher m;

	REQUIRE(m.parse("AAAA =~ \".\""));
	REQUIRE(m.matches(&mock));

	REQUIRE(m.parse("AAAA =~ \"123\""));
	REQUIRE(m.matches(&mock));

	REQUIRE(m.parse("AAAA =~ \"234\""));
	REQUIRE(m.matches(&mock));

	REQUIRE(m.parse("AAAA =~ \"45\""));
	REQUIRE(m.matches(&mock));

	REQUIRE(m.parse("AAAA =~ \"^12345$\""));
	REQUIRE(m.matches(&mock));

	REQUIRE(m.parse("AAAA =~ \"^123456$\""));
	REQUIRE_FALSE(m.matches(&mock));
}

TEST_CASE("Matcher throws if expression contains undefined fields", "[Matcher]")
{
	MatcherMockMatchable mock;
	Matcher m;

	REQUIRE(m.parse("BBBB =~ \"foo\""));
	REQUIRE_THROWS_AS(m.matches(&mock), MatcherException);

	REQUIRE(m.parse("BBBB # \"foo\""));
	REQUIRE_THROWS_AS(m.matches(&mock), MatcherException);

	REQUIRE(m.parse("BBBB < 0"));
	REQUIRE_THROWS_AS(m.matches(&mock), MatcherException);

	REQUIRE(m.parse("BBBB > 0"));
	REQUIRE_THROWS_AS(m.matches(&mock), MatcherException);

	REQUIRE(m.parse("BBBB between 1:23"));
	REQUIRE_THROWS_AS(m.matches(&mock), MatcherException);
}

TEST_CASE("Matcher throws if regex passed to `=~` or `!~` is invalid",
	"[Matcher]")
{
	MatcherMockMatchable mock({{"AAAA", "12345"}});
	Matcher m;

	REQUIRE(m.parse("AAAA =~ \"[[\""));
	REQUIRE_THROWS_AS(m.matches(&mock), MatcherException);

	REQUIRE(m.parse("AAAA !~ \"[[\""));
	REQUIRE_THROWS_AS(m.matches(&mock), MatcherException);
}

TEST_CASE("Operator `!~` checks if field doesn't match given regex",
	"[Matcher]")
{
	MatcherMockMatchable mock({{"AAAA", "12345"}});
	Matcher m;

	REQUIRE(m.parse("AAAA !~ \".\""));
	REQUIRE_FALSE(m.matches(&mock));

	REQUIRE(m.parse("AAAA !~ \"123\""));
	REQUIRE_FALSE(m.matches(&mock));

	REQUIRE(m.parse("AAAA !~ \"234\""));
	REQUIRE_FALSE(m.matches(&mock));

	REQUIRE(m.parse("AAAA !~ \"45\""));
	REQUIRE_FALSE(m.matches(&mock));

	REQUIRE(m.parse("AAAA !~ \"^12345$\""));
	REQUIRE_FALSE(m.matches(&mock));
}

TEST_CASE("Operator `#` checks if \"tags\" field contains given value",
	"[Matcher]")
{
	MatcherMockMatchable mock({{"tags", "foo bar baz quux"}});
	Matcher m;

	REQUIRE(m.parse("tags # \"foo\""));
	REQUIRE(m.matches(&mock));

	REQUIRE(m.parse("tags # \"baz\""));
	REQUIRE(m.matches(&mock));

	REQUIRE(m.parse("tags # \"quux\""));
	REQUIRE(m.matches(&mock));

	REQUIRE(m.parse("tags # \"xyz\""));
	REQUIRE_FALSE(m.matches(&mock));

	REQUIRE(m.parse("tags # \"foo bar\""));
	REQUIRE_FALSE(m.matches(&mock));

	REQUIRE(m.parse("tags # \"foo\" and tags # \"bar\""));
	REQUIRE(m.matches(&mock));

	REQUIRE(m.parse("tags # \"foo\" and tags # \"xyz\""));
	REQUIRE_FALSE(m.matches(&mock));

	REQUIRE(m.parse("tags # \"foo\" or tags # \"xyz\""));
	REQUIRE(m.matches(&mock));
}

TEST_CASE("Operator `!#` checks if \"tags\" field doesn't contain given value",
	"[Matcher]")
{
	MatcherMockMatchable mock({{"tags", "foo bar baz quux"}});
	Matcher m;

	REQUIRE(m.parse("tags !# \"nein\""));
	REQUIRE(m.matches(&mock));

	REQUIRE(m.parse("tags !# \"foo\""));
	REQUIRE_FALSE(m.matches(&mock));
}

TEST_CASE(
	"Operators `>`, `>=`, `<` and `<=` compare field's value to given "
	"value",
	"[Matcher]")
{
	MatcherMockMatchable mock({{"AAAA", "12345"}});
	Matcher m;

	REQUIRE(m.parse("AAAA > 12344"));
	REQUIRE(m.matches(&mock));

	REQUIRE(m.parse("AAAA > 12345"));
	REQUIRE_FALSE(m.matches(&mock));

	REQUIRE(m.parse("AAAA >= 12345"));
	REQUIRE(m.matches(&mock));

	REQUIRE(m.parse("AAAA < 12345"));
	REQUIRE_FALSE(m.matches(&mock));

	REQUIRE(m.parse("AAAA <= 12345"));
	REQUIRE(m.matches(&mock));
}

TEST_CASE("Operator `between` checks if field's value is in given range",
	"[Matcher]")
{
	MatcherMockMatchable mock({{"AAAA", "12345"}});
	Matcher m;

	REQUIRE(m.parse("AAAA between 0:12345"));
	REQUIRE(m.matches(&mock));

	REQUIRE(m.parse("AAAA between 12345:12345"));
	REQUIRE(m.matches(&mock));

	REQUIRE(m.parse("AAAA between 23:12344"));
	REQUIRE_FALSE(m.matches(&mock));

	REQUIRE(m.parse("AAAA between 0"));
	REQUIRE_FALSE(m.matches(&mock));

	REQUIRE(m.parse("AAAA between 12346:12344"));
	REQUIRE(m.matches(&mock));
}

TEST_CASE("Invalid expression results in parsing error", "[Matcher]")
{
	Matcher m;

	REQUIRE_FALSE(m.parse("AAAA between 0:15:30"));
}

TEST_CASE("get_expression() returns previously parsed expression", "[Matcher]")
{
	Matcher m2("AAAA between 1:30000");
	REQUIRE(m2.get_expression() == "AAAA between 1:30000");
}

TEST_CASE("Regexes are matched case-insensitively", "[Matcher]")
{
	// Inspired by https://github.com/newsboat/newsboat/issues/642

	const auto require_matches = [](std::string regex) {
		MatcherMockMatchable mock({{"abcd", "xyz"}});
		Matcher m;

		REQUIRE(m.parse("abcd =~ \"" + regex + "\""));
		REQUIRE(m.matches(&mock));
	};

	require_matches("xyz");
	require_matches("xYz");
	require_matches("xYZ");
	require_matches("Xyz");
	require_matches("yZ");
	require_matches("^xYZ");
	require_matches("xYz$");
	require_matches("^Xyz$");
	require_matches("^xY");
	require_matches("yZ$");
}

TEST_CASE("=~ and !~ use POSIX extended regex syntax", "[Matcher]")
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

	Matcher m;

	// Supported by Perl, PCRE, PHP and others
	SECTION("No support for escape sequence") {
		MatcherMockMatchable mock({{"attr", "*]+"}});

		REQUIRE(m.parse(R"#(attr =~ "\Q*]+\E")#"));
		REQUIRE_FALSE(m.matches(&mock));

		REQUIRE(m.parse(R"#(attr !~ "\Q*]+\E")#"));
		REQUIRE(m.matches(&mock));
	}

	SECTION("No support for hexadecimal escape") {
		MatcherMockMatchable mock({{"attr", "value"}});

		REQUIRE(m.parse(R"#(attr =~ "^va\x6Cue")#"));
		REQUIRE_FALSE(m.matches(&mock));

		REQUIRE(m.parse(R"#(attr !~ "^va\x6Cue")#"));
		REQUIRE(m.matches(&mock));
	}

	SECTION("No support for \\a as alert/bell control character") {
		MatcherMockMatchable mock({{"attr", "\x07"}});

		REQUIRE(m.parse(R"#(attr =~ "\a")#"));
		REQUIRE_FALSE(m.matches(&mock));

		REQUIRE(m.parse(R"#(attr !~ "\a")#"));
		REQUIRE(m.matches(&mock));
	}

	SECTION("No support for \\b as backspace control character") {
		MatcherMockMatchable mock({{"attr", "\x08"}});

		REQUIRE(m.parse(R"#(attr =~ "\b")#"));
		REQUIRE_FALSE(m.matches(&mock));

		REQUIRE(m.parse(R"#(attr !~ "\b")#"));
		REQUIRE(m.matches(&mock));
	}

	// If you add more checks to this test, consider adding the same to RegexManager tests
}

TEST_CASE("get_parse_error() returns textual description of last "
	"filter-expression parsing error",
	"[Matcher]")
{
	Matcher m;

	REQUIRE_FALSE(m.parse("=!"));
	REQUIRE(m.get_parse_error() != "");
}

TEST_CASE("Space characters in filter expression don't affect parsing",
	"[Matcher]")
{
	const auto check = [](std::string expression) {
		INFO("input expression: " << expression);

		MatcherMockMatchable mock({{"array", "foo bar baz"}});
		Matcher m;

		REQUIRE(m.parse(expression));
		REQUIRE(m.matches(&mock));
	};

	check("array # \"bar\"");
	check("   array # \"bar\"");
	check("array   # \"bar\"");
	check("array #   \"bar\"");
	check("array # \"bar\"     ");
	check("array#  \"bar\"  ");
	check("     array         #         \"bar\"      ");
}

TEST_CASE("Only space characters are considered whitespace by filter parser",
	"[Matcher]")
{
	const auto check = [](std::string expression) {
		INFO("input expression: " << expression);

		MatcherMockMatchable mock({{"attr", "value"}});
		Matcher m;

		REQUIRE_FALSE(m.parse(expression));
	};

	check("attr\t= \"value\"");
	check("attr =\t\"value\"");
	check("attr\n=\t\"value\"");
	check("attr\v=\"value\"");
	check("attr=\"value\"\r\n");
}

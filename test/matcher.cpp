#include "matcher.h"

#include "3rd-party/catch.hpp"

#include <map>

#include "matchable.h"
#include "matcherexception.h"
#include "test_helpers/stringmaker/optional.h"

using namespace Newsboat;

class MatcherMockMatchable : public Matchable {
public:
	MatcherMockMatchable() = default;

	MatcherMockMatchable(
		std::initializer_list<std::pair<const std::string, std::string>>
		data)
		: m_data(data)
	{}

	std::optional<std::string> attribute_value(const std::string& attribname)
	const override
	{
		const auto it = m_data.find(attribname);
		if (it != m_data.cend()) {
			return it->second;
		}

		return std::nullopt;
	}

private:
	std::map<std::string, std::string> m_data;
};

TEST_CASE("Operator `=` checks if field has given value", "[Matcher]")
{
	Matcher m;

	SECTION("Works with strings") {
		MatcherMockMatchable mock({{"abcd", "xyz"}});

		REQUIRE(m.parse("abcd = \"xyz\""));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("abcd = \"uiop\""));
		REQUIRE_FALSE(m.matches(&mock));
	}

	SECTION("Works with numbers") {
		MatcherMockMatchable mock({{"answer", "42"}});

		REQUIRE(m.parse("answer = 42"));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("answer = 0042"));
		REQUIRE_FALSE(m.matches(&mock));

		REQUIRE(m.parse("answer = 13"));
		REQUIRE_FALSE(m.matches(&mock));

		SECTION("...but converts arguments to strings to compare") {
			MatcherMockMatchable mock2({{"agent", "007"}});

			REQUIRE(m.parse("agent = 7"));
			REQUIRE_FALSE(m.matches(&mock2));

			REQUIRE(m.parse("agent = 007"));
			REQUIRE(m.matches(&mock2));
		}
	}

	SECTION("Doesn't work with ranges") {
		MatcherMockMatchable mock({{"answer", "42"}});

		REQUIRE(m.parse("answer = 0:100"));
		REQUIRE_FALSE(m.matches(&mock));

		REQUIRE(m.parse("answer = 100:200"));
		REQUIRE_FALSE(m.matches(&mock));

		REQUIRE(m.parse("answer = 42:200"));
		REQUIRE_FALSE(m.matches(&mock));

		REQUIRE(m.parse("answer = 0:42"));
		REQUIRE_FALSE(m.matches(&mock));
	}
}

TEST_CASE("Operator `!=` checks if field doesn't have given value", "[Matcher]")
{
	Matcher m;

	SECTION("Works with strings") {
		MatcherMockMatchable mock({{"abcd", "xyz"}});

		REQUIRE(m.parse("abcd != \"uiop\""));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("abcd != \"xyz\""));
		REQUIRE_FALSE(m.matches(&mock));
	}

	SECTION("Works with numbers") {
		MatcherMockMatchable mock({{"answer", "42"}});

		REQUIRE(m.parse("answer != 13"));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("answer != 42"));
		REQUIRE_FALSE(m.matches(&mock));

		SECTION("...but converts arguments to strings to compare") {
			MatcherMockMatchable mock2({{"agent", "007"}});

			REQUIRE(m.parse("agent != 7"));
			REQUIRE(m.matches(&mock2));

			REQUIRE(m.parse("agent != 007"));
			REQUIRE_FALSE(m.matches(&mock2));
		}
	}

	SECTION("Doesn't work with ranges") {
		MatcherMockMatchable mock({{"answer", "42"}});

		REQUIRE(m.parse("answer != 0:100"));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("answer != 100:200"));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("answer != 42:200"));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("answer != 0:42"));
		REQUIRE(m.matches(&mock));
	}
}

TEST_CASE("Operator `=~` checks if field matches given regex", "[Matcher]")
{
	Matcher m;

	SECTION("Works with strings") {
		MatcherMockMatchable mock({{"AAAA", "12345"}});

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

	SECTION("Converts numbers to strings and uses them as regexes") {
		MatcherMockMatchable mock({{"AAAA", "12345"}});

		REQUIRE(m.parse("AAAA =~ 12345"));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("AAAA =~ 1"));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("AAAA =~ 45"));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("AAAA =~ 9"));
		REQUIRE_FALSE(m.matches(&mock));
	}

	SECTION("Treats ranges as strings") {
		MatcherMockMatchable mock({{"AAAA", "12345"}, {"range", "0:123"}});

		REQUIRE(m.parse("AAAA =~ 0:123456"));
		REQUIRE_FALSE(m.matches(&mock));

		REQUIRE(m.parse("AAAA =~ 12345:99999"));
		REQUIRE_FALSE(m.matches(&mock));

		REQUIRE(m.parse("AAAA =~ 0:12345"));
		REQUIRE_FALSE(m.matches(&mock));

		REQUIRE(m.parse("range =~ 0:123"));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("range =~ 0:12"));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("range =~ 0:1234"));
		REQUIRE_FALSE(m.matches(&mock));
	}
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
	Matcher m;

	SECTION("Works with strings") {
		MatcherMockMatchable mock({{"AAAA", "12345"}});

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

		REQUIRE(m.parse("AAAA !~ \"567\""));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("AAAA !~ \"number\""));
		REQUIRE(m.matches(&mock));
	}

	SECTION("Converts numbers into strings and uses them as regexes") {
		MatcherMockMatchable mock({{"AAAA", "12345"}});

		REQUIRE(m.parse("AAAA !~ 12345"));
		REQUIRE_FALSE(m.matches(&mock));

		REQUIRE(m.parse("AAAA !~ 1"));
		REQUIRE_FALSE(m.matches(&mock));

		REQUIRE(m.parse("AAAA !~ 45"));
		REQUIRE_FALSE(m.matches(&mock));

		REQUIRE(m.parse("AAAA !~ 9"));
		REQUIRE(m.matches(&mock));
	}

	SECTION("Doesn't work with ranges") {
		MatcherMockMatchable mock({{"AAAA", "12345"}});

		REQUIRE(m.parse("AAAA !~ 0:123456"));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("AAAA !~ 12345:99999"));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("AAAA !~ 0:12345"));
		REQUIRE(m.matches(&mock));
	}
}

TEST_CASE("Operator `#` checks if space-separated list contains given value",
	"[Matcher]")
{
	Matcher m;

	SECTION("Works with strings") {
		MatcherMockMatchable mock({{"tags", "foo bar baz quux"}});

		REQUIRE(m.parse("tags # \"foo\""));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("tags # \"baz\""));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("tags # \"quux\""));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("tags # \"uu\""));
		REQUIRE_FALSE(m.matches(&mock));

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

	SECTION("Works with numbers") {
		MatcherMockMatchable mock({{"fibonacci", "1 1 2 3 5 8 13 21 34"}});

		REQUIRE(m.parse("fibonacci # 1"));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("fibonacci # 3"));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("fibonacci # 4"));
		REQUIRE_FALSE(m.matches(&mock));

		SECTION("...but convers them to strings to look them up") {
			REQUIRE(m.parse("fibonacci # \"1\""));
			REQUIRE(m.matches(&mock));

			REQUIRE(m.parse("fibonacci # \"3\""));
			REQUIRE(m.matches(&mock));

			REQUIRE(m.parse("fibonacci # \"4\""));
			REQUIRE_FALSE(m.matches(&mock));
		}
	}

	SECTION("Doesn't work with ranges") {
		MatcherMockMatchable mock({{"fibonacci", "1 1 2 3 5 8 13 21 34"}});

		REQUIRE(m.parse("fibonacci # 1:5"));
		REQUIRE_FALSE(m.matches(&mock));

		REQUIRE(m.parse("fibonacci # 3:100"));
		REQUIRE_FALSE(m.matches(&mock));
	}

	SECTION("Works even on single-value lists") {
		MatcherMockMatchable mock({{"values", "one"}, {"number", "1"}});

		REQUIRE(m.parse("values # \"one\""));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("number # 1"));
		REQUIRE(m.matches(&mock));
	}
}

TEST_CASE("Operator `!#` checks if field doesn't contain given value",
	"[Matcher]")
{
	Matcher m;

	SECTION("Works with strings") {
		MatcherMockMatchable mock({{"tags", "foo bar baz quux"}});

		REQUIRE(m.parse("tags !# \"nein\""));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("tags !# \"foo\""));
		REQUIRE_FALSE(m.matches(&mock));
	}

	SECTION("Works with numbers") {
		MatcherMockMatchable mock({{"fibonacci", "1 1 2 3 5 8 13 21 34"}});

		REQUIRE(m.parse("fibonacci !# 1"));
		REQUIRE_FALSE(m.matches(&mock));

		REQUIRE(m.parse("fibonacci !# 9"));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("fibonacci !# 4"));
		REQUIRE(m.matches(&mock));

		SECTION("...but convers them to strings to look them up") {
			REQUIRE(m.parse("fibonacci !# \"1\""));
			REQUIRE_FALSE(m.matches(&mock));

			REQUIRE(m.parse("fibonacci !# \"9\""));
			REQUIRE(m.matches(&mock));

			REQUIRE(m.parse("fibonacci !# \"4\""));
			REQUIRE(m.matches(&mock));
		}
	}

	SECTION("Doesn't work with ranges") {
		MatcherMockMatchable mock({{"fibonacci", "1 1 2 3 5 8 13 21 34"}});

		REQUIRE(m.parse("fibonacci !# 1:5"));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("fibonacci !# 7:35"));
		REQUIRE(m.matches(&mock));
	}

	SECTION("Works even on single-value lists") {
		MatcherMockMatchable mock({{"values", "one"}, {"number", "1"}});

		REQUIRE(m.parse("values !# \"one\""));
		REQUIRE_FALSE(m.matches(&mock));

		REQUIRE(m.parse("values !# \"two\""));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("number !# 1"));
		REQUIRE_FALSE(m.matches(&mock));

		REQUIRE(m.parse("number !# 2"));
		REQUIRE(m.matches(&mock));
	}
}

TEST_CASE(
	"Operators `>`, `>=`, `<` and `<=` compare field's value to given "
	"value",
	"[Matcher]")
{
	Matcher m;

	SECTION("With string arguments, converts arguments to numbers") {
		MatcherMockMatchable mock({{"AAAA", "12345"}});

		SECTION(">") {
			REQUIRE(m.parse("AAAA > \"12344\""));
			REQUIRE(m.matches(&mock));

			REQUIRE(m.parse("AAAA > \"12345\""));
			REQUIRE_FALSE(m.matches(&mock));

			REQUIRE(m.parse("AAAA > \"123456\""));
			REQUIRE_FALSE(m.matches(&mock));
		}

		SECTION("<") {
			REQUIRE(m.parse("AAAA < \"12345\""));
			REQUIRE_FALSE(m.matches(&mock));

			REQUIRE(m.parse("AAAA < \"12346\""));
			REQUIRE(m.matches(&mock));

			REQUIRE(m.parse("AAAA < \"123456\""));
			REQUIRE(m.matches(&mock));
		}

		SECTION(">=") {
			REQUIRE(m.parse("AAAA >= \"12344\""));
			REQUIRE(m.matches(&mock));

			REQUIRE(m.parse("AAAA >= \"12345\""));
			REQUIRE(m.matches(&mock));

			REQUIRE(m.parse("AAAA >= \"12346\""));
			REQUIRE_FALSE(m.matches(&mock));
		}

		SECTION("<=") {
			REQUIRE(m.parse("AAAA <= \"12344\""));
			REQUIRE_FALSE(m.matches(&mock));

			REQUIRE(m.parse("AAAA <= \"12345\""));
			REQUIRE(m.matches(&mock));

			REQUIRE(m.parse("AAAA <= \"12346\""));
			REQUIRE(m.matches(&mock));
		}

		SECTION("Only numeric prefix is used for conversion") {
			MatcherMockMatchable mock2({{"AAAA", "12345xx"}});

			REQUIRE(m.parse("AAAA >= \"12345\""));
			REQUIRE(m.matches(&mock2));

			REQUIRE(m.parse("AAAA > \"1234a\""));
			REQUIRE(m.matches(&mock2));

			REQUIRE(m.parse("AAAA < \"12345a\""));
			REQUIRE_FALSE(m.matches(&mock2));

			REQUIRE(m.parse("AAAA < \"1234a\""));
			REQUIRE_FALSE(m.matches(&mock2));

			REQUIRE(m.parse("AAAA < \"9999b\""));
			REQUIRE_FALSE(m.matches(&mock2));
		}

		SECTION("If conversion fails, zero is used") {
			MatcherMockMatchable mock2({{"zero", "0"}, {"same_zero", "yeah"}});

			REQUIRE(m.parse("zero < \"unknown\""));
			REQUIRE_FALSE(m.matches(&mock2));

			REQUIRE(m.parse("zero > \"unknown\""));
			REQUIRE_FALSE(m.matches(&mock2));

			REQUIRE(m.parse("zero <= \"unknown\""));
			REQUIRE(m.matches(&mock2));

			REQUIRE(m.parse("zero >= \"unknown\""));
			REQUIRE(m.matches(&mock2));

			REQUIRE(m.parse("same_zero < \"0\""));
			REQUIRE_FALSE(m.matches(&mock2));

			REQUIRE(m.parse("same_zero > \"0\""));
			REQUIRE_FALSE(m.matches(&mock2));

			REQUIRE(m.parse("same_zero <= \"0\""));
			REQUIRE(m.matches(&mock2));

			REQUIRE(m.parse("same_zero >= \"0\""));
			REQUIRE(m.matches(&mock2));
		}
	}

	SECTION("Work with numbers") {
		MatcherMockMatchable mock({{"AAAA", "12345"}});

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

	SECTION("Don't work with ranges") {
		MatcherMockMatchable mock({{"AAAA", "12345"}});

		REQUIRE(m.parse("AAAA > 0:99999"));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("AAAA < 0:99999"));
		REQUIRE_FALSE(m.matches(&mock));

		REQUIRE(m.parse("AAAA >= 0:99999"));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("AAAA <= 0:99999"));
		REQUIRE_FALSE(m.matches(&mock));
	}
}

TEST_CASE("Operator `between` checks if field's value is in given range",
	"[Matcher]")
{
	Matcher m;

	SECTION("Doesn't work with strings") {
		MatcherMockMatchable mock({{"AAAA", "12345"}});

		REQUIRE(m.parse("AAAA between \"123\""));
		REQUIRE_FALSE(m.matches(&mock));

		REQUIRE(m.parse("AAAA between \"12399\""));
		REQUIRE_FALSE(m.matches(&mock));
	}

	SECTION("Doesn't work with numbers") {
		MatcherMockMatchable mock({{"AAAA", "12345"}});

		REQUIRE(m.parse("AAAA between 1"));
		REQUIRE_FALSE(m.matches(&mock));

		REQUIRE(m.parse("AAAA between 12345"));
		REQUIRE_FALSE(m.matches(&mock));

		REQUIRE(m.parse("AAAA between 99999"));
		REQUIRE_FALSE(m.matches(&mock));
	}

	SECTION("Works with ranges") {
		MatcherMockMatchable mock({{"AAAA", "12345"}});

		REQUIRE(m.parse("AAAA between 0:12345"));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("AAAA between 12345:12345"));
		REQUIRE(m.matches(&mock));

		REQUIRE(m.parse("AAAA between 23:12344"));
		REQUIRE_FALSE(m.matches(&mock));

		REQUIRE(m.parse("AAAA between 12346:12344"));
		REQUIRE(m.matches(&mock));

		SECTION("...converting numeric prefix of the attribute if necessary") {
			MatcherMockMatchable mock2({{"value", "123four"}, {"practically_zero", "sure"}});

			REQUIRE(m.parse("value between 122:124"));
			REQUIRE(m.matches(&mock2));

			REQUIRE(m.parse("value between 124:130"));
			REQUIRE_FALSE(m.matches(&mock2));

			REQUIRE(m.parse("practically_zero between 0:1"));
			REQUIRE(m.matches(&mock2));

			REQUIRE(m.parse("practically_zero between 1:100"));
			REQUIRE_FALSE(m.matches(&mock2));
		}
	}
}

TEST_CASE("get_expression() returns previously parsed expression", "[Matcher]")
{
	Matcher m("AAAA between 1:30000");
	REQUIRE(m.get_expression() == "AAAA between 1:30000");

	SECTION("after parse(), get_expression() returns the latest parsed expression") {
		REQUIRE(m.parse("AAAA == 42"));
		REQUIRE(m.get_expression() == "AAAA == 42");
	}
}

TEST_CASE("Regexes are matched case-insensitively", "[Matcher]")
{
	// Inspired by https://github.com/Newsboat/Newsboat/issues/642

	const auto require_matches = [](const std::string& regex) {
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
	// FreeBSD 13 returns an error from regexec() instead of failing the match
	// like other OSes do. Furthermore, REQUIRE() and REQUIRE_FALSE() will
	// handle any exceptions that are thrown, so we have to save the result
	// into an intermediate variable to get a chance to handle the exception
	// ourselves.
	//
	// Ideas gleaned from https://www.regular-expressions.info/refcharacters.html

	Matcher m;

	// Supported by Perl, PCRE, PHP and others
	SECTION("No support for escape sequence") {
		MatcherMockMatchable mock({{"attr", "*]+"}});

		REQUIRE(m.parse(R"#(attr =~ "\Q*]+\E")#"));
		try {
			const auto result = m.matches(&mock);
			REQUIRE_FALSE(result);
		} catch (const MatcherException& e) {
			REQUIRE(e.type() == MatcherException::Type::INVALID_REGEX);
		}

		REQUIRE(m.parse(R"#(attr !~ "\Q*]+\E")#"));
		try {
			const auto result = m.matches(&mock);
			REQUIRE(result);
		} catch (const MatcherException& e) {
			REQUIRE(e.type() == MatcherException::Type::INVALID_REGEX);
		}
	}

	SECTION("No support for hexadecimal escape") {
		MatcherMockMatchable mock({{"attr", "value"}});

		REQUIRE(m.parse(R"#(attr =~ "^va\x6Cue")#"));
		try {
			const auto result = m.matches(&mock);
			REQUIRE_FALSE(result);
		} catch (const MatcherException& e) {
			REQUIRE(e.type() == MatcherException::Type::INVALID_REGEX);
		}

		REQUIRE(m.parse(R"#(attr !~ "^va\x6Cue")#"));
		try {
			const auto result = m.matches(&mock);
			REQUIRE(result);
		} catch (const MatcherException& e) {
			REQUIRE(e.type() == MatcherException::Type::INVALID_REGEX);
		}
	}

	SECTION("No support for \\a as alert/bell control character") {
		MatcherMockMatchable mock({{"attr", "\x07"}});

		REQUIRE(m.parse(R"#(attr =~ "\a")#"));
		try {
			const auto result = m.matches(&mock);
			REQUIRE_FALSE(result);
		} catch (const MatcherException& e) {
			REQUIRE(e.type() == MatcherException::Type::INVALID_REGEX);
		}

		REQUIRE(m.parse(R"#(attr !~ "\a")#"));
		try {
			const auto result = m.matches(&mock);
			REQUIRE(result);
		} catch (const MatcherException& e) {
			REQUIRE(e.type() == MatcherException::Type::INVALID_REGEX);
		}
	}

	SECTION("No support for \\b as backspace control character") {
		MatcherMockMatchable mock({{"attr", "\x08"}});

		REQUIRE(m.parse(R"#(attr =~ "\b")#"));
		try {
			const auto result = m.matches(&mock);
			REQUIRE_FALSE(result);
		} catch (const MatcherException& e) {
			REQUIRE(e.type() == MatcherException::Type::INVALID_REGEX);
		}

		REQUIRE(m.parse(R"#(attr !~ "\b")#"));
		try {
			const auto result = m.matches(&mock);
			REQUIRE(result);
		} catch (const MatcherException& e) {
			REQUIRE(e.type() == MatcherException::Type::INVALID_REGEX);
		}
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
	const auto check = [](const std::string& expression) {
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
	const auto check = [](const std::string& expression) {
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

TEST_CASE("Whitespace before and/or is not required", "[Matcher]")
{
	MatcherMockMatchable mock({{"x", "42"}, {"y", "0"}});
	Matcher m;

	REQUIRE(m.parse("x = 42and y=0"));
	REQUIRE(m.matches(&mock));

	REQUIRE(m.parse("x = \"42\"and y=0"));
	REQUIRE(m.matches(&mock));

	REQUIRE(m.parse("x = \"42\"or y=42"));
	REQUIRE(m.matches(&mock));
}

TEST_CASE("string_to_num() converts numeric prefix of the string to int",
	"[Matcher]")
{
	REQUIRE(Matcher::string_to_num("7654") == 7654);
	REQUIRE(Matcher::string_to_num("123foo") == 123);
	REQUIRE(Matcher::string_to_num("-999999bar") == -999999);

	REQUIRE(Matcher::string_to_num("-2147483648min") == -2147483648);
	REQUIRE(Matcher::string_to_num("2147483647 is ok") == 2147483647);

	// On under-/over-flow, returns min/max representable value
	REQUIRE(Matcher::string_to_num("-2147483649 is too small for i32") ==
		-2147483648);
	REQUIRE(Matcher::string_to_num("2147483648 is too large for i32") ==
		2147483647);
}

TEST_CASE("string_to_num() returns 0 if there is no numeric prefix",
	"[Matcher]")
{
	REQUIRE(Matcher::string_to_num("hello") == 0);
	REQUIRE(Matcher::string_to_num("") == 0);
}

#include "catch.hpp"

#include <matcher.h>
#include <exceptions.h>

using namespace newsbeuter;

struct testmatchable : public matchable {
	virtual bool has_attribute(const std::string& attribname) {
		if (attribname == "abcd" || attribname == "AAAA" || attribname == "tags")
			return true;
		return false;
	}

	virtual std::string get_attribute(const std::string& attribname) {
		if (attribname == "abcd")
			return "xyz";
		if (attribname == "AAAA")
			return "12345";
		if (attribname == "tags")
			return "foo bar baz quux";
		return "";
	}
};

TEST_CASE("Operator `=` checks if field has given value", "[matcher]") {
	testmatchable mock;
	matcher m;

	m.parse("abcd = \"xyz\"");
	// abcd = xyz matches
	REQUIRE(m.matches(&mock));

	m.parse("abcd = \"uiop\"");
	// "abcd = uiop doesn't match"
	REQUIRE_FALSE(m.matches(&mock));
}

TEST_CASE("Operator `!=` checks if field doesn't have given value",
          "[matcher]")
{
	testmatchable mock;
	matcher m;

	m.parse("abcd != \"uiop\"");
	// abcd != uiop matches
	REQUIRE(m.matches(&mock));

	m.parse("abcd != \"xyz\"");
	// "abcd != xyz doesn't match"
	REQUIRE_FALSE(m.matches(&mock));
}

TEST_CASE("Operator `=~` checks if field matches given regex", "[matcher]") {
	testmatchable mock;
	matcher m;

	m.parse("AAAA =~ \".\"");
	// AAAA =~ . matches
	REQUIRE(m.matches(&mock));

	m.parse("AAAA =~ \"123\"");
	// AAAA =~ 123 matches
	REQUIRE(m.matches(&mock));

	m.parse("AAAA =~ \"234\"");
	// AAAA =~ 234 matches
	REQUIRE(m.matches(&mock));

	m.parse("AAAA =~ \"45\"");
	// AAAA =~ 45 matches
	REQUIRE(m.matches(&mock));

	m.parse("AAAA =~ \"^12345$\"");
	// AAAA =~ ^12345$ matches
	REQUIRE(m.matches(&mock));

	m.parse("AAAA =~ \"^123456$\"");
	// "AAAA =~ ^123456$ doesn't match"
	REQUIRE_FALSE(m.matches(&mock));
}

TEST_CASE("Matcher throws if expression contains undefined fields", "[matcher]") {
	testmatchable mock;
	matcher m;

	m.parse("BBBB =~ \"foo\"");
	// attribute BBBB was detected as non-existent
	REQUIRE_THROWS_AS(m.matches(&mock), matcherexception);

	m.parse("BBBB # \"foo\"");
	REQUIRE_THROWS_AS(m.matches(&mock), matcherexception);

	m.parse("BBBB < 0");
	REQUIRE_THROWS_AS(m.matches(&mock), matcherexception);

	m.parse("BBBB > 0");
	REQUIRE_THROWS_AS(m.matches(&mock), matcherexception);

	m.parse("BBBB between 1:23");
	REQUIRE_THROWS_AS(m.matches(&mock), matcherexception);

}

TEST_CASE("Matcher throws if regex passed to `=~` or `!~` is invalid",
          "[matcher]")
{
	testmatchable mock;
	matcher m;

	m.parse("AAAA =~ \"[[\"");
	REQUIRE_THROWS_AS(m.matches(&mock), matcherexception);

	m.parse("AAAA !~ \"[[\"");
	REQUIRE_THROWS_AS(m.matches(&mock), matcherexception);
}

TEST_CASE("Operator `!~` checks if field doesn't match given regex",
          "[matcher]")
{
	testmatchable mock;
	matcher m;

	m.parse("AAAA !~ \".\"");
	// "AAAA !~ . doesn't match"
	REQUIRE_FALSE(m.matches(&mock));

	m.parse("AAAA !~ \"123\"");
	// AAAA !~ 123 doesn't match
	REQUIRE_FALSE(m.matches(&mock));

	m.parse("AAAA !~ \"234\"");
	// AAAA !~ 234 doesn't match
	REQUIRE_FALSE(m.matches(&mock));

	m.parse("AAAA !~ \"45\"");
	// AAAA !~ 45 doesn't match
	REQUIRE_FALSE(m.matches(&mock));

	m.parse("AAAA !~ \"^12345$\"");
	// AAAA !~ ^12345$ doesn't match
	REQUIRE_FALSE(m.matches(&mock));
}

TEST_CASE("Operator `#` checks if \"tags\" field contains given value",
          "[matcher]")
{
	testmatchable mock;
	matcher m;

	m.parse("tags # \"foo\"");
	// tags # foo matches
	REQUIRE(m.matches(&mock));

	m.parse("tags # \"baz\"");
	// tags # baz matches
	REQUIRE(m.matches(&mock));

	m.parse("tags # \"quux\"");
	// tags # quux matches
	REQUIRE(m.matches(&mock));

	m.parse("tags # \"xyz\"");
	// tags # xyz doesn't match
	REQUIRE_FALSE(m.matches(&mock));

	m.parse("tags # \"foo bar\"");
	// tags # foo bar doesn't match
	REQUIRE_FALSE(m.matches(&mock));

	m.parse("tags # \"foo\" and tags # \"bar\"");
	// tags # foo and tags # bar matches
	REQUIRE(m.matches(&mock));

	m.parse("tags # \"foo\" and tags # \"xyz\"");
	// tags # foo and tags # xyz doesn't match
	REQUIRE_FALSE(m.matches(&mock));

	m.parse("tags # \"foo\" or tags # \"xyz\"");
	// tags # foo or tags # xyz matches
	REQUIRE(m.matches(&mock));
}

TEST_CASE("Operator `!#` checks if \"tags\" field doesn't contain given value",
          "[matcher]")
{
	testmatchable mock;
	matcher m;

	m.parse("tags !# \"nein\"");
	// tags !# nein matches
	REQUIRE(m.matches(&mock));

	m.parse("tags !# \"foo\"");
	// tags !# foo doesn't match
	REQUIRE_FALSE(m.matches(&mock));
}

TEST_CASE("Operators `>`, `>=`, `<` and `<=` comparee field's value to given "
          "value", "[matcher]")
{
	testmatchable mock;
	matcher m;

	m.parse("AAAA > 12344");
	// AAAA > 12344 matches
	REQUIRE(m.matches(&mock));

	m.parse("AAAA > 12345");
	// AAAA > 12345 doesn't match
	REQUIRE_FALSE(m.matches(&mock));

	m.parse("AAAA >= 12345");
	// AAAA >= 12345 matches
	REQUIRE(m.matches(&mock));

	m.parse("AAAA < 12345");
	// AAAA < 12345 doesn't match
	REQUIRE_FALSE(m.matches(&mock));

	m.parse("AAAA <= 12345");
	// AAAA <= 12345 matches
	REQUIRE(m.matches(&mock));
}

TEST_CASE("Operator `between` checks if field's value is in given range",
          "[matcher]")
{
	testmatchable mock;
	matcher m;

	m.parse("AAAA between 0:12345");
	// AAAA between 0:12345 matches
	REQUIRE(m.get_parse_error() == "");
	REQUIRE(m.matches(&mock));

	m.parse("AAAA between 12345:12345");
	// AAAA between 12345:12345 matches
	REQUIRE(m.matches(&mock));

	m.parse("AAAA between 23:12344");
	// AAAA between 23:12344 doesn't match
	REQUIRE_FALSE(m.matches(&mock));

	m.parse("AAAA between 0");
	// invalid between expression (1)
	REQUIRE_FALSE(m.matches(&mock));

	m.parse("AAAA between 12346:12344");
	// reverse ranges will match, too
	REQUIRE(m.matches(&mock));
}

TEST_CASE("Invalid expression results in parsing error", "[matcher]") {
	matcher m;

	// invalid between expression won't be parsed
	REQUIRE_FALSE(m.parse("AAAA between 0:15:30"));
	// invalid between expression leads to parse error
	REQUIRE(m.get_parse_error() != "");
}

TEST_CASE("get_expression() returns previously parsed expression",
          "[matcher]")
{
	matcher m2("AAAA between 1:30000");
	REQUIRE(m2.get_expression() == "AAAA between 1:30000");
}

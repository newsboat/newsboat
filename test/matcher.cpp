#include "matcher.h"

#include "3rd-party/catch.hpp"

#include "exceptions.h"

using namespace newsboat;

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
	REQUIRE(m.matches(&mock));

	m.parse("abcd = \"uiop\"");
	REQUIRE_FALSE(m.matches(&mock));
}

TEST_CASE("Operator `!=` checks if field doesn't have given value",
          "[matcher]")
{
	testmatchable mock;
	matcher m;

	m.parse("abcd != \"uiop\"");
	REQUIRE(m.matches(&mock));

	m.parse("abcd != \"xyz\"");
	REQUIRE_FALSE(m.matches(&mock));
}

TEST_CASE("Operator `=~` checks if field matches given regex", "[matcher]") {
	testmatchable mock;
	matcher m;

	m.parse("AAAA =~ \".\"");
	REQUIRE(m.matches(&mock));

	m.parse("AAAA =~ \"123\"");
	REQUIRE(m.matches(&mock));

	m.parse("AAAA =~ \"234\"");
	REQUIRE(m.matches(&mock));

	m.parse("AAAA =~ \"45\"");
	REQUIRE(m.matches(&mock));

	m.parse("AAAA =~ \"^12345$\"");
	REQUIRE(m.matches(&mock));

	m.parse("AAAA =~ \"^123456$\"");
	REQUIRE_FALSE(m.matches(&mock));
}

TEST_CASE("Matcher throws if expression contains undefined fields", "[matcher]") {
	testmatchable mock;
	matcher m;

	m.parse("BBBB =~ \"foo\"");
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
	REQUIRE_FALSE(m.matches(&mock));

	m.parse("AAAA !~ \"123\"");
	REQUIRE_FALSE(m.matches(&mock));

	m.parse("AAAA !~ \"234\"");
	REQUIRE_FALSE(m.matches(&mock));

	m.parse("AAAA !~ \"45\"");
	REQUIRE_FALSE(m.matches(&mock));

	m.parse("AAAA !~ \"^12345$\"");
	REQUIRE_FALSE(m.matches(&mock));
}

TEST_CASE("Operator `#` checks if \"tags\" field contains given value",
          "[matcher]")
{
	testmatchable mock;
	matcher m;

	m.parse("tags # \"foo\"");
	REQUIRE(m.matches(&mock));

	m.parse("tags # \"baz\"");
	REQUIRE(m.matches(&mock));

	m.parse("tags # \"quux\"");
	REQUIRE(m.matches(&mock));

	m.parse("tags # \"xyz\"");
	REQUIRE_FALSE(m.matches(&mock));

	m.parse("tags # \"foo bar\"");
	REQUIRE_FALSE(m.matches(&mock));

	m.parse("tags # \"foo\" and tags # \"bar\"");
	REQUIRE(m.matches(&mock));

	m.parse("tags # \"foo\" and tags # \"xyz\"");
	REQUIRE_FALSE(m.matches(&mock));

	m.parse("tags # \"foo\" or tags # \"xyz\"");
	REQUIRE(m.matches(&mock));
}

TEST_CASE("Operator `!#` checks if \"tags\" field doesn't contain given value",
          "[matcher]")
{
	testmatchable mock;
	matcher m;

	m.parse("tags !# \"nein\"");
	REQUIRE(m.matches(&mock));

	m.parse("tags !# \"foo\"");
	REQUIRE_FALSE(m.matches(&mock));
}

TEST_CASE("Operators `>`, `>=`, `<` and `<=` comparee field's value to given "
          "value", "[matcher]")
{
	testmatchable mock;
	matcher m;

	m.parse("AAAA > 12344");
	REQUIRE(m.matches(&mock));

	m.parse("AAAA > 12345");
	REQUIRE_FALSE(m.matches(&mock));

	m.parse("AAAA >= 12345");
	REQUIRE(m.matches(&mock));

	m.parse("AAAA < 12345");
	REQUIRE_FALSE(m.matches(&mock));

	m.parse("AAAA <= 12345");
	REQUIRE(m.matches(&mock));
}

TEST_CASE("Operator `between` checks if field's value is in given range",
          "[matcher]")
{
	testmatchable mock;
	matcher m;

	m.parse("AAAA between 0:12345");
	REQUIRE(m.get_parse_error() == "");
	REQUIRE(m.matches(&mock));

	m.parse("AAAA between 12345:12345");
	REQUIRE(m.matches(&mock));

	m.parse("AAAA between 23:12344");
	REQUIRE_FALSE(m.matches(&mock));

	m.parse("AAAA between 0");
	REQUIRE_FALSE(m.matches(&mock));

	m.parse("AAAA between 12346:12344");
	REQUIRE(m.matches(&mock));
}

TEST_CASE("Invalid expression results in parsing error", "[matcher]") {
	matcher m;

	REQUIRE_FALSE(m.parse("AAAA between 0:15:30"));
	REQUIRE(m.get_parse_error() != "");
}

TEST_CASE("get_expression() returns previously parsed expression",
          "[matcher]")
{
	matcher m2("AAAA between 1:30000");
	REQUIRE(m2.get_expression() == "AAAA between 1:30000");
}

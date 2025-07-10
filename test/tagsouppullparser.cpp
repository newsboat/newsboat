#include "tagsouppullparser.h"

#include <sstream>

#include "3rd-party/catch.hpp"

using namespace Newsboat;

TEST_CASE("Tagsoup pull parser turns document into a stream of events",
	"[TagSoupPullParser]")
{
	std::istringstream input_stream(
		"<test>"
		"<foo quux='asdf' bar=\"qqq\">text</foo>"
		"more text"
		"<more>&quot;&#33;&#x40;</more>"
		"<xxx foo=bar baz=\"qu ux\" hi='ho ho ho'></xxx>"
		"</test>");

	TagSoupPullParser xpp(input_stream);
	TagSoupPullParser::Event e;

	e = xpp.get_event_type();
	REQUIRE(e == TagSoupPullParser::Event::START_DOCUMENT);

	e = xpp.next();
	REQUIRE(e == TagSoupPullParser::Event::START_TAG);
	REQUIRE(xpp.get_text() == "test");

	e = xpp.next();
	REQUIRE(e == TagSoupPullParser::Event::START_TAG);
	REQUIRE(xpp.get_text() == "foo");
	REQUIRE(xpp.get_attribute_value("quux").value() == "asdf");
	REQUIRE(xpp.get_attribute_value("bar").value() == "qqq");

	e = xpp.next();
	REQUIRE(e == TagSoupPullParser::Event::TEXT);
	REQUIRE(xpp.get_text() == "text");

	e = xpp.next();
	REQUIRE(e == TagSoupPullParser::Event::END_TAG);
	REQUIRE(xpp.get_text() == "foo");

	e = xpp.next();
	REQUIRE(e == TagSoupPullParser::Event::TEXT);
	REQUIRE(xpp.get_text() == "more text");

	e = xpp.next();
	REQUIRE(e == TagSoupPullParser::Event::START_TAG);
	REQUIRE(xpp.get_text() == "more");

	e = xpp.next();
	REQUIRE(e == TagSoupPullParser::Event::TEXT);
	REQUIRE(xpp.get_text() == "\"!@");

	e = xpp.next();
	REQUIRE(e == TagSoupPullParser::Event::END_TAG);
	REQUIRE(xpp.get_text() == "more");

	e = xpp.next();
	REQUIRE(e == TagSoupPullParser::Event::START_TAG);
	REQUIRE(xpp.get_text() == "xxx");
	REQUIRE(xpp.get_attribute_value("foo").value() == "bar");
	REQUIRE(xpp.get_attribute_value("baz").value() == "qu ux");
	REQUIRE(xpp.get_attribute_value("hi").value() == "ho ho ho");

	e = xpp.next();
	REQUIRE(e == TagSoupPullParser::Event::END_TAG);
	REQUIRE(xpp.get_text() == "xxx");

	e = xpp.next();
	REQUIRE(e == TagSoupPullParser::Event::END_TAG);
	REQUIRE(xpp.get_text() == "test");

	e = xpp.next();
	REQUIRE(e == TagSoupPullParser::Event::END_DOCUMENT);

	e = xpp.next();
	REQUIRE(e == TagSoupPullParser::Event::END_DOCUMENT);
}

TEST_CASE("<br>, <br/> and <br /> behave the same way", "[TagSoupPullParser]")
{
	std::istringstream input_stream;
	TagSoupPullParser::Event event;

	for (auto input : {
			"<br>", "<br/>", "<br />"
		}) {
		SECTION(input) {
			input_stream.str(input);
			TagSoupPullParser Parser(input_stream);

			event = Parser.get_event_type();
			REQUIRE(event ==
				TagSoupPullParser::Event::START_DOCUMENT);

			event = Parser.next();
			REQUIRE(event == TagSoupPullParser::Event::START_TAG);
			REQUIRE(Parser.get_text() == "br");

			event = Parser.next();
			REQUIRE(event ==
				TagSoupPullParser::Event::END_DOCUMENT);
		}
	}
}

TEST_CASE("Tagsoup pull parser emits whitespace as is",
	"[TagSoupPullParser]")
{
	std::istringstream input_stream(
		"<test>    &lt;4 spaces\n"
		"<pre>\n"
		"    <span>should have seen spaces</span>"
		"</pre>"
		"</test>");

	TagSoupPullParser xpp(input_stream);
	TagSoupPullParser::Event e;

	e = xpp.get_event_type();
	REQUIRE(e == TagSoupPullParser::Event::START_DOCUMENT);

	e = xpp.next();
	REQUIRE(e == TagSoupPullParser::Event::START_TAG);
	REQUIRE(xpp.get_text() == "test");

	e = xpp.next();
	REQUIRE(e == TagSoupPullParser::Event::TEXT);
	REQUIRE(xpp.get_text() == "    <4 spaces\n");

	e = xpp.next();
	REQUIRE(e == TagSoupPullParser::Event::START_TAG);
	REQUIRE(xpp.get_text() == "pre");

	e = xpp.next();
	REQUIRE(e == TagSoupPullParser::Event::TEXT);
	REQUIRE(xpp.get_text() == "\n    ");

	e = xpp.next();
	REQUIRE(e == TagSoupPullParser::Event::START_TAG);
	REQUIRE(xpp.get_text() == "span");

	e = xpp.next();
	REQUIRE(e == TagSoupPullParser::Event::TEXT);
	REQUIRE(xpp.get_text() == "should have seen spaces");

	e = xpp.next();
	REQUIRE(e == TagSoupPullParser::Event::END_TAG);
	REQUIRE(xpp.get_text() == "span");

	e = xpp.next();
	REQUIRE(e == TagSoupPullParser::Event::END_TAG);
	REQUIRE(xpp.get_text() == "pre");

	e = xpp.next();
	REQUIRE(e == TagSoupPullParser::Event::END_TAG);
	REQUIRE(xpp.get_text() == "test");

	e = xpp.next();
	REQUIRE(e == TagSoupPullParser::Event::END_DOCUMENT);

	e = xpp.next();
	REQUIRE(e == TagSoupPullParser::Event::END_DOCUMENT);
}

TEST_CASE("TagSoupPullParser can decode HTML entities", "[TagSoupPullParser]")
{
	SECTION("Numbered entites") {
		SECTION("Decimal") {
			// 133 designates a horizontal ellipsis
			std::istringstream input_stream("&#020;&#42;&#189;&#133;&#963;");

			TagSoupPullParser xpp(input_stream);
			TagSoupPullParser::Event e;

			e = xpp.get_event_type();
			REQUIRE(e == TagSoupPullParser::Event::START_DOCUMENT);

			e = xpp.next();
			REQUIRE(e == TagSoupPullParser::Event::TEXT);
			REQUIRE(xpp.get_text() == "\x14*½…σ");

			e = xpp.next();
			REQUIRE(e == TagSoupPullParser::Event::END_DOCUMENT);
		}

		SECTION("Hexadecimal") {
			// x97 designates an mdash
			std::istringstream input_stream("&#x97;&#x20;&#x048;&#x0069;");

			TagSoupPullParser xpp(input_stream);
			TagSoupPullParser::Event e;

			e = xpp.get_event_type();
			REQUIRE(e == TagSoupPullParser::Event::START_DOCUMENT);

			e = xpp.next();
			REQUIRE(e == TagSoupPullParser::Event::TEXT);
			REQUIRE(xpp.get_text() == "— Hi");

			e = xpp.next();
			REQUIRE(e == TagSoupPullParser::Event::END_DOCUMENT);
		}

		SECTION("Windows codepoints") {
			std::istringstream input_stream(
				"&#x80;&#x82;&#x83;&#x84;&#x85;&#x86;&#x87;"
				"&#x88;&#x89;&#x8A;&#x8B;&#x8C;&#x8E;&#x91;"
				"&#x92;&#x93;&#x94;&#x95;&#x96;&#x97;&#x98;"
				"&#x99;&#x9A;&#x9B;&#x9C;&#x9E;&#x9F;");

			TagSoupPullParser xpp(input_stream);
			TagSoupPullParser::Event e;

			e = xpp.get_event_type();
			REQUIRE(e == TagSoupPullParser::Event::START_DOCUMENT);

			e = xpp.next();
			REQUIRE(e == TagSoupPullParser::Event::TEXT);
			REQUIRE(xpp.get_text() ==
				"€‚ƒ„…†‡ˆ‰Š‹ŒŽ‘’“”•–—˜™š›œžŸ");

			e = xpp.next();
			REQUIRE(e == TagSoupPullParser::Event::END_DOCUMENT);
		}
	}

	SECTION("Named entities") {
		std::istringstream input_stream("&sigma;&trade;");

		TagSoupPullParser xpp(input_stream);
		TagSoupPullParser::Event e;

		e = xpp.get_event_type();
		REQUIRE(e == TagSoupPullParser::Event::START_DOCUMENT);

		e = xpp.next();
		REQUIRE(e == TagSoupPullParser::Event::TEXT);
		REQUIRE(xpp.get_text() == "σ™");

		e = xpp.next();
		REQUIRE(e == TagSoupPullParser::Event::END_DOCUMENT);
	}
}

TEST_CASE("TagSoupPullParser ignores unknown and invalid entities",
	"[TagSoupPullParser]")
{
	SECTION("Missing semicolon") {
		std::istringstream input_stream("some & text");

		TagSoupPullParser xpp(input_stream);
		TagSoupPullParser::Event e;

		e = xpp.get_event_type();
		REQUIRE(e == TagSoupPullParser::Event::START_DOCUMENT);

		e = xpp.next();
		REQUIRE(e == TagSoupPullParser::Event::TEXT);
		REQUIRE(xpp.get_text() == "some & text");

		e = xpp.next();
		REQUIRE(e == TagSoupPullParser::Event::END_DOCUMENT);
	}

	SECTION("Unknown entity") {
		std::istringstream input_stream("some &more; text");

		TagSoupPullParser xpp(input_stream);
		TagSoupPullParser::Event e;

		e = xpp.get_event_type();
		REQUIRE(e == TagSoupPullParser::Event::START_DOCUMENT);

		e = xpp.next();
		REQUIRE(e == TagSoupPullParser::Event::TEXT);
		REQUIRE(xpp.get_text() == "some &more; text");

		e = xpp.next();
		REQUIRE(e == TagSoupPullParser::Event::END_DOCUMENT);
	}

	SECTION("Valid entities after invalid entities") {
		std::istringstream input_stream("a lone ampersand: &, and some entities: &lt;&gt;");

		TagSoupPullParser xpp(input_stream);
		TagSoupPullParser::Event e;

		e = xpp.get_event_type();
		REQUIRE(e == TagSoupPullParser::Event::START_DOCUMENT);

		e = xpp.next();
		REQUIRE(e == TagSoupPullParser::Event::TEXT);
		REQUIRE(xpp.get_text() == "a lone ampersand: &, and some entities: <>");

		e = xpp.next();
		REQUIRE(e == TagSoupPullParser::Event::END_DOCUMENT);
	}
}

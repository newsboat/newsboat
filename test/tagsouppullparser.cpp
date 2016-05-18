#include "catch.hpp"

#include <tagsouppullparser.h>

using namespace newsbeuter;

TEST_CASE("Tagsoup pull parser behaves properly") {
	std::istringstream input_stream(
			"<test>"
				"<foo quux='asdf' bar=\"qqq\">text</foo>"
				"more text"
				"<more>&quot;&#33;&#x40;</more>"
				"<xxx foo=bar baz=\"qu ux\" hi='ho ho ho'></xxx>"
			"</test>");

	tagsouppullparser xpp;
	tagsouppullparser::event e;
	xpp.setInput(input_stream);

	e = xpp.getEventType();
	REQUIRE(e == tagsouppullparser::START_DOCUMENT);

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::START_TAG);
	REQUIRE(xpp.getText() == "test");

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::START_TAG);
	REQUIRE(xpp.getText() == "foo");
	REQUIRE(xpp.getAttributeValue("quux") == "asdf");
	REQUIRE(xpp.getAttributeValue("bar") == "qqq");

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::TEXT);
	REQUIRE(xpp.getText() == "text");

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::END_TAG);
	REQUIRE(xpp.getText() == "foo");

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::TEXT);
	REQUIRE(xpp.getText() == "more text");

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::START_TAG);
	REQUIRE(xpp.getText() == "more");

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::TEXT);
	REQUIRE(xpp.getText() == "\"!@");

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::END_TAG);
	REQUIRE(xpp.getText() == "more");

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::START_TAG);
	REQUIRE(xpp.getText() == "xxx");
	REQUIRE(xpp.getAttributeValue("foo") == "bar");
	REQUIRE(xpp.getAttributeValue("baz") == "qu ux");
	REQUIRE(xpp.getAttributeValue("hi") == "ho ho ho");

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::END_TAG);
	REQUIRE(xpp.getText() == "xxx");

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::END_TAG);
	REQUIRE(xpp.getText() == "test");

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::END_DOCUMENT);

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::END_DOCUMENT);
}

TEST_CASE("<br>, <br/> and <br /> behave the same way") {
	std::istringstream input_stream;
	tagsouppullparser parser;
	tagsouppullparser::event event;

	for (auto input : {"<br>", "<br/>", "<br />"}) {
		SECTION(input) {
			input_stream.str(input);
			parser.setInput(input_stream);

			event = parser.getEventType();
			REQUIRE(event == tagsouppullparser::START_DOCUMENT);

			event = parser.next();
			REQUIRE(event == tagsouppullparser::START_TAG);
			REQUIRE(parser.getText() == "br");

			event = parser.next();
			REQUIRE(event == tagsouppullparser::END_DOCUMENT);
		}
	}
}

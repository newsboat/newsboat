#include "tagsouppullparser.h"

#include <sstream>

#include "3rd-party/catch.hpp"

using namespace newsboat;

TEST_CASE("Tagsoup pull parser turns document into a stream of events",
	"[tagsouppullparser]")
{
	std::istringstream input_stream(
		"<test>"
		"<foo quux='asdf' bar=\"qqq\">text</foo>"
		"more text"
		"<more>&quot;&#33;&#x40;</more>"
		"<xxx foo=bar baz=\"qu ux\" hi='ho ho ho'></xxx>"
		"</test>");

	tagsouppullparser xpp;
	tagsouppullparser::event e;
	xpp.set_input(input_stream);

	e = xpp.get_event_type();
	REQUIRE(e == tagsouppullparser::event::START_DOCUMENT);

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::event::START_TAG);
	REQUIRE(xpp.get_text() == "test");

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::event::START_TAG);
	REQUIRE(xpp.get_text() == "foo");
	REQUIRE(xpp.get_attribute_value("quux") == "asdf");
	REQUIRE(xpp.get_attribute_value("bar") == "qqq");

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::event::TEXT);
	REQUIRE(xpp.get_text() == "text");

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::event::END_TAG);
	REQUIRE(xpp.get_text() == "foo");

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::event::TEXT);
	REQUIRE(xpp.get_text() == "more text");

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::event::START_TAG);
	REQUIRE(xpp.get_text() == "more");

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::event::TEXT);
	REQUIRE(xpp.get_text() == "\"!@");

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::event::END_TAG);
	REQUIRE(xpp.get_text() == "more");

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::event::START_TAG);
	REQUIRE(xpp.get_text() == "xxx");
	REQUIRE(xpp.get_attribute_value("foo") == "bar");
	REQUIRE(xpp.get_attribute_value("baz") == "qu ux");
	REQUIRE(xpp.get_attribute_value("hi") == "ho ho ho");

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::event::END_TAG);
	REQUIRE(xpp.get_text() == "xxx");

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::event::END_TAG);
	REQUIRE(xpp.get_text() == "test");

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::event::END_DOCUMENT);

	e = xpp.next();
	REQUIRE(e == tagsouppullparser::event::END_DOCUMENT);
}

TEST_CASE("<br>, <br/> and <br /> behave the same way", "[tagsouppullparser]")
{
	std::istringstream input_stream;
	tagsouppullparser parser;
	tagsouppullparser::event event;

	for (auto input : {"<br>", "<br/>", "<br />"}) {
		SECTION(input)
		{
			input_stream.str(input);
			parser.set_input(input_stream);

			event = parser.get_event_type();
			REQUIRE(event ==
				tagsouppullparser::event::START_DOCUMENT);

			event = parser.next();
			REQUIRE(event == tagsouppullparser::event::START_TAG);
			REQUIRE(parser.get_text() == "br");

			event = parser.next();
			REQUIRE(event ==
				tagsouppullparser::event::END_DOCUMENT);
		}
	}
}

#include "catch.hpp"

#include <regexmanager.h>
#include <exceptions.h>

using namespace newsbeuter;

TEST_CASE("RegexManager behaves correctly") {
	regexmanager rxman;
	std::vector<std::string> params;
	std::string input;

	SECTION("Throws on incorrect `highlight' definition") {
		SECTION("on `highlight' without parameters") {
			REQUIRE_THROWS_AS(rxman.handle_action("highlight", params), confighandlerexception);
		}

		SECTION("on invalid location") {
			params = {"invalidloc", "foo", "blue", "red"};
			REQUIRE_THROWS_AS(rxman.handle_action("highlight", params), confighandlerexception);
		}

		SECTION("on invalid regex") {
			params = {"feedlist", "*", "blue", "red"};
			REQUIRE_THROWS_AS(rxman.handle_action("highlight", params), confighandlerexception);
		}

		SECTION("on invalid command") {
			REQUIRE_THROWS_AS(rxman.handle_action("an-invalid-command", params), confighandlerexception);
		}
	}

	SECTION("Doesn't throw on correct `highlight' definition") {
		params = {"articlelist", "foo", "blue", "red"};
		REQUIRE_NOTHROW(rxman.handle_action("highlight", params));

		params = {"feedlist", "foo", "blue", "red"};
		REQUIRE_NOTHROW(rxman.handle_action("highlight", params));

		params = {"feedlist", "fbo", "blue", "red", "bold", "underline"};
		REQUIRE_NOTHROW(rxman.handle_action("highlight", params));

		params = {"all", "fba", "blue", "red", "bold", "underline"};
		REQUIRE_NOTHROW(rxman.handle_action("highlight", params));

		SECTION("Highlights according to definition") {
			input = "xfoox";
			rxman.quote_and_highlight(input, "articlelist");
			REQUIRE(input == "x<0>foo</>x");

			input = "yfooy";
			rxman.quote_and_highlight(input, "feedlist");
			REQUIRE(input == "y<0>foo</>y");
		}

		SECTION("Preserves text when there's nothing to highlight") {
			input = "xbarx";
			rxman.quote_and_highlight(input, "feedlist");
			REQUIRE(input == "xbarx");

			input = "<";
			rxman.quote_and_highlight(input, "feedlist");
			REQUIRE(input == "<");

			input = "a<b>";
			rxman.quote_and_highlight(input, "feedlist");
			REQUIRE(input == "a<b>");
		}
	}
}

TEST_CASE("regexmanager: `highlight all` adds rules for all locations") {
	regexmanager rxman;
	std::vector<std::string> params = {"all", "foo", "red"};
	REQUIRE_NOTHROW(rxman.handle_action("highlight", params));
	std::string input = "xxfooyy";

	for (auto location : {"article", "articlelist", "feedlist"}) {
		SECTION(location) {
		rxman.quote_and_highlight(input, location);
		REQUIRE(input == "xx<0>foo</>yy");
		}
	}
}

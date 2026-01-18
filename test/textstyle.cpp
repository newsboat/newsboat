#include "textstyle.h"

#include "3rd-party/catch.hpp"

#include "confighandlerexception.h"

using namespace newsboat;

TEST_CASE("TextStyle constructor rejects invalid values", "[TextStyle]")
{
	SECTION("invalid foreground color") {
		CHECK_THROWS_AS(TextStyle("", "default", {}), ConfigHandlerException);
		CHECK_THROWS_AS(TextStyle("nocolor", "default", {}), ConfigHandlerException);
	}

	SECTION("invalid background color") {
		CHECK_THROWS_AS(TextStyle("default", "", {}), ConfigHandlerException);
		CHECK_THROWS_AS(TextStyle("default", "nocolor", {}), ConfigHandlerException);
	}

	SECTION("invalid attribute") {
		CHECK_THROWS_AS(TextStyle("default", "default", {""}), ConfigHandlerException);
		CHECK_THROWS_AS(TextStyle("default", "default", {"noattribute"}), ConfigHandlerException);
	}
}

TEST_CASE("get_stfl_style_string omits default parts", "[TextStyle]")
{
	SECTION("only colors") {
		REQUIRE(TextStyle("red", "black", {}).get_stfl_style_string() == "fg=red,bg=black");
	}

	SECTION("default attribute") {
		REQUIRE(TextStyle("red", "black", {"default"}).get_stfl_style_string() ==
			"fg=red,bg=black");
	}

	SECTION("all default") {
		REQUIRE(TextStyle("default", "default", {}).get_stfl_style_string() == "");
		REQUIRE(TextStyle("default", "default", {"default"}).get_stfl_style_string() == "");
	}

	SECTION("default foreground") {
		REQUIRE(TextStyle("default", "black", {}).get_stfl_style_string() == "bg=black");
		REQUIRE(TextStyle("default", "black", {"bold"}).get_stfl_style_string() ==
			"bg=black,attr=bold");
	}

	SECTION("default background") {
		REQUIRE(TextStyle("red", "default", {}).get_stfl_style_string() == "fg=red");
		REQUIRE(TextStyle("red", "default", {"bold"}).get_stfl_style_string() ==
			"fg=red,attr=bold");
	}
}

TEST_CASE("get_stfl_style_string supports multiple attributes", "[TextStyle]")
{
	REQUIRE(TextStyle("default", "default", {"bold"}).get_stfl_style_string() == "attr=bold");
	REQUIRE(TextStyle("default", "default", {"bold", "blink"}).get_stfl_style_string() ==
		"attr=bold,attr=blink");
	REQUIRE(TextStyle("default", "default", {"bold", "blink", "reverse"}).get_stfl_style_string()
		== "attr=bold,attr=blink,attr=reverse");
}

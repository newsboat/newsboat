#include "3rd-party/catch.hpp"

#include "colormanager.h"
#include "exceptions.h"

using namespace newsboat;

TEST_CASE("colors_loaded() signals if any \"color\" actions have been processed",
		"[colormanager]")
{
	colormanager c;

	{
		INFO("By default, no colors are loaded");

		REQUIRE_FALSE(c.colors_loaded());
	}

	{
		INFO("Processing \"color\" action makes it return `true`");

		c.handle_action("color", { "listnormal", "default", "default" });
		REQUIRE(c.colors_loaded());
	}

	{
		INFO("Processing more \"color\" actions doesn't affect the return value");

		c.handle_action("color", { "listfocus", "default", "default" });
		REQUIRE(c.colors_loaded());
		c.handle_action("color", { "listfocus_unread", "default", "cyan" });
		REQUIRE(c.colors_loaded());
	}
}

TEST_CASE("get_fgcolors() returns foreground colors for each element that "
		"was processed", "[colormanager]")
{
	using results = std::map<std::string, std::string>;

	colormanager c;

	{
		INFO("By default, the list is empty");
		REQUIRE(c.get_fgcolors().empty());
	}

	{
		INFO("Each processed action adds corresponding entry to return value");

		c.handle_action("color", { "listnormal", "default", "default" });
		results expected { { "listnormal", "default" } };
		REQUIRE(c.get_fgcolors() == expected);

		c.handle_action("color", { "listfocus", "cyan", "default" });
		expected.emplace("listfocus", "cyan");
		REQUIRE(c.get_fgcolors() == expected);

		c.handle_action("color", { "background", "red", "default" });
		expected.emplace("background", "red");
		REQUIRE(c.get_fgcolors() == expected);
	}
}

TEST_CASE("get_bgcolors() returns foreground colors for each element that "
		"was processed", "[colormanager]")
{
	using results = std::map<std::string, std::string>;

	colormanager c;

	{
		INFO("By default, the list is empty");
		REQUIRE(c.get_bgcolors().empty());
	}

	{
		INFO("Each processed action adds corresponding entry to return value");

		c.handle_action("color", { "listnormal", "default", "default" });
		results expected { { "listnormal", "default" } };
		REQUIRE(c.get_bgcolors() == expected);

		c.handle_action("color", { "listfocus", "cyan", "yellow" });
		expected.emplace("listfocus", "yellow");
		REQUIRE(c.get_bgcolors() == expected);

		c.handle_action("color", { "background", "red", "green" });
		expected.emplace("background", "green");
		REQUIRE(c.get_bgcolors() == expected);
	}
}

TEST_CASE("register_commands() registers colormanager with configparser",
		"[colormanager]")
{
	configparser cfg;
	colormanager clr;

	REQUIRE_NOTHROW(clr.register_commands(cfg));

	REQUIRE_FALSE(clr.colors_loaded());
	cfg.parse("data/config-with-colors");
	REQUIRE(clr.colors_loaded());
}

TEST_CASE("handle_action() throws confighandlerexception if there aren't "
		"enough parameters", "[colormanager]")
{
	colormanager c;

	CHECK_THROWS_AS(c.handle_action("color", {}), confighandlerexception);
	CHECK_THROWS_AS(
			c.handle_action("color", { "one" }),
			confighandlerexception);
	CHECK_THROWS_AS(
			c.handle_action("color", { "one", "two" }),
			confighandlerexception);
}

#include "colormanager.h"

#include <string>
#include <unordered_set>
#include <vector>

#include "3rd-party/catch.hpp"
#include "exceptions.h"

using namespace newsboat;

TEST_CASE(
	"colors_loaded() signals if any \"color\" actions have been processed",
	"[colormanager]")
{
	colormanager c;

	{
		INFO("By default, no colors are loaded");

		REQUIRE_FALSE(c.colors_loaded());
	}

	{
		INFO("Processing \"color\" action makes it return `true`");

		c.handle_action("color", {"listnormal", "default", "default"});
		REQUIRE(c.colors_loaded());
	}

	{
		INFO("Processing more \"color\" actions doesn't affect the "
		     "return value");

		c.handle_action("color", {"listfocus", "default", "default"});
		REQUIRE(c.colors_loaded());
		c.handle_action(
			"color", {"listfocus_unread", "default", "cyan"});
		REQUIRE(c.colors_loaded());
	}
}

TEST_CASE(
	"get_fgcolors() returns foreground colors for each element that "
	"was processed",
	"[colormanager]")
{
	using results = std::map<std::string, std::string>;

	colormanager c;

	{
		INFO("By default, the list is empty");
		REQUIRE(c.get_fgcolors().empty());
	}

	{
		INFO("Each processed action adds corresponding entry to return "
		     "value");

		c.handle_action("color", {"listnormal", "default", "default"});
		results expected{{"listnormal", "default"}};
		REQUIRE(c.get_fgcolors() == expected);

		c.handle_action("color", {"listfocus", "cyan", "default"});
		expected.emplace("listfocus", "cyan");
		REQUIRE(c.get_fgcolors() == expected);

		c.handle_action("color", {"background", "red", "default"});
		expected.emplace("background", "red");
		REQUIRE(c.get_fgcolors() == expected);
	}
}

TEST_CASE(
	"get_bgcolors() returns foreground colors for each element that "
	"was processed",
	"[colormanager]")
{
	using results = std::map<std::string, std::string>;

	colormanager c;

	{
		INFO("By default, the list is empty");
		REQUIRE(c.get_bgcolors().empty());
	}

	{
		INFO("Each processed action adds corresponding entry to return "
		     "value");

		c.handle_action("color", {"listnormal", "default", "default"});
		results expected{{"listnormal", "default"}};
		REQUIRE(c.get_bgcolors() == expected);

		c.handle_action("color", {"listfocus", "cyan", "yellow"});
		expected.emplace("listfocus", "yellow");
		REQUIRE(c.get_bgcolors() == expected);

		c.handle_action("color", {"background", "red", "green"});
		expected.emplace("background", "green");
		REQUIRE(c.get_bgcolors() == expected);
	}
}

TEST_CASE(
	"register_commands() registers colormanager with configparser",
	"[colormanager]")
{
	configparser cfg;
	colormanager clr;

	REQUIRE_NOTHROW(clr.register_commands(cfg));

	REQUIRE_FALSE(clr.colors_loaded());
	cfg.parse("data/config-with-colors");
	REQUIRE(clr.colors_loaded());
}

TEST_CASE(
	"handle_action() throws confighandlerexception if there aren't "
	"enough parameters",
	"[colormanager]")
{
	colormanager c;

	CHECK_THROWS_AS(c.handle_action("color", {}), confighandlerexception);
	CHECK_THROWS_AS(
		c.handle_action("color", {"one"}), confighandlerexception);
	CHECK_THROWS_AS(
		c.handle_action("color", {"one", "two"}),
		confighandlerexception);
}

TEST_CASE(
	"handle_action() throws confighandlerexception if foreground color "
	"is invalid",
	"[colormanager]")
{
	colormanager c;

	const std::vector<std::string> non_colors{
		{"awesome", "but", "nonexistent", "colors"}};
	for (const auto& color : non_colors) {
		CHECK_THROWS_AS(
			c.handle_action(
				"color", {"listfocus", color, "default"}),
			confighandlerexception);
	}
}

TEST_CASE(
	"handle_action() throws confighandlerexception if background color "
	"is invalid",
	"[colormanager]")
{
	colormanager c;

	const std::vector<std::string> non_colors{
		{"awesome", "but", "nonexistent", "colors"}};
	for (const auto& color : non_colors) {
		CHECK_THROWS_AS(
			c.handle_action(
				"color", {"listfocus", "default", color}),
			confighandlerexception);
	}
}

TEST_CASE(
	"handle_action() throws confighandlerexception if color attribute "
	"is invalid",
	"[colormanager]")
{
	colormanager c;

	const std::vector<std::string> non_attributes{
		{"awesome", "but", "nonexistent", "attributes"}};
	for (const auto& attr : non_attributes) {
		CHECK_THROWS_AS(
			c.handle_action(
				"color", {"listfocus", "red", "red", attr}),
			confighandlerexception);
	}
}

TEST_CASE(
	"handle_action() throws confighandlerexception if color is applied "
	"to non-existent element",
	"[colormanager]")
{
	colormanager c;

	const std::vector<std::string> non_elements{
		{"awesome", "but", "nonexistent", "elements"}};
	for (const auto& element : non_elements) {
		CHECK_THROWS_AS(
			c.handle_action("color", {element, "red", "green"}),
			confighandlerexception);
	}
}

TEST_CASE(
	"handle_action() throws confighandlerexception if it's passed a "
	"command other than \"color\"",
	"[colormanager]")
{
	colormanager c;

	const std::vector<std::string> other_commands{
		{"browser", "include", "auto-reload", "ocnews-flag-star"}};
	for (const auto& command : other_commands) {
		CHECK_THROWS_AS(
			c.handle_action(command, {}), confighandlerexception);
	}
}

TEST_CASE(
	"dump_config() returns everything we put into colormanager",
	"[colormanager]")
{
	colormanager c;

	std::unordered_set<std::string> expected;
	std::vector<std::string> config;

	// Checks that `expected` contains the same lines as `config` contains,
	// and nothing more.
	auto equivalent = [&]() -> bool {
		std::size_t found = 0;
		for (const auto& line : config) {
			if (expected.find(line) == expected.end())
				return false;

			found++;
		}

		return found == expected.size();
	};

	{
		INFO("Empty colormanager outputs nothing");
		c.dump_config(config);
		REQUIRE(config.empty());
		REQUIRE(equivalent());
	}

	expected.emplace("color listfocus default red");
	c.handle_action("color", {"listfocus", "default", "red"});
	config.clear();
	c.dump_config(config);
	REQUIRE(config.size() == 1);
	REQUIRE(equivalent());

	expected.emplace("color background green cyan bold");
	c.handle_action("color", {"background", "green", "cyan", "bold"});
	config.clear();
	c.dump_config(config);
	REQUIRE(config.size() == 2);
	REQUIRE(equivalent());

	expected.emplace("color listnormal black yellow underline standout");
	c.handle_action(
		"color",
		{"listnormal", "black", "yellow", "underline", "standout"});
	config.clear();
	c.dump_config(config);
	REQUIRE(config.size() == 3);
	REQUIRE(equivalent());
}

#include "colormanager.h"

#include <string>
#include <unordered_set>
#include <vector>

#include "3rd-party/catch.hpp"
#include "confighandlerexception.h"

using namespace newsboat;

TEST_CASE(
	"get_styles() returns foreground/background colors and attributes for each "
	"element that was processed",
	"[ColorManager]")
{
	ColorManager c;

	{
		INFO("By default, the list is empty");
		REQUIRE(c.get_styles().size() == 0);
	}

	{
		INFO("Each processed action adds corresponding entry to return "
			"value");

		c.handle_action("color", {"listnormal", "default", "default"});
		REQUIRE(c.get_styles().size() == 1);
		REQUIRE(c.get_styles().count("listnormal") == 1);

		c.handle_action("color", {"listfocus", "cyan", "default", "bold", "underline"});
		REQUIRE(c.get_styles().size() == 2);
		REQUIRE(c.get_styles().count("listfocus") == 1);

		c.handle_action("color", {"background", "red", "yellow"});
		REQUIRE(c.get_styles().size() == 3);
		REQUIRE(c.get_styles().count("background") == 1);

		REQUIRE(c.get_styles()["listnormal"].fg_color == "default");
		REQUIRE(c.get_styles()["listnormal"].bg_color == "default");
		REQUIRE(c.get_styles()["listnormal"].attributes.size() == 0);

		REQUIRE(c.get_styles()["listfocus"].fg_color == "cyan");
		REQUIRE(c.get_styles()["listfocus"].bg_color == "default");
		REQUIRE(c.get_styles()["listfocus"].attributes.size() == 2);
		REQUIRE(c.get_styles()["listfocus"].attributes[0] == "bold");
		REQUIRE(c.get_styles()["listfocus"].attributes[1] == "underline");

		REQUIRE(c.get_styles()["background"].fg_color == "red");
		REQUIRE(c.get_styles()["background"].bg_color == "yellow");
		REQUIRE(c.get_styles()["background"].attributes.size() == 0);
	}
}

TEST_CASE("register_commands() registers ColorManager with ConfigParser",
	"[ColorManager]")
{
	ConfigParser cfg;
	ColorManager clr;

	REQUIRE_NOTHROW(clr.register_commands(cfg));

	REQUIRE(clr.get_styles().size() == 0);

	cfg.parse_file("data/config-with-colors");

	REQUIRE(clr.get_styles().size() == 2);
}

TEST_CASE(
	"handle_action() throws ConfigHandlerException if there aren't "
	"enough parameters",
	"[ColorManager]")
{
	ColorManager c;

	CHECK_THROWS_AS(c.handle_action("color", {}), ConfigHandlerException);
	CHECK_THROWS_AS(
		c.handle_action("color", {"one"}), ConfigHandlerException);
	CHECK_THROWS_AS(c.handle_action("color", {"one", "two"}),
		ConfigHandlerException);
}

TEST_CASE(
	"handle_action() throws ConfigHandlerException if foreground color "
	"is invalid",
	"[ColorManager]")
{
	ColorManager c;

	const std::vector<std::string> non_colors{
		{"awesome", "but", "nonexistent", "colors"}};
	for (const auto& color : non_colors) {
		CHECK_THROWS_AS(c.handle_action("color",
		{"listfocus", color, "default"}),
		ConfigHandlerException);
	}
}

TEST_CASE(
	"handle_action() throws ConfigHandlerException if background color "
	"is invalid",
	"[ColorManager]")
{
	ColorManager c;

	const std::vector<std::string> non_colors{
		{"awesome", "but", "nonexistent", "colors"}};
	for (const auto& color : non_colors) {
		CHECK_THROWS_AS(c.handle_action("color",
		{"listfocus", "default", color}),
		ConfigHandlerException);
	}
}

TEST_CASE(
	"handle_action() throws ConfigHandlerException if color attribute "
	"is invalid",
	"[ColorManager]")
{
	ColorManager c;

	const std::vector<std::string> non_attributes{
		{"awesome", "but", "nonexistent", "attributes"}};
	for (const auto& attr : non_attributes) {
		CHECK_THROWS_AS(c.handle_action("color",
		{"listfocus", "red", "red", attr}),
		ConfigHandlerException);
	}
}

TEST_CASE(
	"handle_action() throws ConfigHandlerException if color is applied "
	"to non-existent element",
	"[ColorManager]")
{
	ColorManager c;

	const std::vector<std::string> non_elements{
		{"awesome", "but", "nonexistent", "elements"}};
	for (const auto& element : non_elements) {
		CHECK_THROWS_AS(
			c.handle_action("color", {element, "red", "green"}),
			ConfigHandlerException);
	}
}

TEST_CASE(
	"handle_action() throws ConfigHandlerException if it's passed a "
	"command other than \"color\"",
	"[ColorManager]")
{
	ColorManager c;

	const std::vector<std::string> other_commands{
		{"browser", "include", "auto-reload", "ocnews-flag-star"}};
	for (const auto& command : other_commands) {
		CHECK_THROWS_AS(
			c.handle_action(command, {}), ConfigHandlerException);
	}
}

TEST_CASE("dump_config() returns everything we put into ColorManager",
	"[ColorManager]")
{
	ColorManager c;

	std::unordered_set<std::string> expected;
	std::vector<std::string> config;

	// Checks that `expected` contains the same lines as `config` contains,
	// and nothing more.
	auto equivalent = [&]() -> bool {
		std::size_t found = 0;
		for (const auto& line : config)
		{
			if (expected.find(line) == expected.end()) {
				return false;
			}

			found++;
		}

		return found == expected.size();
	};

	{
		INFO("Empty ColorManager outputs nothing");
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
	c.handle_action("color",
	{"listnormal", "black", "yellow", "underline", "standout"});
	config.clear();
	c.dump_config(config);
	REQUIRE(config.size() == 3);
	REQUIRE(equivalent());
}

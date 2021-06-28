#include "colormanager.h"

#include <string>
#include <unordered_set>
#include <vector>

#include "3rd-party/catch.hpp"

#include "confighandlerexception.h"
#include "configparser.h"

using namespace newsboat;

TEST_CASE(
	"apply_colors() invokes the callback for each element, supplying the element name and its style",
	"[ColorManager]")
{
	ColorManager c;

	SECTION("By default, the list is empty") {
		unsigned int counter = 0;
		c.apply_colors([&counter](const std::string&, const std::string&) {
			counter++;
		});
		REQUIRE(counter == 0);
	}

	SECTION("Each processed action adds corresponding entry to return value") {
		c.handle_action("color", {"listnormal", "default", "default"});
		c.handle_action("color", {"listfocus_unread", "cyan", "default", "bold", "underline"});
		c.handle_action("color", {"background", "red", "yellow"});
		c.handle_action("color", {"info", "green", "white", "reverse"});
		c.handle_action("color", {"end-of-text-marker", "color123", "default", "dim", "protect"});

		std::map<std::string, std::string> styles;
		c.apply_colors([&styles](const std::string& element, const std::string& style) {
			REQUIRE(styles.find(element) == styles.end());
			styles[element] = style;
		});

		REQUIRE(styles.size() == 5);
		REQUIRE(styles["listnormal"] == "");
		REQUIRE(styles["listfocus_unread"] == "fg=cyan,attr=bold,attr=underline");
		REQUIRE(styles["background"] == "fg=red,bg=yellow");
		REQUIRE(styles["info"] == "fg=green,bg=white,attr=reverse");
		REQUIRE(styles["end-of-text-marker"] == "fg=color123,attr=dim,attr=protect");
	}

	SECTION("For `article` element, two additional elements are emitted") {
		c.handle_action("color", {"article", "white", "blue", "reverse"});

		std::map<std::string, std::string> styles;
		c.apply_colors([&styles](const std::string& element, const std::string& style) {
			REQUIRE(styles.find(element) == styles.end());
			styles[element] = style;
		});

		REQUIRE(styles.size() == 3);
		REQUIRE(styles["article"] == "fg=white,bg=blue,attr=reverse");
		REQUIRE(styles["color_bold"] == "fg=white,bg=blue,attr=reverse,attr=bold");
		REQUIRE(styles["color_underline"] == "fg=white,bg=blue,attr=reverse,attr=underline");
	}
}

TEST_CASE("register_commands() registers ColorManager with ConfigParser",
	"[ColorManager]")
{
	ConfigParser cfg;
	ColorManager clr;

	unsigned int counter = 0;
	const auto tester = [&counter](const std::string&, const std::string&) {
		counter++;
	};

	REQUIRE_NOTHROW(clr.register_commands(cfg));

	clr.apply_colors(tester);
	REQUIRE(counter == 0);

	cfg.parse_file("data/config-with-colors");

	clr.apply_colors(tester);
	REQUIRE(counter == 2);
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

	expected.emplace("color article green cyan bold");
	c.handle_action("color", {"article", "green", "cyan", "bold"});
	config.clear();
	c.dump_config(config);
	REQUIRE(config.size() == 2);
	REQUIRE(equivalent());

	expected.emplace("color listnormal_unread black yellow underline standout");
	c.handle_action("color",
	{"listnormal_unread", "black", "yellow", "underline", "standout"});
	config.clear();
	c.dump_config(config);
	REQUIRE(config.size() == 3);
	REQUIRE(equivalent());
}

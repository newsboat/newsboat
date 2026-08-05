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

	SECTION("Each processed action adds corresponding entry to return value") {
		c.handle_action("color", {"listnormal", "default", "default"});
		c.handle_action("color", {"listfocus_unread", "cyan", "default", "bold", "underline"});
		c.handle_action("color", {"background", "red", "yellow"});
		c.handle_action("color", {"info", "green", "white", "reverse"});
		c.handle_action("color", {"end-of-text-marker", "color123", "default", "dim", "protect"});
		c.handle_action("color", {"hint-key", "default", "color2"});
		c.handle_action("color", {"hint-description", "color3", "default"});

		const auto styles = c.get_stfl_styles();

		REQUIRE(styles.at("listnormal") == "");
		REQUIRE(styles.at("listfocus_unread") == "fg=cyan,attr=bold,attr=underline");
		REQUIRE(styles.at("background") == "fg=red,bg=yellow");
		REQUIRE(styles.at("info") == "fg=green,bg=white,attr=reverse");
		REQUIRE(styles.at("title") == "fg=green,bg=white,attr=reverse");
		REQUIRE(styles.at("end-of-text-marker") == "fg=color123,attr=dim,attr=protect");
		REQUIRE(styles.at("hint-key") == "bg=color2");
		REQUIRE(styles.at("hint-description") == "fg=color3");

		// These two weren't set explicitly and fell back to `info`
		REQUIRE(styles.at("hint-keys-delimiter") == "fg=green,bg=white,attr=reverse");
		REQUIRE(styles.at("hint-separator") == "fg=green,bg=white,attr=reverse");
	}

	SECTION("For `article` element, two additional elements are emitted") {
		c.handle_action("color", {"article", "white", "blue", "reverse"});

		const auto styles = c.get_stfl_styles();

		REQUIRE(styles.at("article") == "fg=white,bg=blue,attr=reverse");
		REQUIRE(styles.at("color_bold") == "fg=white,bg=blue,attr=reverse,attr=bold");
		REQUIRE(styles.at("color_underline") ==
			"fg=white,bg=blue,attr=reverse,attr=underline");
	}
}

TEST_CASE("register_commands() registers ColorManager with ConfigParser",
	"[ColorManager]")
{
	ConfigParser cfg;
	ColorManager clr;

	REQUIRE_NOTHROW(clr.register_commands(cfg));

	cfg.parse_file("data/config-with-colors"_path);

	const auto styles = clr.get_stfl_styles();
	REQUIRE(styles.at("background") == "fg=red,bg=green");
	REQUIRE(styles.at("listfocus") == "fg=blue,bg=black,attr=bold");
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

	expected.emplace("color hint-keys-delimiter color5 default dim");
	c.handle_action("color", {"hint-keys-delimiter", "color5", "default", "dim"});
	config.clear();
	c.dump_config(config);
	REQUIRE(config.size() == 4);
	REQUIRE(equivalent());

	expected.emplace("color hint-separator color7 color8");
	c.handle_action("color", {"hint-separator", "color7", "color8"});
	config.clear();
	c.dump_config(config);
	REQUIRE(config.size() == 5);
	REQUIRE(equivalent());

}

TEST_CASE("If no colors were specified for the "
	"`title`/`hint-key`/`hint-keys-delimiter`/`hint-separator`/`hint-description` elements, "
	"then use colors from the `info` element (if any)",
	"[ColorManager]")
{
	const std::vector<std::string> elements {
		"title", "hint-key", "hint-keys-delimiter", "hint-separator", "hint-description"};

	for (const auto& element : elements) {
		DYNAMIC_SECTION("element: " << element) {
			ColorManager c;

			SECTION("Element's style can be changed as usual") {
				c.handle_action("color", {element, "green", "default", "underline"});

				const auto styles = c.get_stfl_styles();

				REQUIRE(styles.size() >= 1);
				REQUIRE(styles.at(element) == "fg=green,attr=underline");
			}

			SECTION("Element and `info` don't interfere with each other") {
				SECTION("Element's style is set before `info`") {
					c.handle_action("color", {element, "blue", "black"});
					c.handle_action("color", {"info", "green", "yellow", "bold"});
				}

				SECTION("Element's style is set after `info`") {
					c.handle_action("color", {"info", "green", "yellow", "bold"});
					c.handle_action("color", {element, "blue", "black"});
				}

				const auto styles = c.get_stfl_styles();

				REQUIRE(styles.size() >= 2);
				REQUIRE(styles.at(element) == "fg=blue,bg=black");
				REQUIRE(styles.at("info") == "fg=green,bg=yellow,attr=bold");
			}

			SECTION("Element inherits the `info` style when available") {
				c.handle_action("color", {"info", "red", "magenta", "reverse"});

				const auto styles = c.get_stfl_styles();

				REQUIRE(styles.size() >= 2);
				REQUIRE(styles.at(element) == "fg=red,bg=magenta,attr=reverse");
				REQUIRE(styles.at("info") == "fg=red,bg=magenta,attr=reverse");
			}
		}
	}

	SECTION("Element has its default style if there is no style for `info` either") {
		ColorManager c;
		c.handle_action("color", {"listnormal", "black", "white"});

		const auto styles = c.get_stfl_styles();

		REQUIRE(styles.size() >= 1);
		REQUIRE(styles.at("listnormal") == "fg=black,bg=white");

		REQUIRE(styles.at("info") == "fg=yellow,bg=blue,attr=bold");
		REQUIRE(styles.at("title") == "fg=yellow,bg=blue,attr=bold");
		REQUIRE(styles.at("hint-key") == "fg=yellow,bg=blue,attr=bold");
		REQUIRE(styles.at("hint-keys-delimiter") == "fg=white,bg=blue");
		REQUIRE(styles.at("hint-separator") == "fg=white,bg=blue,attr=bold");
		REQUIRE(styles.at("hint-description") == "fg=white,bg=blue");
	}
}

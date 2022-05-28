#include <textviewwidget.h>

#include "3rd-party/catch.hpp"
#include "configpaths.h"
#include "controller.h"
#include "view.h"
#include "listformatter.h"
#include "stflpp.h"

using namespace newsboat;

const static std::string stflTextviewForm =
	"vbox\n"
	"  textview[textview-name]\n"
	"    .expand:0\n"
	"    offset[textview-name_offset]:0\n"
	"    richtext:1\n"
	"    height:5\n";

const static std::string widgetName = "textview-name";

TEST_CASE("stfl_replace_lines() keeps text in view", "[TextviewWidget]")
{
	Stfl::Form form(stflTextviewForm);
	TextviewWidget widget(widgetName, form);

	REQUIRE(widget.get_scroll_offset() == 0);

	ListFormatter listfmt;
	listfmt.add_line("one");
	listfmt.add_line("two");
	listfmt.add_line("three");
	widget.stfl_replace_lines(listfmt.get_lines_count(), listfmt.format_list());

	widget.set_scroll_offset(2);
	REQUIRE(widget.get_scroll_offset() == 2);

	SECTION("emptying textview results in scroll to top") {
		listfmt.clear();
		widget.stfl_replace_lines(listfmt.get_lines_count(), listfmt.format_list());

		REQUIRE(widget.get_scroll_offset() == 0);
	}

	SECTION("widget scrolls upwards if lines are removed") {
		listfmt.clear();
		listfmt.add_line("one");
		listfmt.add_line("two");
		widget.stfl_replace_lines(listfmt.get_lines_count(), listfmt.format_list());

		REQUIRE(widget.get_scroll_offset() == 1);
	}

	SECTION("no change in scroll location when adding lines") {
		listfmt.add_line("four");
		listfmt.add_line("five");
		widget.stfl_replace_lines(listfmt.get_lines_count(), listfmt.format_list());

		REQUIRE(widget.get_scroll_offset() == 2);
	}
}

TEST_CASE("Basic movements work as expected", "[TextviewWidget]")
{
	ConfigPaths paths;
	Controller c(paths);
	newsboat::View v(&c);

	Stfl::Form form(stflTextviewForm);
	TextviewWidget widget(widgetName, form);

	// The following sections have some interactions with STFL.
	// We call `Stfl::reset()` to make sure the terminal is in a regular mode
	// before calling Catch2 functions. Without the reset, Catch2 might output
	// text while the terminal is in application mode, which makes it invisble
	// when back in regular mode.
	// The `View` object calls `Stfl::reset()` in its destructor so we can be
	// sure we always return to the regular terminal mode, even when an
	// exception is thrown.

	form.run(-3); // Recalculate widget dimensions (updates effective width/height fields of textview)
	Stfl::reset();

	REQUIRE(widget.get_height() == 5);

	REQUIRE(widget.get_scroll_offset() == 0);

	ListFormatter listfmt;
	listfmt.add_line("one");
	listfmt.add_line("two");
	listfmt.add_line("three");
	listfmt.add_line("four");
	listfmt.add_line("five");
	listfmt.add_line("six");
	listfmt.add_line("seven");
	listfmt.add_line("eight");
	listfmt.add_line("nine");
	listfmt.add_line("ten");
	widget.stfl_replace_lines(listfmt.get_lines_count(), listfmt.format_list());

	widget.set_scroll_offset(2);
	REQUIRE(widget.get_scroll_offset() == 2);

	SECTION("scroll_up() scrolls up a single line") {
		widget.scroll_up();
		REQUIRE(widget.get_scroll_offset() == 1);

		widget.scroll_up();
		REQUIRE(widget.get_scroll_offset() == 0);
	}

	SECTION("scroll_down() scrolls down a single line") {
		widget.scroll_down();
		REQUIRE(widget.get_scroll_offset() == 3);

		widget.scroll_down();
		REQUIRE(widget.get_scroll_offset() == 4);
	}

	SECTION("scroll_to_top() moves scroll location to first line") {
		widget.scroll_to_top();
		REQUIRE(widget.get_scroll_offset() == 0);
	}

	SECTION("scroll_to_bottom() moves scroll location to last line") {
		widget.scroll_to_bottom();
		// Scroll offset is set in such a way that one line past the end is visible at the bottom. Required scroll offset depends on the height of the textview (hardcoded: 5 lines)
		// Given 10 items in the list, scroll offset 6 shows line seven as first line, followed by eight, nine, ten, and an empty line.
		REQUIRE(widget.get_scroll_offset() == 6);
	}

	SECTION("scroll_page_up() moves up just enough such that there is a single line of overlap between old and new view") {
		widget.set_scroll_offset(6);
		REQUIRE(widget.get_scroll_offset() == 6);

		widget.scroll_page_up();
		REQUIRE(widget.get_scroll_offset() == 2);

		widget.scroll_page_up();
		REQUIRE(widget.get_scroll_offset() == 0);
	}

	SECTION("scroll_page_down() moves down just enough such that there is a single line of overlap between old and new view") {
		widget.set_scroll_offset(3);
		REQUIRE(widget.get_scroll_offset() == 3);

		widget.scroll_page_down();
		REQUIRE(widget.get_scroll_offset() == 7);

		widget.scroll_page_down();
		REQUIRE(widget.get_scroll_offset() == 9);
	}
}

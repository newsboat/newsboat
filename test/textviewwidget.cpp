#include <textviewwidget.h>

#include "3rd-party/catch.hpp"
#include "listformatter.h"
#include "stflpp.h"

using namespace newsboat;

const static std::string stflTextviewForm =
	"vbox\n"
	"  textview[textview-name]\n"
	"    .expand:vh\n"
	"    offset[textview-name_offset]:0\n"
	"    richtext:1\n";

TEST_CASE("stfl_replace_lines() keeps text in view", "[TextviewWidget]")
{
	Stfl::Form form(stflTextviewForm);
	TextviewWidget widget("textview-name", form);

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

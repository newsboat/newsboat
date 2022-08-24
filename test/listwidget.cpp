#include "listwidget.h"

#include "stflpp.h"
#include "utils.h"

#include "3rd-party/catch.hpp"

using namespace newsboat;

const static std::string stflListForm =
	"vbox\n"
	"  list[list-name]\n"
	"    richtext:1\n"
	"    pos[list-name_pos]:0\n"
	"    offset[list-name_offset]:0";


TEST_CASE("stfl_replace_lines() makes sure `position < num_lines`", "[ListWidget]")
{
	const std::uint32_t scrolloff = 0;
	Stfl::Form listForm(stflListForm);

	ListFormatter fmt;
	fmt.add_line("line 1");
	fmt.add_line("line 2");
	fmt.add_line("line 3");

	ListWidget listWidget("list-name", listForm);
	listWidget.set_num_context_lines(scrolloff);

	GIVEN("a ListWidget with 3 lines and position=2") {
		listWidget.stfl_replace_lines(fmt);
		listWidget.set_position(2);
		REQUIRE(listWidget.get_position() == 2);

		WHEN("the number of lines is reduced") {
			fmt.clear();
			fmt.add_line("line 1");
			fmt.add_line("line 2");
			listWidget.stfl_replace_lines(fmt);

			THEN("the position is equal to the bottom item of the new list") {
				REQUIRE(listWidget.get_position() == 1);
			}
		}

		WHEN("all lines are removed") {
			fmt.clear();
			listWidget.stfl_replace_lines(fmt);

			THEN("the position is changed to 0") {
				REQUIRE(listWidget.get_position() == 0);
			}
		}

		WHEN("a line is added") {
			fmt.add_line("line 4");
			listWidget.stfl_replace_lines(fmt);

			THEN("the position is not changed") {
				REQUIRE(listWidget.get_position() == 2);
			}
		}
	}
}

TEST_CASE("stfl_replace_list() makes sure `position < num_lines`", "[ListWidget]")
{
	const std::uint32_t scrolloff = 0;
	Stfl::Form listForm(stflListForm);

	ListFormatter fmt;
	fmt.add_line("line 1");
	fmt.add_line("line 2");
	fmt.add_line("line 3");

	ListWidget listWidget("list-name", listForm);
	listWidget.set_num_context_lines(scrolloff);

	const std::string stflListPrototype =
		"{list[list-name] "
		"richtext:1 "
		"pos[list-name_pos]:0 "
		"offset[list-name_offset]:0 "
		"%s}";

	GIVEN("a ListWidget with 3 lines and position=2") {
		listWidget.stfl_replace_lines(fmt);
		listWidget.set_position(2);
		REQUIRE(listWidget.get_position() == 2);

		WHEN("the number of lines is reduced") {
			const std::string stflList = strprintf::fmt(stflListPrototype,
					R"({listitem text:"line 1"}{listitem text:"line 2"})");
			listWidget.stfl_replace_list(2, stflList);

			THEN("the position is changed to 0") {
				REQUIRE(listWidget.get_position() == 1);
			}
		}

		WHEN("all lines are removed") {
			const std::string stflList = strprintf::fmt(stflListPrototype, "");
			listWidget.stfl_replace_list(0, stflList);

			THEN("the position is changed to 0") {
				REQUIRE(listWidget.get_position() == 0);
			}
		}

		WHEN("a line is added") {
			const std::string stflList = strprintf::fmt(stflListPrototype,
					R"({listitem text:"line 1"}{listitem text:"line 2"}{listitem text:"line 3"}{listitem text:"line 4"})");
			listWidget.stfl_replace_list(4, stflList);

			THEN("the position is changed to 0") {
				REQUIRE(listWidget.get_position() == 2);
			}
		}
	}
}

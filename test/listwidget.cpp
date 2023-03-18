#include "listwidget.h"

#include "stflpp.h"
#include "utils.h"

#include "3rd-party/catch.hpp"
#include <cstdint>
#include <string>

using namespace newsboat;

const static std::string stflListForm =
	"vbox\n"
	"  list[list-name]\n"
	"    richtext:1\n"
	"    pos[list-name_pos]:0\n"
	"    offset[list-name_offset]:0";

std::string render_empty_line(std::uint32_t, std::uint32_t)
{
	return "";
}


TEST_CASE("invalidate_list_content() makes sure `position < num_lines`", "[NewListWidget]")
{
	const std::uint32_t scrolloff = 0;
	Stfl::Form listForm(stflListForm);

	NewListWidget listWidget("list-name", listForm, scrolloff);

	GIVEN("a NewListWidget with 3 lines and position=2") {
		listWidget.invalidate_list_content(3, render_empty_line);
		listWidget.set_position(2);
		REQUIRE(listWidget.get_position() == 2);

		WHEN("the number of lines is reduced") {
			listWidget.invalidate_list_content(2, render_empty_line);

			THEN("the position is equal to the bottom item of the new list") {
				REQUIRE(listWidget.get_position() == 1);
			}
		}

		WHEN("all lines are removed") {
			listWidget.invalidate_list_content(0, render_empty_line);

			THEN("the position is changed to 0") {
				REQUIRE(listWidget.get_position() == 0);
			}
		}

		WHEN("a line is added") {
			listWidget.invalidate_list_content(4, render_empty_line);

			THEN("the position is not changed") {
				REQUIRE(listWidget.get_position() == 2);
			}
		}
	}
}

TEST_CASE("stfl_replace_list() makes sure the position is reset", "[NewListWidget]")
{
	const std::uint32_t scrolloff = 0;
	Stfl::Form listForm(stflListForm);

	NewListWidget listWidget("list-name", listForm, scrolloff);

	const std::string stflListPrototype =
		"{list[list-name] "
		"richtext:1 "
		"pos[list-name_pos]:0 "
		"offset[list-name_offset]:0 "
		"}";

	GIVEN("a NewListWidget with 3 lines and position=2") {
		listWidget.invalidate_list_content(3, render_empty_line);
		listWidget.set_position(2);
		REQUIRE(listWidget.get_position() == 2);

		WHEN("the list is replaced") {
			listWidget.stfl_replace_list(stflListPrototype);

			THEN("the position is changed to 0") {
				REQUIRE(listWidget.get_position() == 0);
			}
		}
	}
}

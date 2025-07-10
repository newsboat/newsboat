#include "listwidget.h"

#include "stflpp.h"
#include "stflrichtext.h"

#include "3rd-party/catch.hpp"
#include <cstdint>
#include <set>
#include <string>

using namespace Newsboat;

const static std::string stflListForm =
	"vbox\n"
	"  list[list-name]\n"
	"    richtext:1\n"
	"    pos[list-name_pos]:0\n"
	"    offset[list-name_offset]:0";

StflRichText render_empty_line(std::uint32_t, std::uint32_t)
{
	return StflRichText::from_plaintext("");
}


TEST_CASE("invalidate_list_content() makes sure `position < num_lines`", "[ListWidget]")
{
	const std::uint32_t scrolloff = 0;
	Stfl::Form listForm(stflListForm);

	ListWidget listWidget("list-name", listForm, scrolloff);

	GIVEN("a ListWidget with 3 lines and position=2") {
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

TEST_CASE("stfl_replace_list() makes sure the position is reset", "[ListWidget]")
{
	const std::uint32_t scrolloff = 0;
	Stfl::Form listForm(stflListForm);

	ListWidget listWidget("list-name", listForm, scrolloff);

	const std::string stflListPrototype =
		"{list[list-name] "
		"richtext:1 "
		"pos[list-name_pos]:0 "
		"offset[list-name_offset]:0 "
		"}";

	GIVEN("a ListWidget with 3 lines and position=2") {
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

TEST_CASE("invalidate_list_content() clears internal caches", "[ListWidget]")
{
	const std::uint32_t scrolloff = 0;
	Stfl::Form listForm(stflListForm);
	// Recalculate list dimensions
	listForm.run(-3);
	Stfl::reset();

	ListWidget listWidget("list-name", listForm, scrolloff);

	std::set<std::uint32_t> requested_lines;
	auto render_line = [&](std::uint32_t line, std::uint32_t) -> StflRichText {
		requested_lines.insert(line);
		return StflRichText::from_plaintext("");
	};

	GIVEN("a ListWidget with 3 lines") {
		listWidget.invalidate_list_content(3, render_empty_line);

		WHEN("the invalidate_list_content() is called") {
			requested_lines.clear();
			listWidget.invalidate_list_content(3, render_line);

			THEN("all lines are requested again") {
				REQUIRE(requested_lines.size() == 3);
				REQUIRE(requested_lines.count(0) == 1);
				REQUIRE(requested_lines.count(1) == 1);
				REQUIRE(requested_lines.count(2) == 1);
			}
		}
	}
}

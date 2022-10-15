#include "listmovementcontrol.h"

#include <cstdint>

#include "3rd-party/catch.hpp"

using namespace newsboat;

namespace {

class ListStub {
public:
	ListStub(const std::string&, Stfl::Form&)
		: height(80)
		, num_lines(0)
		, position(0)
		, scroll_offset(0)
	{
	}

	virtual ~ListStub() = default;

	virtual void on_list_changed() = 0;

	std::uint32_t get_height()
	{
		return height;
	}

	std::uint32_t get_num_lines()
	{
		return num_lines;
	}

	void update_position(std::uint32_t position, std::uint32_t scroll_offset)
	{
		this->position = position;
		this->scroll_offset = scroll_offset;
	}

public: // Stub
	std::uint32_t height;
	std::uint32_t num_lines;
	std::uint32_t position;
	std::uint32_t scroll_offset;
};

const static std::string stflListForm =
	"vbox\n"
	"  list[list-name]\n"
	"    richtext:1\n"
	"    pos[list-name_pos]:0\n"
	"    offset[list-name_offset]:0";

const std::string dummyFormName = "dummy";
Stfl::Form dummyForm(stflListForm);

}

TEST_CASE("get_position() returns position of selected item in list",
	"[ListMovementControl]")
{
	auto list_movement = ListMovementControl<ListStub>(dummyFormName, dummyForm, 0);
	ListStub& list = list_movement;

	GIVEN("a list of 10 items, where the 5th item is selected") {
		list.num_lines = 10;
		list_movement.set_position(4);

		REQUIRE(list_movement.get_position() == 4);
	}
}

TEST_CASE("on_lines_changed() makes sure `position < num_lines`", "[ListMovementControl]")
{
	auto list_movement = ListMovementControl<ListStub>(dummyFormName, dummyForm, 0);
	ListStub& list = list_movement;

	GIVEN("a list of 10 items, where the 5th item is selected") {
		list.num_lines = 10;
		list_movement.set_position(4);

		WHEN("the number of items is reduced to 3") {
			list.num_lines = 3;
			list.on_list_changed();

			THEN("the last item of the list is selected") {
				REQUIRE(list.position == 2);
				REQUIRE(list.scroll_offset == 0);
			}
		}

		WHEN("the number of items is reduced to 0") {
			list.num_lines = 0;
			list.on_list_changed();

			THEN("the position is reset to 0") {
				REQUIRE(list.position == 0);
				REQUIRE(list.scroll_offset == 0);
			}
		}
	}
}

TEST_CASE("move_up() moves up by 1 line, and respects `wrap_scroll`",
	"[ListMovementControl]")
{
	auto list_movement = ListMovementControl<ListStub>(dummyFormName, dummyForm, 0);
	ListStub& list = list_movement;

	GIVEN("a list of 10 items, where the first item is selected") {
		list.num_lines = 10;
		list_movement.set_position(0);

		WHEN("move_up() is called with wrapping disabled") {
			bool moved = list_movement.move_up(false);

			THEN("the first item is still selected") {
				REQUIRE(moved == false);
				REQUIRE(list.position == 0);
				REQUIRE(list.scroll_offset == 0);
			}
		}

		WHEN("move_up() is called with wrapping enabled") {
			bool moved = list_movement.move_up(true);

			THEN("the last item is selected") {
				REQUIRE(moved == true);
				REQUIRE(list.position == 9);
				REQUIRE(list.scroll_offset == 0);
			}
		}
	}

	GIVEN("a list of 10 items, where the 5th item is selected") {
		list.num_lines = 10;
		list_movement.set_position(4);

		WHEN("move_up() is called with wrapping disabled") {
			bool moved = list_movement.move_up(false);

			THEN("the 4th item is selected") {
				REQUIRE(moved == true);
				REQUIRE(list.position == 3);
				REQUIRE(list.scroll_offset == 0);
			}
		}

		WHEN("move_up() is called with wrapping enabled") {
			bool moved = list_movement.move_up(false);

			THEN("the 4th item is selected") {
				REQUIRE(moved == true);
				REQUIRE(list.position == 3);
				REQUIRE(list.scroll_offset == 0);
			}
		}
	}
}

TEST_CASE("move_down() moves down by 1 line, and respects `wrap_scroll`",
	"[ListMovementControl]")
{
	auto list_movement = ListMovementControl<ListStub>(dummyFormName, dummyForm, 0);
	ListStub& list = list_movement;

	GIVEN("a list of 10 items, where the last item is selected") {
		list.num_lines = 10;
		list_movement.set_position(9);

		WHEN("move_down() is called with wrapping disabled") {
			bool moved = list_movement.move_down(false);

			THEN("the last item is still selected") {
				REQUIRE(moved == false);
				REQUIRE(list.position == 9);
				REQUIRE(list.scroll_offset == 0);
			}
		}

		WHEN("move_down() is called with wrapping enabled") {
			bool moved = list_movement.move_down(true);

			THEN("the first item is selected") {
				REQUIRE(moved == true);
				REQUIRE(list.position == 0);
				REQUIRE(list.scroll_offset == 0);
			}
		}
	}

	GIVEN("a list of 10 items, where the 5th item is selected") {
		list.num_lines = 10;
		list_movement.set_position(4);

		WHEN("move_down() is called with wrapping disabled") {
			bool moved = list_movement.move_down(false);

			THEN("the 6th item is selected") {
				REQUIRE(moved == true);
				REQUIRE(list.position == 5);
				REQUIRE(list.scroll_offset == 0);
			}
		}

		WHEN("move_down() is called with wrapping enabled") {
			bool moved = list_movement.move_down(false);

			THEN("the 6th item is selected") {
				REQUIRE(moved == true);
				REQUIRE(list.position == 5);
				REQUIRE(list.scroll_offset == 0);
			}
		}
	}
}

TEST_CASE("move_to_first() moves to the topmost item", "[ListMovementControl]")
{
	auto list_movement = ListMovementControl<ListStub>(dummyFormName, dummyForm, 0);
	ListStub& list = list_movement;

	GIVEN("a list of 10 items, where the first item is selected") {
		list.num_lines = 10;
		list_movement.set_position(0);

		WHEN("move_to_first() is called") {
			list_movement.move_to_first();

			THEN("the first item is still selected") {
				REQUIRE(list.position == 0);
				REQUIRE(list.scroll_offset == 0);
			}
		}
	}

	GIVEN("a list of 10 items, where the 5th item is selected") {
		list.num_lines = 10;
		list_movement.set_position(4);

		WHEN("move_to_first() is called") {
			list_movement.move_to_first();

			THEN("the first item is selected") {
				REQUIRE(list.position == 0);
				REQUIRE(list.scroll_offset == 0);
			}
		}
	}
}

TEST_CASE("move_to_last() moves to the bottom item", "[ListMovementControl]")
{
	auto list_movement = ListMovementControl<ListStub>(dummyFormName, dummyForm, 0);
	ListStub& list = list_movement;

	GIVEN("a list of 10 items, where the last item is selected") {
		list.num_lines = 10;
		list_movement.set_position(9);

		WHEN("move_to_last() is called") {
			list_movement.move_to_last();

			THEN("the last item is still selected") {
				REQUIRE(list.position == 9);
				REQUIRE(list.scroll_offset == 0);
			}
		}
	}

	GIVEN("a list of 10 items, where the 5th item is selected") {
		list.num_lines = 10;
		list_movement.set_position(4);

		WHEN("move_to_last() is called") {
			list_movement.move_to_last();

			THEN("the last item is selected") {
				REQUIRE(list.position == 9);
				REQUIRE(list.scroll_offset == 0);
			}
		}
	}
}

TEST_CASE("move_page_up() moves up by `list height` lines, and respects `wrap_scroll`",
	"[ListMovementControl]")
{
	auto list_movement = ListMovementControl<ListStub>(dummyFormName, dummyForm, 0);
	ListStub& list = list_movement;

	GIVEN("a list of 10 items, where the first item is selected") {
		list.num_lines = 10;
		list_movement.set_position(0);

		WHEN("move_page_up() is called with wrapping disabled") {
			list_movement.move_page_up(false);

			THEN("the first item is still selected") {
				REQUIRE(list.position == 0);
				REQUIRE(list.scroll_offset == 0);
			}
		}

		WHEN("move_page_up() is called with wrapping enabled") {
			list_movement.move_page_up(true);

			THEN("the last item is selected") {
				REQUIRE(list.position == 9);
				REQUIRE(list.scroll_offset == 0);
			}
		}
	}

	GIVEN("a list of height 4 with 10 items, where the 8th item is selected") {
		list.num_lines = 10;
		list.height = 4;
		list.scroll_offset = 6;
		list_movement.set_position(7);

		WHEN("move_page_up() is called with wrapping disabled") {
			list_movement.move_page_up(false);

			THEN("the first item is selected") {
				REQUIRE(list.position == 3);
				REQUIRE(list.scroll_offset == 3);
			}
		}

		WHEN("move_page_up() is called with wrapping enabled") {
			list_movement.move_page_up(false);

			THEN("the first item is selected") {
				REQUIRE(list.position == 3);
				REQUIRE(list.scroll_offset == 3);
			}
		}
	}
}

TEST_CASE("move_page_down() moves up by `list height` lines, and respects `wrap_scroll`",
	"[ListMovementControl]")
{
	auto list_movement = ListMovementControl<ListStub>(dummyFormName, dummyForm, 0);
	ListStub& list = list_movement;

	GIVEN("a list of 10 items, where the last item is selected") {
		list.num_lines = 10;
		list_movement.set_position(9);

		WHEN("move_page_down() is called with wrapping disabled") {
			list_movement.move_page_down(false);

			THEN("the last item is still selected") {
				REQUIRE(list.position == 9);
				REQUIRE(list.scroll_offset == 0);
			}
		}

		WHEN("move_page_down() is called with wrapping enabled") {
			list_movement.move_page_down(true);

			THEN("the first item is selected") {
				REQUIRE(list.position == 0);
				REQUIRE(list.scroll_offset == 0);
			}
		}
	}

	GIVEN("a list of height 4 with 10 items, where the 2nd item is selected") {
		list.num_lines = 10;
		list.height = 4;
		list_movement.set_position(1);

		WHEN("move_page_down() is called with wrapping disabled") {
			list_movement.move_page_down(false);

			THEN("the 6th item is selected") {
				REQUIRE(list.position == 5);
				REQUIRE(list.scroll_offset == 2);
			}
		}

		WHEN("move_page_down() is called with wrapping enabled") {
			list_movement.move_page_down(false);

			THEN("the 6th item is selected") {
				REQUIRE(list.position == 5);
				REQUIRE(list.scroll_offset == 2);
			}
		}
	}
}

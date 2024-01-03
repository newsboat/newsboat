#include "keycombination.h"

#include "3rd-party/catch.hpp"
#include "test_helpers/stringmaker/keycombination.h"

using namespace newsboat;

TEST_CASE("KeyCombination parses bind-key key combinations", "[KeyCombination]")
{
	const auto check = [](const std::string& input, const std::string& key, bool shift,
	bool control, bool alt) {
		const auto key_combination = KeyCombination::from_bindkey(input);
		REQUIRE(key_combination.get_key() == key);
		REQUIRE(key_combination.has_shift() == shift);
		REQUIRE(key_combination.has_control() == control);
		REQUIRE(key_combination.has_alt() == alt);
	};

	check("a", "a", false, false, false);
	check("A", "a", true, false, false);
	check("^a", "a", false, true, false);
	check("^A", "a", false, true, false);

	check("<", "<", false, false, false);
	check(">", ">", false, false, false);
	check("ENTER", "ENTER", false, false, false);
}

TEST_CASE("KeyCombination can output bind-key styled strings", "[KeyCombination]")
{
	const auto check = [](const std::string& expected, const std::string& key,
	ShiftState shift, ControlState control) {
		REQUIRE(KeyCombination(key, shift, control,
				AltState::NoAlt).to_bindkey_string() == expected);
	};

	check("a", "a", ShiftState::NoShift, ControlState::NoControl);
	check("A", "a", ShiftState::Shift, ControlState::NoControl);
	check("^A", "a", ShiftState::NoShift, ControlState::Control);
	check("^A", "a", ShiftState::Shift, ControlState::Control);

	check("ENTER", "ENTER", ShiftState::NoShift, ControlState::NoControl);
}

TEST_CASE("KeyCombination equality operator", "[KeyCombination]")
{
	SECTION("KeyCombinations equal") {
		REQUIRE(KeyCombination::from_bindkey("a")
			== KeyCombination("a", ShiftState::NoShift, ControlState::NoControl, AltState::NoAlt));
		REQUIRE(KeyCombination("a", ShiftState::NoShift, ControlState::NoControl, AltState::NoAlt)
			== KeyCombination("a", ShiftState::NoShift, ControlState::NoControl, AltState::NoAlt));
		REQUIRE(KeyCombination("a", ShiftState::Shift, ControlState::Control, AltState::Alt)
			== KeyCombination("a", ShiftState::Shift, ControlState::Control, AltState::Alt));
	}

	SECTION("KeyCombinations differ") {
		REQUIRE_FALSE(KeyCombination::from_bindkey("a")
			== KeyCombination("a", ShiftState::Shift, ControlState::NoControl, AltState::NoAlt));
		REQUIRE_FALSE(KeyCombination::from_bindkey("a")
			== KeyCombination("a", ShiftState::NoShift, ControlState::Control, AltState::NoAlt));
		REQUIRE_FALSE(KeyCombination::from_bindkey("a")
			== KeyCombination("a", ShiftState::NoShift, ControlState::NoControl, AltState::Alt));

		REQUIRE_FALSE(KeyCombination::from_bindkey("^A")
			== KeyCombination("a", ShiftState::Shift, ControlState::Control, AltState::NoAlt));
		REQUIRE_FALSE(KeyCombination::from_bindkey("^A")
			== KeyCombination("a", ShiftState::NoShift, ControlState::NoControl, AltState::NoAlt));
		REQUIRE_FALSE(KeyCombination::from_bindkey("^A")
			== KeyCombination("a", ShiftState::NoShift, ControlState::Control, AltState::Alt));

		REQUIRE_FALSE(
			KeyCombination("a", ShiftState::NoShift, ControlState::NoControl, AltState::NoAlt)
			== KeyCombination("a", ShiftState::Shift, ControlState::NoControl, AltState::NoAlt));
		REQUIRE_FALSE(
			KeyCombination("a", ShiftState::NoShift, ControlState::NoControl, AltState::NoAlt)
			== KeyCombination("a", ShiftState::NoShift, ControlState::Control, AltState::NoAlt));
		REQUIRE_FALSE(
			KeyCombination("a", ShiftState::NoShift, ControlState::NoControl, AltState::NoAlt)
			== KeyCombination("a", ShiftState::NoShift, ControlState::NoControl, AltState::Alt));
	}
}

TEST_CASE("KeyCombination less-than operator", "[KeyCombination]")
{
	SECTION("Consistent results") {
		const auto check_smaller = [](const KeyCombination& lhs, const KeyCombination& rhs) {
			REQUIRE(lhs < rhs);
			REQUIRE_FALSE(rhs < lhs);
		};

		check_smaller(
			KeyCombination("a", ShiftState::NoShift, ControlState::NoControl, AltState::NoAlt),
			KeyCombination("b", ShiftState::NoShift, ControlState::NoControl, AltState::NoAlt));
		check_smaller(
			KeyCombination("a", ShiftState::Shift, ControlState::NoControl, AltState::NoAlt),
			KeyCombination("a", ShiftState::NoShift, ControlState::NoControl, AltState::NoAlt));
		check_smaller(
			KeyCombination("a", ShiftState::NoShift, ControlState::Control, AltState::NoAlt),
			KeyCombination("a", ShiftState::NoShift, ControlState::NoControl, AltState::NoAlt));
		check_smaller(
			KeyCombination("a", ShiftState::NoShift, ControlState::NoControl, AltState::Alt),
			KeyCombination("a", ShiftState::NoShift, ControlState::NoControl, AltState::NoAlt));
	}

	SECTION("Equal values") {
		REQUIRE_FALSE(
			KeyCombination("a", ShiftState::NoShift, ControlState::NoControl, AltState::NoAlt)
			< KeyCombination("a", ShiftState::NoShift, ControlState::NoControl, AltState::NoAlt));
		REQUIRE_FALSE(
			KeyCombination("ENTER", ShiftState::Shift, ControlState::Control, AltState::Alt)
			< KeyCombination("ENTER", ShiftState::Shift, ControlState::Control, AltState::Alt));
	}
}

#include "keycombination.h"

#include "3rd-party/catch.hpp"
#include "test_helpers/stringmaker/keycombination.h"

using namespace Newsboat;

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

	check("<", "LT", false, false, false);
	check(">", "GT", false, false, false);
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

TEST_CASE("to_bind_string", "[KeyCombination]")
{
	REQUIRE(KeyCombination("a").to_bind_string() == "a");
	REQUIRE(KeyCombination("=").to_bind_string() == "=");
	REQUIRE(KeyCombination("^").to_bind_string() == "<^>");
	REQUIRE(KeyCombination(">").to_bind_string() == ">");
	REQUIRE(KeyCombination("a", ShiftState::Shift).to_bind_string() == "A");
	REQUIRE(KeyCombination("a", ShiftState::NoShift,
			ControlState::Control).to_bind_string() == "^A");
	REQUIRE(KeyCombination("ENTER").to_bind_string() == "<ENTER>");

	REQUIRE(KeyCombination("F1", ShiftState::NoShift, ControlState::NoControl,
			AltState::NoAlt).to_bind_string() == "<F1>");
	REQUIRE(KeyCombination("F1", ShiftState::NoShift, ControlState::NoControl,
			AltState::Alt).to_bind_string() == "<M-F1>");
	REQUIRE(KeyCombination("F1", ShiftState::NoShift, ControlState::Control,
			AltState::NoAlt).to_bind_string() == "<C-F1>");
	REQUIRE(KeyCombination("F1", ShiftState::NoShift, ControlState::Control,
			AltState::Alt).to_bind_string() == "<C-M-F1>");
	REQUIRE(KeyCombination("F1", ShiftState::Shift, ControlState::NoControl,
			AltState::NoAlt).to_bind_string() == "<S-F1>");
	REQUIRE(KeyCombination("F1", ShiftState::Shift, ControlState::NoControl,
			AltState::Alt).to_bind_string() == "<S-M-F1>");
	REQUIRE(KeyCombination("F1", ShiftState::Shift, ControlState::Control,
			AltState::NoAlt).to_bind_string() == "<C-S-F1>");
	REQUIRE(KeyCombination("F1", ShiftState::Shift, ControlState::Control,
			AltState::Alt).to_bind_string() == "<C-S-M-F1>");
}

TEST_CASE("from_bind parses sequence of key combinations", "[KeyCombination]")
{
	SECTION("Empty input") {
		const auto key_combinations = KeyCombination::from_bind("");
		REQUIRE(key_combinations.size() == 0);
	}

	SECTION("Single key") {
		SECTION("Regular letter key") {
			const auto key_combinations = KeyCombination::from_bind("g");
			REQUIRE(key_combinations.size() == 1);
			INFO(key_combinations[0].to_bind_string());
			REQUIRE(key_combinations[0] == KeyCombination("g"));
		}
		SECTION("Letter with shift") {
			const auto key_combinations = KeyCombination::from_bind("G");
			REQUIRE(key_combinations.size() == 1);
			INFO(key_combinations[0].to_bind_string());
			REQUIRE(key_combinations[0] == KeyCombination("g", ShiftState::Shift));
		}
		SECTION("Letter with control") {
			const auto key_combinations = KeyCombination::from_bind("^G");
			REQUIRE(key_combinations.size() == 1);
			INFO(key_combinations[0].to_bind_string());
			REQUIRE(key_combinations[0] == KeyCombination("g", ShiftState::NoShift,
					ControlState::Control));
		}
		SECTION("Symbol key") {
			const auto key_combinations = KeyCombination::from_bind("=");
			REQUIRE(key_combinations.size() == 1);
			INFO(key_combinations[0].to_bind_string());
			REQUIRE(key_combinations[0] == KeyCombination("="));
		}
		SECTION("Special key") {
			const auto key_combinations = KeyCombination::from_bind("<SPACE>");
			REQUIRE(key_combinations.size() == 1);
			INFO(key_combinations[0].to_bind_string());
			REQUIRE(key_combinations[0] == KeyCombination("SPACE"));
		}
		SECTION("Special key with modifiers") {
			std::vector<std::pair<std::string, KeyCombination>> test_cases = {
				{ "<SPACE>", KeyCombination("SPACE") },
				{ "<M-SPACE>", KeyCombination("SPACE", ShiftState::NoShift, ControlState::NoControl, AltState::Alt) },
				{ "<S-SPACE>", KeyCombination("SPACE", ShiftState::Shift, ControlState::NoControl, AltState::NoAlt) },
				{ "<S-M-SPACE>", KeyCombination("SPACE", ShiftState::Shift, ControlState::NoControl, AltState::Alt) },
				{ "<C-SPACE>", KeyCombination("SPACE", ShiftState::NoShift, ControlState::Control, AltState::NoAlt) },
				{ "<C-M-SPACE>", KeyCombination("SPACE", ShiftState::NoShift, ControlState::Control, AltState::Alt) },
				{ "<C-S-SPACE>", KeyCombination("SPACE", ShiftState::Shift, ControlState::Control, AltState::NoAlt) },
				{ "<C-S-M-SPACE>", KeyCombination("SPACE", ShiftState::Shift, ControlState::Control, AltState::Alt) },
			};

			for (const auto& test_case : test_cases) {
				DYNAMIC_SECTION("Expected output: " << test_case.second.to_bind_string()) {
					const auto key_combinations = KeyCombination::from_bind(test_case.first);
					REQUIRE(key_combinations.size() == 1);
					INFO(key_combinations[0].to_bind_string());
					REQUIRE(key_combinations[0] == test_case.second);
				}
			}
		}
		SECTION("Special key with modifiers in nonstandard order") {
			const auto key_combinations = KeyCombination::from_bind("<M-S-C-key>");
			REQUIRE(key_combinations.size() == 1);
			INFO(key_combinations[0].to_bind_string());
			REQUIRE(key_combinations[0] == KeyCombination("key", ShiftState::Shift,
					ControlState::Control, AltState::Alt));
		}
	}

	SECTION("Sequence of multiple keys") {
		SECTION("Regular keys") {
			const auto key_combinations = KeyCombination::from_bind("abc");
			REQUIRE(key_combinations.size() == 3);
			REQUIRE(key_combinations[0] == KeyCombination("a"));
			REQUIRE(key_combinations[1] == KeyCombination("b"));
			REQUIRE(key_combinations[2] == KeyCombination("c"));
		}
		SECTION("Special keys") {
			const auto key_combinations = KeyCombination::from_bind("<F1><S-ENTER>");
			REQUIRE(key_combinations.size() == 2);
			REQUIRE(key_combinations[0] == KeyCombination("F1"));
			REQUIRE(key_combinations[1] == KeyCombination("ENTER", ShiftState::Shift));
		}
		SECTION("Mixed") {
			const auto key_combinations = KeyCombination::from_bind("^G<ENTER>p");
			REQUIRE(key_combinations.size() == 3);
			REQUIRE(key_combinations[0] == KeyCombination("g", ShiftState::NoShift,
					ControlState::Control));
			REQUIRE(key_combinations[1] == KeyCombination("ENTER"));
			REQUIRE(key_combinations[2] == KeyCombination("p"));
		}
	}
}

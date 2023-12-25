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
	const auto check = [](const std::string& expected, const std::string& key, bool shift,
	bool control) {
		REQUIRE(KeyCombination(key, shift, control, false).to_bindkey_string() == expected);
	};

	check("a", "a", false, false);
	check("A", "a", true, false);
	check("^A", "a", false, true);
	check("^A", "a", true, true);

	check("ENTER", "ENTER", false, false);
}

TEST_CASE("KeyCombination equality operator", "[KeyCombination]")
{
	SECTION("KeyCombinations equal") {
		REQUIRE(KeyCombination::from_bindkey("a")
			== KeyCombination("a", false, false, false));
		REQUIRE(KeyCombination("a", false, false, false)
			== KeyCombination("a", false, false, false));
		REQUIRE(KeyCombination("a", true, true, true)
			== KeyCombination("a", true, true, true));
	}

	SECTION("KeyCombinations differ") {
		REQUIRE_FALSE(KeyCombination::from_bindkey("a")
			== KeyCombination("a", true, false, false));
		REQUIRE_FALSE(KeyCombination::from_bindkey("a")
			== KeyCombination("a", false, true, false));
		REQUIRE_FALSE(KeyCombination::from_bindkey("a")
			== KeyCombination("a", false, false, true));

		REQUIRE_FALSE(KeyCombination::from_bindkey("^A")
			== KeyCombination("a", true, true, false));
		REQUIRE_FALSE(KeyCombination::from_bindkey("^A")
			== KeyCombination("a", false, false, false));
		REQUIRE_FALSE(KeyCombination::from_bindkey("^A")
			== KeyCombination("a", false, true, true));

		REQUIRE_FALSE(KeyCombination("a", false, false, false)
			== KeyCombination("a", true, false, false));
		REQUIRE_FALSE(KeyCombination("a", false, false, false)
			== KeyCombination("a", false, true, false));
		REQUIRE_FALSE(KeyCombination("a", false, false, false)
			== KeyCombination("a", false, false, true));
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
			KeyCombination("a", false, false, false),
			KeyCombination("b", false, false, false));
		check_smaller(
			KeyCombination("a", false, false, false),
			KeyCombination("a", true, false, false));
		check_smaller(
			KeyCombination("a", false, false, false),
			KeyCombination("a", false, true, false));
		check_smaller(
			KeyCombination("a", false, false, false),
			KeyCombination("a", false, false, true));
	}

	SECTION("Equal values") {
		REQUIRE_FALSE(KeyCombination("a", false, false, false)
			< KeyCombination("a", false, false, false));
		REQUIRE_FALSE(KeyCombination("ENTER", true, true, true)
			< KeyCombination("ENTER", true, true, true));
	}
}

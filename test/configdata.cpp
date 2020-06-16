#include "3rd-party/catch.hpp"

#include "configdata.h"

using namespace newsboat;

TEST_CASE("set_value() doesn't return errors if new value is aligned "
	"with the setting's type",
	"[ConfigData]")
{
	SECTION("boolean") {
		ConfigData setting("yes", ConfigDataType::BOOL);

		REQUIRE(setting.set_value("no"));
		REQUIRE(setting.set_value("yes"));
		REQUIRE(setting.set_value("true"));
		REQUIRE(setting.set_value("false"));
	}

	SECTION("unsigned integer") {
		ConfigData setting("42", ConfigDataType::INT);

		REQUIRE(setting.set_value("13"));
		REQUIRE(setting.set_value("100500"));
		REQUIRE(setting.set_value("65535"));
	}

	SECTION("enum") {
		ConfigData setting("charlie", {"alpha", "bravo", "charlie", "delta"});

		REQUIRE(setting.set_value("alpha"));
		REQUIRE(setting.set_value("bravo"));
		REQUIRE(setting.set_value("charlie"));
		REQUIRE(setting.set_value("delta"));
	}

	SECTION("string") {
		ConfigData setting("johndoe", ConfigDataType::STR);

		REQUIRE(setting.set_value("minoru"));
		REQUIRE(setting.set_value("noname"));
		REQUIRE(setting.set_value("username"));
		REQUIRE(setting.set_value("nobody"));
	}

	SECTION("path") {
		ConfigData setting("~/urls", ConfigDataType::PATH);

		REQUIRE(setting.set_value("/tmp/whatever.txt"));
		REQUIRE(setting.set_value("C:\\Users\\Minoru\\urls.txt"));
		REQUIRE(setting.set_value("/usr/local/home/minoru/.newsboat/urls"));
	}
}

TEST_CASE("set_value() returns error if new value for a boolean setting is not "
	"a recognized boolean",
	"[ConfigData]")
{
	ConfigData setting("yes", ConfigDataType::BOOL);

	REQUIRE_FALSE(setting.set_value("enable"));
	REQUIRE_FALSE(setting.set_value("disabled"));
	REQUIRE_FALSE(setting.set_value("active"));
}

TEST_CASE("set_value() returns error if new value for a \"number\" setting "
	"is not a sequence of digits",
	"[ConfigData]")
{
	ConfigData setting("yes", ConfigDataType::INT);

	REQUIRE_FALSE(setting.set_value("0x42"));
	REQUIRE_FALSE(setting.set_value("infinity"));
	REQUIRE_FALSE(setting.set_value("123 minutes"));
}

TEST_CASE("set_value() returns error if new value for an \"enum\" setting "
	"does not belong to the enum",
	"[ConfigData]")
{
	ConfigData setting("N", {"H", "He", "Li", "Be", "B", "C", "N", "O", "F"});

	REQUIRE_FALSE(setting.set_value("Mg"));
	REQUIRE_FALSE(setting.set_value("Al"));
	REQUIRE_FALSE(setting.set_value("something entirely different"));
}

#include "test-helpers.h"

#include "3rd-party/catch.hpp"

TEST_CASE("EnvVar object restores the environment variable to its original "
		"state when the object is destroyed",
		"[test-helpers]")
{
	const char var[] = "nEwSb0a7-tEsT-eNvIroNm3Nt-v4rIabLe";
	{
		INFO("Checking that noone else is using that environment variable");
		REQUIRE(::getenv(var) == nullptr);
	}

	const auto expected = std::string("let's try this out, shall we?");

	const auto overwrite = true;

	::setenv(var, expected.c_str(), (int)overwrite);
	REQUIRE(expected == ::getenv(var));

	{
		TestHelpers::EnvVar envVar(var);
		REQUIRE(expected == ::getenv(var));

		const auto newValue = std::string("totally new value");
		::setenv(var, newValue.c_str(), (int)overwrite);

		REQUIRE_FALSE(expected == ::getenv(var));
	}

	REQUIRE(expected == ::getenv(var));

	::unsetenv(var);
}

TEST_CASE("EnvVar::set() changes the current state of the environment variable",
		"[test-helpers]")
{
	const char var[] = "nEwSb0a7-tEsT-eNvIroNm3Nt-v4rIabLe";
	{
		INFO("Checking that noone else is using that environment variable");
		REQUIRE(::getenv(var) == nullptr);
	}

	const auto expected = std::string("let's try this out, shall we?");

	TestHelpers::EnvVar envVar(var);

	envVar.set("absolutely new value");
	REQUIRE_FALSE(expected == ::getenv(var));

	envVar.set(expected);
	REQUIRE(expected == ::getenv(var));
}

TEST_CASE("EnvVar::set() doesn't change the value to which the environment "
		"variable is restored",
		"[test-helpers]")
{
	const char var[] = "nEwSb0a7-tEsT-eNvIroNm3Nt-v4rIabLe";
	{
		INFO("Checking that noone else is using that environment variable");
		REQUIRE(::getenv(var) == nullptr);
	}

	const auto expected = std::string("let's try this out, shall we?");

	const auto overwrite = true;
	::setenv(var, expected.c_str(), (int)overwrite);
	REQUIRE(expected == ::getenv(var));

	{
		TestHelpers::EnvVar envVar(var);
		envVar.set("some new value");
	}

	REQUIRE(expected == ::getenv(var));

	::unsetenv(var);
}

TEST_CASE("EnvVar::set() runs a function (set by on_change()) after changing "
		"the environment variable",
		"[test-helpers]")
{
	const char var[] = "nEwSb0a7-tEsT-eNvIroNm3Nt-v4rIabLe";
	{
		INFO("Checking that noone else is using that environment variable");
		REQUIRE(::getenv(var) == nullptr);
	}

	const auto expected = std::string("let's try this out, shall we?");

	const auto overwrite = true;
	::setenv(var, expected.c_str(), (int)overwrite);

	SECTION("The function is ran *after* the change") {
		TestHelpers::EnvVar envVar(var);

		const auto newValue = std::string("totally new value here");
		auto valueChanged = false;
		envVar.on_change([&valueChanged, &newValue, &var](){
			valueChanged = newValue == ::getenv(var);
		});

		envVar.set(newValue);

		REQUIRE(valueChanged);
	}

	SECTION("The function is ran *once* per change") {
		TestHelpers::EnvVar envVar(var);

		auto counter = unsigned{};
		envVar.on_change([&counter](){ counter++; });

		REQUIRE(counter == 0);
		envVar.set("new value");
		REQUIRE(counter == 1);
		envVar.set("some other value");
		REQUIRE(counter == 2);

		INFO("Same value as before, but function should still be called");
		envVar.set("some other value");
		REQUIRE(counter == 3);
	}

	::unsetenv(var);
}

TEST_CASE("EnvVar::unset() completely removes the variable from the environment",
		"[test-helpers]")
{
	const char var[] = "nEwSb0a7-tEsT-eNvIroNm3Nt-v4rIabLe";
	{
		INFO("Checking that noone else is using that environment variable");
		REQUIRE(::getenv(var) == nullptr);
	}

	const auto expected = std::string("let's try this out, shall we?");

	const auto overwrite = true;
	::setenv(var, expected.c_str(), (int)overwrite);

	char* value = ::getenv(var);
	REQUIRE_FALSE(value == nullptr);
	REQUIRE(expected == value);
	value = nullptr;

	{
		TestHelpers::EnvVar envVar(var);

		value = ::getenv(var);
		REQUIRE_FALSE(value == nullptr);
		REQUIRE(expected == value);
		value = nullptr;

		envVar.unset();

		value = ::getenv(var);
		REQUIRE(value == nullptr);
		value = nullptr;
	}

	::unsetenv(var);
}

TEST_CASE("EnvVar::unset() doesn't change the value to which the environment "
		"variable is restored",
		"[test-helpers]")
{
	const char var[] = "nEwSb0a7-tEsT-eNvIroNm3Nt-v4rIabLe";
	{
		INFO("Checking that noone else is using that environment variable");
		REQUIRE(::getenv(var) == nullptr);
	}

	const auto expected = std::string("let's try this out, shall we?");

	const auto overwrite = true;
	::setenv(var, expected.c_str(), (int)overwrite);

	char* value = ::getenv(var);
	REQUIRE_FALSE(value == nullptr);
	REQUIRE(expected == value);
	value = nullptr;

	{
		TestHelpers::EnvVar envVar(var);

		value = ::getenv(var);
		REQUIRE_FALSE(value == nullptr);
		REQUIRE(expected == value);
		value = nullptr;

		envVar.unset();

		value = ::getenv(var);
		REQUIRE(value == nullptr);
		value = nullptr;
	}

	value = ::getenv(var);
	REQUIRE_FALSE(value == nullptr);
	REQUIRE(expected == value);
	value = nullptr;

	::unsetenv(var);
}

TEST_CASE("EnvVar::unset() runs a function (set by on_change()) after changing "
		"the environment variable",
		"[test-helpers]")
{
	const char var[] = "nEwSb0a7-tEsT-eNvIroNm3Nt-v4rIabLe";
	{
		INFO("Checking that noone else is using that environment variable");
		REQUIRE(::getenv(var) == nullptr);
	}

	const auto expected = std::string("let's try this out, shall we?");

	const auto overwrite = true;
	::setenv(var, expected.c_str(), (int)overwrite);

	char* value = ::getenv(var);
	REQUIRE_FALSE(value == nullptr);
	REQUIRE(expected == value);
	value = nullptr;

	{
		TestHelpers::EnvVar envVar(var);

		auto counter = unsigned{};
		auto as_expected = false;
		envVar.on_change([&counter, &as_expected, var]() {
			as_expected = nullptr == ::getenv(var);
			counter++;
		});

		value = ::getenv(var);
		REQUIRE_FALSE(value == nullptr);
		REQUIRE(expected == value);
		value = nullptr;

		envVar.unset();

		REQUIRE(as_expected);
		REQUIRE(counter == 1);
	}

	::unsetenv(var);
}

TEST_CASE("EnvVar's destructor runs a function (set by on_change()) after "
		"restoring the varibale to its original state",
		"[test-helpers]")
{
	const char var[] = "nEwSb0a7-tEsT-eNvIroNm3Nt-v4rIabLe";
	{
		INFO("Checking that noone else is using that environment variable");
		REQUIRE(::getenv(var) == nullptr);
	}

	const auto expected = std::string("let's try this out, shall we?");

	SECTION("Variable wasn't even set") {
	}

	SECTION("Variable was set before EnvVar is created") {
		const auto overwrite = true;
		::setenv(var, expected.c_str(), (int)overwrite);
	}

	auto counter = unsigned{};

	{
		TestHelpers::EnvVar envVar(var);
		envVar.on_change([&counter](){ counter++; });
	}

	REQUIRE(counter == 1);

	// It's a no-op if the variable is absent from the environment, so we don't
	// need to put this inside a conditional to match SECTIONs above.
	::unsetenv(var);
}


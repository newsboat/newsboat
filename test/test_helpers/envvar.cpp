#include "envvar.h"

#include <stdexcept>

#include "3rd-party/catch.hpp"

test_helpers::EnvVar::EnvVar(std::string name_)
	: EnvVar(std::move(name_), true)
{
	if (name == "TZ") {
		throw std::invalid_argument("Using EnvVar(\"TZ\") is discouraged. Try test_helpers::TzEnvVar instead.");
	} else if (name == "LC_CTYPE") {
		throw std::invalid_argument("Using EnvVar(\"LC_CTYPE\") is discouraged. Try test_helpers::LcCtypeEnvVar instead.");
	}
}

test_helpers::EnvVar::EnvVar(std::string name_, bool /* unused */)
	: name(std::move(name_))
{
	const char* original = ::getenv(name.c_str());
	was_set = original != nullptr;
	if (was_set) {
		value = std::string(original);
	}
}

test_helpers::EnvVar::~EnvVar()
{
	if (was_set) {
		set(value);
	} else {
		unset();
	}
}

void test_helpers::EnvVar::set(const std::string& new_value) const
{
	const auto overwrite = true;
	::setenv(name.c_str(), new_value.c_str(), overwrite);
	if (on_change_fn) {
		on_change_fn(new_value);
	}
}

void test_helpers::EnvVar::unset() const
{
	::unsetenv(name.c_str());
	if (on_change_fn) {
		on_change_fn(std::nullopt);
	}
}

void test_helpers::EnvVar::on_change(
	std::function<void(std::optional<std::string>)> fn)
{
	on_change_fn = std::move(fn);
}

test_helpers::TzEnvVar::TzEnvVar()
	: EnvVar("TZ", true)
{
	on_change([](std::optional<std::string>) {
		::tzset();
	});
}

test_helpers::LcCtypeEnvVar::LcCtypeEnvVar()
	: EnvVar("LC_CTYPE", true)
{
	on_change([](std::optional<std::string> new_charset) {
		if (new_charset.has_value()) {
			::setlocale(LC_CTYPE, new_charset.value().c_str());
		} else {
			::setlocale(LC_CTYPE, "");
		}
	});
}


TEST_CASE("EnvVar object restores the environment variable to its original "
	"state when the object is destroyed",
	"[test_helpers]")
{
	const char var[] = "nEwSb0a7-tEsT-eNvIroNm3Nt-v4rIabLe";
	{
		INFO("Checking that noone else is using that environment variable");
		REQUIRE(::getenv(var) == nullptr);
	}

	const auto expected = std::string("let's try this out, shall we?");

	const auto overwrite = true;

	REQUIRE(::setenv(var, expected.c_str(), overwrite) == 0);
	REQUIRE(expected == ::getenv(var));

	{
		test_helpers::EnvVar envVar(var);
		REQUIRE(expected == ::getenv(var));

		const auto newValue = std::string("totally new value");
		REQUIRE(::setenv(var, newValue.c_str(), overwrite) == 0);

		REQUIRE_FALSE(expected == ::getenv(var));
	}

	REQUIRE(expected == ::getenv(var));

	REQUIRE(::unsetenv(var) == 0);
}

TEST_CASE("EnvVar::set() changes the current state of the environment variable",
	"[test_helpers]")
{
	const char var[] = "nEwSb0a7-tEsT-eNvIroNm3Nt-v4rIabLe";
	{
		INFO("Checking that noone else is using that environment variable");
		REQUIRE(::getenv(var) == nullptr);
	}

	const auto expected = std::string("let's try this out, shall we?");

	test_helpers::EnvVar envVar(var);

	envVar.set("absolutely new value");
	REQUIRE_FALSE(expected == ::getenv(var));

	envVar.set(expected);
	REQUIRE(expected == ::getenv(var));
}

TEST_CASE("EnvVar::set() doesn't change the value to which the environment "
	"variable is restored",
	"[test_helpers]")
{
	const char var[] = "nEwSb0a7-tEsT-eNvIroNm3Nt-v4rIabLe";
	{
		INFO("Checking that noone else is using that environment variable");
		REQUIRE(::getenv(var) == nullptr);
	}

	const auto expected = std::string("let's try this out, shall we?");

	const auto overwrite = true;
	REQUIRE(::setenv(var, expected.c_str(), overwrite) == 0);
	REQUIRE(expected == ::getenv(var));

	{
		test_helpers::EnvVar envVar(var);
		envVar.set("some new value");
	}

	REQUIRE(expected == ::getenv(var));

	REQUIRE(::unsetenv(var) == 0);
}

TEST_CASE("EnvVar::set() runs a function (set by on_change()) after changing "
	"the environment variable",
	"[test_helpers]")
{
	const char var[] = "nEwSb0a7-tEsT-eNvIroNm3Nt-v4rIabLe";
	{
		INFO("Checking that noone else is using that environment variable");
		REQUIRE(::getenv(var) == nullptr);
	}

	const auto expected = std::string("let's try this out, shall we?");

	const auto overwrite = true;
	REQUIRE(::setenv(var, expected.c_str(), overwrite) == 0);

	SECTION("The function is ran *after* the change") {
		// It's important to declare `newValue` before declaring `envVar`,
		// because the string will be used in `on_change` function which is
		// also ran on `envVar` destruction. If we declare `newValue` after
		// `envVar`, it will be destroyed *before* `envVar`, and `on_change`
		// function will try to access an already-freed memory.
		const auto newValue = std::string("totally new value here");
		auto valueChanged = false;

		test_helpers::EnvVar envVar(var);
		envVar.on_change([&valueChanged,
		&var](std::optional<std::string> new_value) {
			valueChanged = new_value.has_value() && (new_value.value() == ::getenv(var));
		});

		envVar.set(newValue);

		REQUIRE(valueChanged);
	}

	SECTION("The function is ran *once* per change") {
		auto counter = unsigned{};

		test_helpers::EnvVar envVar(var);
		envVar.on_change([&counter](std::optional<std::string>) {
			counter++;
		});

		REQUIRE(counter == 0);
		envVar.set("new value");
		REQUIRE(counter == 1);
		envVar.set("some other value");
		REQUIRE(counter == 2);

		INFO("Same value as before, but function should still be called");
		envVar.set("some other value");
		REQUIRE(counter == 3);
	}

	SECTION("Function is passed a non-empty `optional`") {
		const auto expected_new_value = std::string("test sentry");
		auto checks_ok = false;

		test_helpers::EnvVar envVar(var);
		envVar.on_change([&expected_new_value,
		&checks_ok](std::optional<std::string> new_value) {
			checks_ok = new_value.has_value() && (new_value.value() == expected_new_value);
		});

		envVar.set(expected_new_value);

		REQUIRE(checks_ok);
	}

	REQUIRE(::unsetenv(var) == 0);
}

TEST_CASE("EnvVar::unset() completely removes the variable from the environment",
	"[test_helpers]")
{
	const char var[] = "nEwSb0a7-tEsT-eNvIroNm3Nt-v4rIabLe";
	{
		INFO("Checking that noone else is using that environment variable");
		REQUIRE(::getenv(var) == nullptr);
	}

	const auto expected = std::string("let's try this out, shall we?");

	const auto overwrite = true;
	REQUIRE(::setenv(var, expected.c_str(), overwrite) == 0);

	char* value = ::getenv(var);
	REQUIRE_FALSE(value == nullptr);
	REQUIRE(expected == value);
	value = nullptr;

	{
		test_helpers::EnvVar envVar(var);

		value = ::getenv(var);
		REQUIRE_FALSE(value == nullptr);
		REQUIRE(expected == value);
		value = nullptr;

		envVar.unset();

		value = ::getenv(var);
		REQUIRE(value == nullptr);
		value = nullptr;
	}

	REQUIRE(::unsetenv(var) == 0);
}

TEST_CASE("EnvVar::unset() doesn't change the value to which the environment "
	"variable is restored",
	"[test_helpers]")
{
	const char var[] = "nEwSb0a7-tEsT-eNvIroNm3Nt-v4rIabLe";
	{
		INFO("Checking that noone else is using that environment variable");
		REQUIRE(::getenv(var) == nullptr);
	}

	const auto expected = std::string("let's try this out, shall we?");

	const auto overwrite = true;
	REQUIRE(::setenv(var, expected.c_str(), overwrite) == 0);

	char* value = ::getenv(var);
	REQUIRE_FALSE(value == nullptr);
	REQUIRE(expected == value);
	value = nullptr;

	{
		test_helpers::EnvVar envVar(var);

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

	REQUIRE(::unsetenv(var) == 0);
}

TEST_CASE("EnvVar::unset() runs a function (set by on_change()) after changing "
	"the environment variable",
	"[test_helpers]")
{
	const char var[] = "nEwSb0a7-tEsT-eNvIroNm3Nt-v4rIabLe";
	{
		INFO("Checking that noone else is using that environment variable");
		REQUIRE(::getenv(var) == nullptr);
	}

	const auto expected = std::string("let's try this out, shall we?");

	const auto overwrite = true;
	REQUIRE(::setenv(var, expected.c_str(), overwrite) == 0);

	SECTION("The function is ran *after* the change") {
		auto value_unset = false;

		test_helpers::EnvVar envVar(var);
		envVar.on_change([&value_unset, &var](std::optional<std::string> new_value) {
			value_unset = !new_value.has_value() && (nullptr == ::getenv(var));
		});

		envVar.unset();

		REQUIRE(value_unset);
	}

	SECTION("The function is run *once* per change") {
		auto counter = unsigned{};

		test_helpers::EnvVar envVar(var);
		envVar.on_change([&counter](std::optional<std::string>) {
			counter++;
		});

		envVar.unset();

		REQUIRE(counter == 1);

		envVar.unset();

		REQUIRE(counter == 2);
	}

	SECTION("Function is passed `nullopt`") {
		auto checks_ok = false;

		test_helpers::EnvVar envVar(var);
		envVar.on_change([&checks_ok](std::optional<std::string> new_value) {
			checks_ok = !new_value.has_value();
		});

		envVar.unset();

		REQUIRE(checks_ok);
	}


	REQUIRE(::unsetenv(var) == 0);
}

TEST_CASE("EnvVar's destructor runs a function (set by on_change()) after "
	"restoring the variable to its original state",
	"[test_helpers]")
{
	const char var[] = "nEwSb0a7-tEsT-eNvIroNm3Nt-v4rIabLe";
	{
		INFO("Checking that noone else is using that environment variable");
		REQUIRE(::getenv(var) == nullptr);
	}

	const auto expected = std::string("let's try this out, shall we?");

	const auto check = [&]() {
		auto counter = unsigned{};

		{
			test_helpers::EnvVar envVar(var);
			envVar.on_change([&counter](std::optional<std::string>) {
				counter++;
			});
		}

		REQUIRE(counter == 1);
	};

	SECTION("Variable wasn't even set") {
		// Intentionally left empty.
		//
		// Catch framework runs each test case multiple times, each time
		// entering some section it hasn't entered before. In our situation
		// here, the test case will be ran twice: once with setenv() being
		// called, and once without. As a result, EnvVar below will be created
		// in two different situations: once when the environment variable is
		// already present, and once when it's absent. The point of the test is
		// that this wouldn't matter: EnvVar will behave the same regardless.

		check();
	}

	SECTION("Variable was set before EnvVar is created") {
		const auto overwrite = true;
		REQUIRE(::setenv(var, expected.c_str(), overwrite) == 0);

		check();
	}

	// It's a no-op if the variable is absent from the environment, so we don't
	// need to put this inside a conditional to match SECTIONs above.
	REQUIRE(::unsetenv(var) == 0);
}

TEST_CASE("EnvVar can't be constructed for TZ variable", "[test_helpers]")
{
	REQUIRE_THROWS_AS(test_helpers::EnvVar("TZ"), std::invalid_argument);
}

TEST_CASE("EnvVar can't be constructed for LC_CTYPE variable", "[test_helpers]")
{
	REQUIRE_THROWS_AS(test_helpers::EnvVar("LC_CTYPE"), std::invalid_argument);
}

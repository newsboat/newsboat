#include "catch.hpp"

#include <configcontainer.h>
#include <configparser.h>
#include <keymap.h>
#include <exceptions.h>

using namespace newsboat;

TEST_CASE("Parses test config without exceptions", "[configcontainer]")
{
	configcontainer cfg;
	configparser cfgparser;
	cfg.register_commands(cfgparser);
	keymap k(KM_NEWSBOAT);
	cfgparser.register_handler("macro", &k);

	REQUIRE_NOTHROW(cfgparser.parse("data/test-config.txt"));

	SECTION("bool value") {
		REQUIRE(cfg.get_configvalue("show-read-feeds") == "no");
		REQUIRE_FALSE(cfg.get_configvalue_as_bool("show-read-feeds"));
	}

	SECTION("string value") {
		REQUIRE(cfg.get_configvalue("browser") == "firefox");
	}

	SECTION("integer value") {
		REQUIRE(cfg.get_configvalue("max-items") == "100");
		REQUIRE(cfg.get_configvalue_as_int("max-items") == 100);
	}

	SECTION("Tilde got expanded into path to user's home directory") {
		std::string cachefilecomp = ::getenv("HOME");
		cachefilecomp.append("/foo");
		REQUIRE(cfg.get_configvalue("cache-file") == cachefilecomp);
	}
}

TEST_CASE("Parses test config correctly, even if there's no \\n at the end line.",
          "[configcontainer]")
{
	configcontainer cfg;
	configparser cfgparser;
	cfg.register_commands(cfgparser);

	REQUIRE_NOTHROW(cfgparser.parse("data/test-config-without-newline-at-the-end.txt"));

	SECTION("first line") {
		REQUIRE(cfg.get_configvalue("browser") == "firefox");
	}

	SECTION("last line") {
		REQUIRE(cfg.get_configvalue("download-path") == "whatever");
	}
}

TEST_CASE("Throws if invalid command is encountered", "[configcontainer]") {
	configcontainer cfg;

	CHECK_THROWS_AS(
			cfg.handle_action(
				"command-that-surely-does-not-exist",
				{ "and", "its", "arguments" }),
			confighandlerexception);
}

TEST_CASE("Throws if there are no arguments", "[configcontainer]") {
	configcontainer cfg;

	CHECK_THROWS_AS(
			cfg.handle_action("auto-reload", {}),
			confighandlerexception);
}

TEST_CASE("Throws if command argument has invalid type", "[configcontainer]") {
	configcontainer cfg;

	SECTION("bool") {
		CHECK_THROWS_AS(
				cfg.handle_action("always-display-description", { "whatever" }),
				confighandlerexception);
	}

	SECTION("int") {
		CHECK_THROWS_AS(
				cfg.handle_action("download-retries", { "whatever" }),
				confighandlerexception);
	}

	SECTION("enum") {
		CHECK_THROWS_AS(
				cfg.handle_action("proxy-type", { "whatever" }),
				confighandlerexception);
	}
}

TEST_CASE("reset_to_default changes setting to its default value",
		"[configcontainer]")
{
	configcontainer cfg;

	const std::string default_value = "any";
	const std::vector<std::string> tests {
		"any", "basic", "digest", "digest_ie", "gssnegotiate", "ntlm",
		"anysafe" };
	const std::string key("http-auth-method");

	REQUIRE(cfg.get_configvalue(key) == default_value);

	for (const std::string& test_value : tests) {
		cfg.set_configvalue(key, test_value);
		REQUIRE(cfg.get_configvalue(key) == test_value);
		REQUIRE_NOTHROW(cfg.reset_to_default(key));
		REQUIRE(cfg.get_configvalue(key) == default_value);
	}
}

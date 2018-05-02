#include "keymap.h"

#include "3rd-party/catch.hpp"

#include "exceptions.h"

using namespace newsboat;

TEST_CASE("get_operation()", "[keymap]")
{
	keymap k(KM_NEWSBOAT);

	REQUIRE(k.get_operation("u", "article") == OP_SHOWURLS);
	REQUIRE(k.get_operation("X", "feedlist") == OP_NIL);
	REQUIRE(k.get_operation("", "feedlist") == OP_NIL);
	REQUIRE(k.get_operation("ENTER", "feedlist") == OP_OPEN);

	SECTION("Returns OP_NIL after unset_key()")
	{
		k.unset_key("ENTER", "all");
		REQUIRE(k.get_operation("ENTER", "all") == OP_NIL);
	}
}

TEST_CASE("unset_key() and set_key()", "[keymap]")
{
	keymap k(KM_NEWSBOAT);

	REQUIRE(k.get_operation("ENTER", "feedlist") == OP_OPEN);
	REQUIRE(k.getkey(OP_OPEN, "all") == "ENTER");

	SECTION("unset_key() removes the mapping")
	{
		k.unset_key("ENTER", "all");
		REQUIRE(k.get_operation("ENTER", "all") == OP_NIL);

		SECTION("set_key() sets the mapping")
		{
			k.set_key(OP_OPEN, "ENTER", "all");
			REQUIRE(k.get_operation("ENTER", "all") == OP_OPEN);
			REQUIRE(k.getkey(OP_OPEN, "all") == "ENTER");
		}
	}
}

TEST_CASE("get_opcode()", "[keymap]")
{
	keymap k(KM_NEWSBOAT);

	REQUIRE(k.get_opcode("open") == OP_OPEN);
	REQUIRE(k.get_opcode("some-noexistent-operation") == OP_NIL);
}

TEST_CASE("getkey()", "[keymap]")
{
	keymap k(KM_NEWSBOAT);

	SECTION("Retrieves general bindings")
	{
		REQUIRE(k.getkey(OP_OPEN, "all") == "ENTER");
		REQUIRE(k.getkey(OP_TOGGLEITEMREAD, "all") == "N");
		REQUIRE(k.getkey(static_cast<operation>(30000), "all") ==
			"<none>");
	}

	SECTION("Returns context-specific bindings only in that context")
	{
		k.unset_key("q", "article");
		k.set_key(OP_QUIT, "O", "article");
		REQUIRE(k.getkey(OP_QUIT, "article") == "O");
		REQUIRE(k.getkey(OP_QUIT, "all") == "q");
	}
}

TEST_CASE("get_key()", "[keymap]")
{
	keymap k(KM_NEWSBOAT);

	REQUIRE(k.get_key(" ") == ' ');
	REQUIRE(k.get_key("U") == 'U');
	REQUIRE(k.get_key("~") == '~');
	REQUIRE(k.get_key("INVALID") == 0);
	REQUIRE(k.get_key("ENTER") == '\n');
	REQUIRE(k.get_key("ESC") == '\033');
	REQUIRE(k.get_key("^A") == '\001');
}

TEST_CASE("handle_action()", "[keymap]")
{
	keymap k(KM_NEWSBOAT);
	std::vector<std::string> params;

	SECTION("without parameters")
	{
		REQUIRE_THROWS_AS(k.handle_action("bind-key", params),
			confighandlerexception);
		REQUIRE_THROWS_AS(k.handle_action("unbind-key", params),
			confighandlerexception);
		REQUIRE_THROWS_AS(k.handle_action("macro", params),
			confighandlerexception);
	}

	SECTION("with one parameter")
	{
		params.push_back("r");

		REQUIRE_THROWS_AS(k.handle_action("bind-key", params),
			confighandlerexception);
		REQUIRE_NOTHROW(k.handle_action("unbind-key", params));
	}

	SECTION("with two parameters")
	{
		params.push_back("r");
		params.push_back("open");
		REQUIRE_NOTHROW(k.handle_action("bind-key", params));
		REQUIRE_THROWS_AS(k.handle_action("an-invalid-action", params),
			confighandlerexception);
	}
}

#include "keymap.h"

#include "3rd-party/catch.hpp"

#include "confighandlerexception.h"

using namespace newsboat;

TEST_CASE("get_operation()", "[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);

	REQUIRE(k.get_operation("u", "article") == OP_SHOWURLS);
	REQUIRE(k.get_operation("X", "feedlist") == OP_NIL);
	REQUIRE(k.get_operation("", "feedlist") == OP_NIL);
	REQUIRE(k.get_operation("ENTER", "feedlist") == OP_OPEN);

	SECTION("Returns OP_NIL after unset_key()") {
		k.unset_key("ENTER", "all");
		REQUIRE(k.get_operation("ENTER", "all") == OP_NIL);
	}
}

TEST_CASE("unset_key() and set_key()", "[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);

	REQUIRE(k.get_operation("ENTER", "feedlist") == OP_OPEN);
	REQUIRE(k.getkey(OP_OPEN, "all") == "ENTER");

	SECTION("unset_key() removes the mapping") {
		k.unset_key("ENTER", "all");
		REQUIRE(k.get_operation("ENTER", "all") == OP_NIL);

		SECTION("set_key() sets the mapping") {
			k.set_key(OP_OPEN, "ENTER", "all");
			REQUIRE(k.get_operation("ENTER", "all") == OP_OPEN);
			REQUIRE(k.getkey(OP_OPEN, "all") == "ENTER");
		}
	}
}

TEST_CASE(
	"unset_all_keys() clears the keymap from key bindings for a given context",
	"[KeyMap]")
{
	SECTION("KeyMap has most of the keys set up by default") {
		KeyMap k(KM_NEWSBOAT);

		for (int i = OP_QUIT; i < OP_NB_MAX; ++i) {
			if (i == OP_OPENALLUNREADINBROWSER ||
				i == OP_MARKALLABOVEASREAD ||
				i == OP_OPENALLUNREADINBROWSER_AND_MARK ||
				i == OP_SAVEALL) {
				continue;
			}
			REQUIRE(k.getkey(static_cast<Operation>(i), "all") != "<none>");
		}
	}

	SECTION("\"all\" context clears the keymap from all defined keybindings") {
		KeyMap k(KM_NEWSBOAT);
		k.unset_all_keys("all");

		for (int i = OP_NB_MIN; i < OP_SK_MAX; ++i) {
			REQUIRE(k.getkey(static_cast<Operation>(i), "all") == "<none>");
		}
	}

	SECTION("\"all\" context doesn't clear the keymap from internal keybindings") {
		KeyMap default_keymap(KM_NEWSBOAT);
		KeyMap unset_keymap(KM_NEWSBOAT);
		unset_keymap.unset_all_keys("all");

		for (int i = OP_INT_MIN; i < OP_INT_MAX; ++i) {
			REQUIRE(default_keymap.getkey(static_cast<Operation>(i), "all")
				== unset_keymap.getkey(static_cast<Operation>(i), "all"));
		}
	}

	SECTION("Contexts don't have their internal keybindings cleared") {
		const auto contexts = { "feedlist", "filebrowser", "help", "articlelist",
				   "article", "tagselection", "filterselection", "urlview", "podboat",
				   "dialogs", "dirbrowser"
			   };
		KeyMap default_keymap(KM_NEWSBOAT);

		for (const auto& context : contexts) {
			KeyMap unset_keymap(KM_NEWSBOAT);
			unset_keymap.unset_all_keys(context);

			for (int i = OP_INT_MIN; i < OP_INT_MAX; ++i) {
				REQUIRE(default_keymap.getkey(static_cast<Operation>(i), "all")
					== unset_keymap.getkey(static_cast<Operation>(i), "all"));
			}
		}
	}

	SECTION("Clears key bindings just for a given context") {
		KeyMap k(KM_NEWSBOAT);
		k.unset_all_keys("articlelist");

		for (int i = OP_NB_MIN; i < OP_NB_MAX; ++i) {
			REQUIRE(k.getkey(static_cast<Operation>(i), "articlelist") == "<none>");
		}

		KeyMap default_keys(KM_NEWSBOAT);
		for (int i = OP_QUIT; i < OP_NB_MAX; ++i) {
			const auto op = static_cast<Operation>(i);
			REQUIRE(k.getkey(op, "feedlist") == default_keys.getkey(op, "feedlist"));
		}
	}
}

TEST_CASE("get_opcode()", "[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);

	REQUIRE(k.get_opcode("open") == OP_OPEN);
	REQUIRE(k.get_opcode("some-noexistent-operation") == OP_NIL);
}

TEST_CASE("getkey()", "[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);

	SECTION("Retrieves general bindings") {
		REQUIRE(k.getkey(OP_OPEN, "all") == "ENTER");
		REQUIRE(k.getkey(OP_TOGGLEITEMREAD, "all") == "N");
	}

	SECTION("Returns context-specific bindings only in that context") {
		k.unset_key("q", "article");
		k.set_key(OP_QUIT, "O", "article");
		REQUIRE(k.getkey(OP_QUIT, "article") == "O");
		REQUIRE(k.getkey(OP_QUIT, "all") == "q");
	}

	SECTION("Returns context-specific binding if asked to search in all contexts") {
		k.unset_all_keys("all");
		REQUIRE(k.getkey(OP_QUIT, "all") == "<none>");
		k.set_key(OP_QUIT, "O", "article");
		REQUIRE(k.getkey(OP_QUIT, "all") == "O");
	}
}

TEST_CASE("get_key()", "[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);

	REQUIRE(k.get_key(" ") == ' ');
	REQUIRE(k.get_key("U") == 'U');
	REQUIRE(k.get_key("~") == '~');
	REQUIRE(k.get_key("INVALID") == 0);
	REQUIRE(k.get_key("ENTER") == '\n');
	REQUIRE(k.get_key("ESC") == '\033');
	REQUIRE(k.get_key("^A") == '\001');
}

TEST_CASE("handle_action()", "[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);
	std::vector<std::string> params;

	SECTION("without parameters") {
		REQUIRE_THROWS_AS(k.handle_action("bind-key", params),
			ConfigHandlerException);
		REQUIRE_THROWS_AS(k.handle_action("unbind-key", params),
			ConfigHandlerException);
		REQUIRE_THROWS_AS(k.handle_action("macro", params),
			ConfigHandlerException);
	}

	SECTION("with one parameter") {
		params.push_back("r");

		REQUIRE_THROWS_AS(k.handle_action("bind-key", params),
			ConfigHandlerException);
		REQUIRE_NOTHROW(k.handle_action("unbind-key", params));
	}

	SECTION("with two parameters") {
		params.push_back("r");
		params.push_back("open");
		REQUIRE_NOTHROW(k.handle_action("bind-key", params));
		REQUIRE_THROWS_AS(k.handle_action("an-invalid-action", params),
			ConfigHandlerException);
	}

	SECTION("invalid-op throws exception") {
		params.push_back("I");
		params.push_back("invalid-op");

		REQUIRE_THROWS_AS(k.handle_action("bind-key", params),
			ConfigHandlerException);
		REQUIRE_THROWS_AS(k.handle_action("macro", params),
			ConfigHandlerException);
	}
}

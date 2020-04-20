#include "keymap.h"

#include <algorithm>

#include "3rd-party/catch.hpp"

#include "confighandlerexception.h"

using namespace newsboat;

static const auto contexts = { "feedlist", "filebrowser", "help", "articlelist",
	"article", "tagselection", "filterselection", "urlview", "podboat",
	"dialogs", "dirbrowser"
};

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
	REQUIRE(k.get_keys(OP_OPEN, "feedlist") == std::vector<std::string>({"ENTER"}));

	SECTION("unset_key() removes the mapping") {
		k.unset_key("ENTER", "all");
		REQUIRE(k.get_operation("ENTER", "feedlist") == OP_NIL);

		SECTION("set_key() sets the mapping") {
			k.set_key(OP_OPEN, "ENTER", "all");
			REQUIRE(k.get_operation("ENTER", "feedlist") == OP_OPEN);
			REQUIRE(k.get_keys(OP_OPEN, "feedlist") == std::vector<std::string>({"ENTER"}));
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
			bool used_in_some_context = false;
			for (const auto& context : contexts) {
				if (!k.get_keys(static_cast<Operation>(i), context).empty()) {
					used_in_some_context = true;
				}
			}
			REQUIRE(used_in_some_context);
		}
	}

	SECTION("\"all\" context clears the keymap from all defined keybindings") {
		KeyMap k(KM_NEWSBOAT);
		k.unset_all_keys("all");

		for (int i = OP_NB_MIN; i < OP_SK_MAX; ++i) {
			for (const auto& context : contexts) {
				INFO("Operation: " << i);
				INFO("used in context: " << context);
				REQUIRE(k.get_keys(static_cast<Operation>(i),
						context) == std::vector<std::string>());
			}
		}
	}

	SECTION("\"all\" context doesn't clear the keymap from internal keybindings") {
		KeyMap default_keymap(KM_NEWSBOAT);
		KeyMap unset_keymap(KM_NEWSBOAT);
		unset_keymap.unset_all_keys("all");

		for (int i = OP_INT_MIN; i < OP_INT_MAX; ++i) {
			REQUIRE(default_keymap.get_keys(static_cast<Operation>(i), "feedlist")
				== unset_keymap.get_keys(static_cast<Operation>(i), "feedlist"));
		}
	}

	SECTION("Contexts don't have their internal keybindings cleared") {
		KeyMap default_keymap(KM_NEWSBOAT);

		for (const auto& context : contexts) {
			KeyMap unset_keymap(KM_NEWSBOAT);
			unset_keymap.unset_all_keys(context);

			for (int i = OP_INT_MIN; i < OP_INT_MAX; ++i) {
				REQUIRE(default_keymap.get_keys(static_cast<Operation>(i), context)
					== unset_keymap.get_keys(static_cast<Operation>(i), context));
			}
		}
	}

	SECTION("Clears key bindings just for a given context") {
		KeyMap k(KM_NEWSBOAT);
		k.unset_all_keys("articlelist");

		for (int i = OP_NB_MIN; i < OP_NB_MAX; ++i) {
			REQUIRE(k.get_keys(static_cast<Operation>(i),
					"articlelist") == std::vector<std::string>());
		}

		KeyMap default_keys(KM_NEWSBOAT);
		for (int i = OP_QUIT; i < OP_NB_MAX; ++i) {
			const auto op = static_cast<Operation>(i);
			REQUIRE(k.get_keys(op, "feedlist") == default_keys.get_keys(op, "feedlist"));
		}
	}
}

TEST_CASE("get_opcode()", "[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);

	REQUIRE(k.get_opcode("open") == OP_OPEN);
	REQUIRE(k.get_opcode("some-noexistent-operation") == OP_NIL);
}

TEST_CASE("get_keys()", "[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);

	SECTION("Retrieves general bindings") {
		REQUIRE(k.get_keys(OP_OPEN, "feedlist") == std::vector<std::string>({"ENTER"}));
		REQUIRE(k.get_keys(OP_TOGGLEITEMREAD,
				"articlelist") == std::vector<std::string>({"N"}));
	}

	SECTION("Returns context-specific bindings only in that context") {
		k.unset_key("q", "article");
		k.set_key(OP_QUIT, "O", "article");
		REQUIRE(k.get_keys(OP_QUIT, "article") == std::vector<std::string>({"O"}));
		REQUIRE(k.get_keys(OP_QUIT, "feedlist") == std::vector<std::string>({"q"}));
	}

	SECTION("Returns all keys bound to an operation (both default and added)") {
		k.set_key(OP_QUIT, "a", "article");
		k.set_key(OP_QUIT, "d", "article");
		REQUIRE(k.get_keys(OP_QUIT, "article") == std::vector<std::string>({"a", "d", "q"}));
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

	SECTION("allows binding multiple keys to OP_SK_xxx operations") {
		REQUIRE_NOTHROW(k.handle_action("bind-key", {"u", "pageup"}));
		REQUIRE_NOTHROW(k.handle_action("bind-key", {"p", "pageup"}));

		REQUIRE(k.get_keys(OP_SK_PGUP, "feedlist")
			== std::vector<std::string>({"PAGEUP", "p", "u"}));
	}
}

TEST_CASE("test current get_keymap_descriptions() behavior, including its flaws",
	"[KeyMap]")
{
	WHEN("calling get_keymap_descriptions(KM_FEEDLIST)") {
		KeyMap k(KM_NEWSBOAT);
		const auto descriptions = k.get_keymap_descriptions(KM_FEEDLIST);

		THEN("the descriptions do not include any entries with context \"podboat\"") {
			REQUIRE(descriptions.size() > 0);
			for (const auto& description : descriptions) {
				REQUIRE(description.ctx != "podboat");
			}
		}

		THEN("the descriptions include entries with different contexts") {
			REQUIRE(std::any_of(descriptions.begin(), descriptions.end(),
			[](const KeyMapDesc& x) {
				return x.ctx == "feedlist";
			}));

			REQUIRE(std::any_of(descriptions.begin(), descriptions.end(),
			[](const KeyMapDesc& x) {
				return x.ctx == "articlelist";
			}));
		}
	}

	WHEN("calling get_keymap_descriptions(KM_PODBOAT)") {
		KeyMap k(KM_PODBOAT);
		const auto descriptions = k.get_keymap_descriptions(KM_PODBOAT);

		THEN("the descriptions only include entries with context \"podboat\"") {
			REQUIRE(descriptions.size() > 0);
			for (const auto& description : descriptions) {
				REQUIRE(description.ctx == "podboat");
			}
		}
	}

	WHEN("calling get_keymap_descriptions(KM_FEEDLIST)") {
		KeyMap k(KM_NEWSBOAT);
		const auto descriptions = k.get_keymap_descriptions(KM_FEEDLIST);

		THEN("by default it does always set .cmd (command) and .desc (command description)") {
			for (const auto& description : descriptions) {
				REQUIRE(description.cmd != "");
				REQUIRE(description.desc != "");
			}
		}
	}

	GIVEN("that multiple keys are bound to the same operation (\"quit\")") {
		KeyMap k(KM_NEWSBOAT);
		k.set_key(OP_QUIT, "a", "feedlist");
		k.set_key(OP_QUIT, "b", "feedlist");

		WHEN("calling get_keymap_descriptions(KM_FEEDLIST)") {
			const auto descriptions = k.get_keymap_descriptions(KM_FEEDLIST);

			THEN("some entries have no description and command configured") {
				REQUIRE(std::any_of(descriptions.begin(), descriptions.end(),
				[](const KeyMapDesc& x) {
					return x.cmd == "" && x.desc == "";
				}));
			}

			THEN("all entries with non-empty command also have non-empty description") {
				for (const auto& description : descriptions) {
					if (!description.cmd.empty()) {
						REQUIRE(description.desc != "");
					}
				}
			}
		}
	}

	GIVEN("that a key is bound to an operation which by default has no key configured") {
		KeyMap k(KM_NEWSBOAT);
		const std::string key = "O";
		k.set_key(OP_OPENALLUNREADINBROWSER_AND_MARK, key, "feedlist");

		WHEN("calling get_keymap_descriptions(KM_FEEDLIST)") {
			const auto descriptions = k.get_keymap_descriptions(KM_FEEDLIST);

			THEN("there is an entry with the configured key") {
				REQUIRE(std::any_of(descriptions.begin(), descriptions.end(),
				[&key](const KeyMapDesc& x) {
					return x.key == key;
				}));
			}

			THEN("the entry for the configured key has empty command and description fields") {
				for (const auto& description : descriptions) {
					if (description.key == key) {
						REQUIRE(description.cmd == "");
						REQUIRE(description.desc == "");
					}
				}
			}

			THEN("there is an entry with the configured command") {
				REQUIRE(std::any_of(descriptions.begin(), descriptions.end(),
				[&key](const KeyMapDesc& x) {
					return x.cmd == "open-all-unread-in-browser-and-mark-read";
				}));
			}

			THEN("there is an entry for the configured operation with an empty key") {
				for (const auto& description : descriptions) {
					if (description.cmd == "open-all-unread-in-browser-and-mark-read") {
						REQUIRE(description.key == "");
					}
				}
			}
		}
	}
}

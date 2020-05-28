#include "keymap.h"

#include <algorithm>
#include <set>

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
		REQUIRE(k.get_operation("ENTER", "feedlist") == OP_NIL);
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
		REQUIRE_THROWS_AS(k.handle_action("macro", params),
			ConfigHandlerException);
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
			== std::vector<std::string>({"PPAGE", "p", "u"}));
	}
}

TEST_CASE("verify get_keymap_descriptions() behavior",
	"[KeyMap]")
{
	WHEN("calling get_keymap_descriptions(\"feedlist\")") {
		KeyMap k(KM_NEWSBOAT);
		const auto descriptions = k.get_keymap_descriptions("feedlist");

		THEN("the descriptions do not include any entries with context \"podboat\"") {
			REQUIRE(descriptions.size() > 0);
			for (const auto& description : descriptions) {
				REQUIRE(description.ctx != "podboat");
			}
		}

		THEN("the descriptions include only entries with the specified context") {
			REQUIRE(std::all_of(descriptions.begin(), descriptions.end(),
			[](const KeyMapDesc& x) {
				return x.ctx == "feedlist";
			}));
		}
	}

	WHEN("calling get_keymap_descriptions(KM_PODBOAT)") {
		KeyMap k(KM_PODBOAT);
		const auto descriptions = k.get_keymap_descriptions("podboat");

		THEN("the descriptions only include entries with context \"podboat\"") {
			REQUIRE(descriptions.size() > 0);
			for (const auto& description : descriptions) {
				REQUIRE(description.ctx == "podboat");
			}
		}
	}

	WHEN("calling get_keymap_descriptions(\"feedlist\")") {
		KeyMap k(KM_NEWSBOAT);
		const auto descriptions = k.get_keymap_descriptions("feedlist");

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

		WHEN("calling get_keymap_descriptions(\"feedlist\")") {
			const auto descriptions = k.get_keymap_descriptions("feedlist");

			THEN("all entries have both description and command configured") {
				REQUIRE(std::all_of(descriptions.begin(), descriptions.end(),
				[](const KeyMapDesc& x) {
					return x.cmd != "" && x.desc != "";
				}));
			}
		}
	}

	GIVEN("that a key is bound to an operation which by default has no key configured") {
		KeyMap k(KM_NEWSBOAT);
		const std::string key = "O";
		k.set_key(OP_OPENALLUNREADINBROWSER_AND_MARK, key, "feedlist");

		WHEN("calling get_keymap_descriptions(\"feedlist\")") {
			const auto descriptions = k.get_keymap_descriptions("feedlist");

			THEN("there is an entry with the configured key") {
				REQUIRE(std::any_of(descriptions.begin(), descriptions.end(),
				[&key](const KeyMapDesc& x) {
					return x.key == key;
				}));
			}

			THEN("the entry for the configured key has non-empty command and description fields") {
				for (const auto& description : descriptions) {
					if (description.key == key) {
						REQUIRE(description.cmd != "");
						REQUIRE(description.desc != "");
					}
				}
			}

			THEN("all entries for the operation have non-empty key") {
				for (const auto& description : descriptions) {
					if (description.cmd == "open-all-unread-in-browser-and-mark-read") {
						REQUIRE(description.key != "");
					}
				}
			}
		}
	}
}

TEST_CASE("get_keymap_descriptions() returns at most one entry per key",
	"[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);

	const auto descriptions = k.get_keymap_descriptions("feedlist");

	std::set<std::string> keys;
	for (const auto& description : descriptions) {
		if (description.cmd == "set-tag" || description.cmd == "select-tag") {
			// Ignore set-tag/select-tag as these have the same operation-enum value
			continue;
		}

		const std::string& key = description.key;
		INFO("key: \"" << key << "\"");

		if (!key.empty()) {
			REQUIRE(keys.count(key) == 0);
			keys.insert(key);
		}
	}
}

TEST_CASE("get_keymap_descriptions() does not return empty commands or descriptions",
	"[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);

	const std::string operation = "open-all-unread-in-browser-and-mark-read";

	REQUIRE_NOTHROW(k.handle_action("bind-key", {"a", operation}));
	REQUIRE_NOTHROW(k.handle_action("bind-key", {"b", operation}));
	REQUIRE_NOTHROW(k.handle_action("bind-key", {"c", operation}));

	const auto descriptions = k.get_keymap_descriptions("feedlist");
	for (const auto& description : descriptions) {
		REQUIRE(description.cmd != "");
		REQUIRE(description.desc != "");
	}
}

TEST_CASE("get_keymap_descriptions() includes entries which include different keys bound to same operation",
	"[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);

	const std::string operation = "open-all-unread-in-browser-and-mark-read";

	REQUIRE_NOTHROW(k.handle_action("bind-key", {"a", operation}));
	REQUIRE_NOTHROW(k.handle_action("bind-key", {"b", operation}));
	REQUIRE_NOTHROW(k.handle_action("bind-key", {"c", operation}));

	const auto descriptions = k.get_keymap_descriptions("feedlist");

	std::set<std::string> keys;
	for (const auto& description : descriptions) {
		if (description.cmd == operation) {
			CHECK(description.key != "");
			keys.insert(description.key);
		}
	}

	REQUIRE(keys == std::set<std::string>({"a", "b", "c"}));
}

TEST_CASE("dump_config() returns a line for each keybind and macro", "[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);

	std::vector<std::string> dumpOutput;

	GIVEN("that all keybindings are removed") {
		k.unset_all_keys("all");

		WHEN("calling dump_config()") {
			k.dump_config(dumpOutput);

			THEN("the output is empty") {
				REQUIRE(dumpOutput.size() == 0);
			}
		}
	}

	GIVEN("the default keybindings") {

		WHEN("calling dump_config()") {
			k.dump_config(dumpOutput);

			THEN("all lines start with 'bind-key'") {
				for (const auto& line : dumpOutput) {
					INFO("processing: " << line);
					REQUIRE(line.find("bind-key ") == 0);
				}
			}
		}
	}

	GIVEN("a few keybindings") {
		k.unset_all_keys("all");
		k.set_key(OP_OPEN, "ENTER", "feedlist");
		k.set_key(OP_NEXT, "j", "articlelist");
		k.set_key(OP_PREV, "k", "articlelist");

		WHEN("calling dump_config()") {
			k.dump_config(dumpOutput);

			THEN("there is one line per configured binding") {
				REQUIRE(dumpOutput.size() == 3);

				REQUIRE(dumpOutput[0] == R"(bind-key "j" next articlelist)");
				REQUIRE(dumpOutput[1] == R"(bind-key "k" prev articlelist)");
				REQUIRE(dumpOutput[2] == R"(bind-key "ENTER" open feedlist)");
			}
		}
	}

	GIVEN("a few registered macros and one regular keybinding") {
		k.unset_all_keys("all");

		k.set_key(OP_OPEN, "ENTER", "feedlist");
		k.handle_action("macro", {"1", "open"});
		k.handle_action("macro", {"2", "open", ";", "next"});
		k.handle_action("macro", {"3", "open", ";", "next", ";", "prev"});
		k.handle_action("macro", {"4", "open", ";", "next", ";", "prev", ";", "quit"});

		WHEN("calling dump_config()") {
			k.dump_config(dumpOutput);

			THEN("there is one line per configured macro") {
				REQUIRE(dumpOutput.size() == 5);

				REQUIRE(dumpOutput[0] == R"(bind-key "ENTER" open feedlist)");
				REQUIRE(dumpOutput[1] == R"(macro 1 open)");
				REQUIRE(dumpOutput[2] == R"(macro 2 open ; next ; )");
				REQUIRE(dumpOutput[3] == R"(macro 3 open ; next ; prev ; )");
				REQUIRE(dumpOutput[4] == R"(macro 4 open ; next ; prev ; quit ; )");
			}
		}
	}

	GIVEN("a few registered macros with arguments") {
		k.unset_all_keys("all");

		k.handle_action("macro", {"1", "set", "arg 1"});
		k.handle_action("macro", {"2", "set", "arg 1", ";", "set", "arg 2", "arg 3"});

		WHEN("calling dump_config()") {
			k.dump_config(dumpOutput);

			THEN("there is one line per configured macro ; all arguments are included") {
				REQUIRE(dumpOutput.size() == 2);

				REQUIRE(dumpOutput[0] == R"(macro 1 set "arg 1")");
				REQUIRE(dumpOutput[1] == R"(macro 2 set "arg 1" ; set "arg 2" "arg 3" ; )");
			}
		}
	}
}

#include "keymap.h"

#include <algorithm>
#include <set>

#include "3rd-party/catch.hpp"

#include "confighandlerexception.h"
#include "keycombination.h"

using namespace newsboat;

static const auto contexts = { "feedlist", "filebrowser", "help", "articlelist",
	"article", "tagselection", "filterselection", "urlview", "podboat",
	"dialogs", "dirbrowser"
};

TEST_CASE("get_operation()", "[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);

	REQUIRE(k.get_operation(KeyCombination("u"), "article") == OP_SHOWURLS);
	REQUIRE(k.get_operation(KeyCombination("x", ShiftState::Shift), "feedlist") == OP_NIL);
	REQUIRE(k.get_operation(KeyCombination(""), "feedlist") == OP_NIL);
	REQUIRE(k.get_operation(KeyCombination("ENTER"), "feedlist") == OP_OPEN);

	SECTION("Returns OP_NIL after unset_key()") {
		k.unset_key(KeyCombination("ENTER"), "all");
		REQUIRE(k.get_operation(KeyCombination("ENTER"), "feedlist") == OP_NIL);
	}
}

TEST_CASE("unset_key() and set_key()", "[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);

	REQUIRE(k.get_operation(KeyCombination("ENTER"), "feedlist") == OP_OPEN);
	REQUIRE(k.get_keys(OP_OPEN, "feedlist") == std::vector<KeyCombination>({KeyCombination("ENTER")}));

	SECTION("unset_key() removes the mapping") {
		k.unset_key(KeyCombination("ENTER"), "all");
		REQUIRE(k.get_operation(KeyCombination("ENTER"), "feedlist") == OP_NIL);

		SECTION("set_key() sets the mapping") {
			k.set_key(OP_OPEN, KeyCombination("ENTER"), "all");
			REQUIRE(k.get_operation(KeyCombination("ENTER"), "feedlist") == OP_OPEN);
			REQUIRE(k.get_keys(OP_OPEN, "feedlist") == std::vector<KeyCombination>({KeyCombination("ENTER")}));
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
				i == OP_SAVEALL ||
				i == OP_GOTO_TITLE ||
				i == OP_OPENINBROWSER_NONINTERACTIVE) {
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
						context) == std::vector<KeyCombination>());
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
					"articlelist") == std::vector<KeyCombination>());
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
		REQUIRE(k.get_keys(OP_OPEN, "feedlist") == std::vector<KeyCombination>({KeyCombination("ENTER")}));
		REQUIRE(k.get_keys(OP_TOGGLEITEMREAD,
				"articlelist") == std::vector<KeyCombination>({KeyCombination("n", ShiftState::Shift)}));
	}

	SECTION("Returns context-specific bindings only in that context") {
		k.unset_key(KeyCombination("q"), "article");
		k.set_key(OP_QUIT, KeyCombination("o", ShiftState::Shift), "article");
		REQUIRE(k.get_keys(OP_QUIT, "article") == std::vector<KeyCombination>({KeyCombination("o", ShiftState::Shift)}));
		REQUIRE(k.get_keys(OP_QUIT, "feedlist") == std::vector<KeyCombination>({KeyCombination("q")}));
	}

	SECTION("Returns all keys bound to an operation (both default and added)") {
		k.set_key(OP_QUIT, KeyCombination("a"), "article");
		k.set_key(OP_QUIT, KeyCombination("d"), "article");
		REQUIRE(k.get_keys(OP_QUIT, "article") == std::vector<KeyCombination>({KeyCombination("a"), KeyCombination("d"), KeyCombination("q")}));
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

	SECTION("without parameters") {
		REQUIRE_THROWS_AS(k.handle_action("bind-key", ""),
			ConfigHandlerException);
		REQUIRE_THROWS_AS(k.handle_action("unbind-key", ""),
			ConfigHandlerException);
		REQUIRE_THROWS_AS(k.handle_action("macro", ""),
			ConfigHandlerException);
	}

	SECTION("with one parameter") {
		REQUIRE_THROWS_AS(k.handle_action("bind-key", "r"),
			ConfigHandlerException);
		REQUIRE_NOTHROW(k.handle_action("unbind-key", "r"));
		REQUIRE_THROWS_AS(k.handle_action("macro", "r"),
			ConfigHandlerException);
	}

	SECTION("with two parameters") {
		REQUIRE_NOTHROW(k.handle_action("bind-key", "r open"));
		REQUIRE_THROWS_AS(k.handle_action("an-invalid-action", "r open"),
			ConfigHandlerException);
	}

	SECTION("invalid-op throws exception") {
		REQUIRE_THROWS_AS(k.handle_action("bind-key", "I invalid-op"),
			ConfigHandlerException);
		REQUIRE_THROWS_AS(k.handle_action("macro", "I invalid-op"),
			ConfigHandlerException);
	}

	SECTION("allows binding multiple keys to OP_SK_xxx operations") {
		REQUIRE_NOTHROW(k.handle_action("bind-key", "u pageup"));
		REQUIRE_NOTHROW(k.handle_action("bind-key", "p pageup"));

		REQUIRE(k.get_keys(OP_SK_PGUP, "feedlist")
			== std::vector<KeyCombination>({KeyCombination("PPAGE"), KeyCombination("p"), KeyCombination("u")}));
	}

	SECTION("macro without commands results in exception") {
		REQUIRE_THROWS_AS(k.handle_action("macro", "r ; ; ; ;"),
			ConfigHandlerException);
	}
}

TEST_CASE("handle_action() for bind", "[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);

	SECTION("supports optional description") {
		REQUIRE_NOTHROW(k.handle_action("bind", R"(a everywhere open -- "a description")"));
		REQUIRE_NOTHROW(k.handle_action("bind", R"(a everywhere open)"));
	}

	SECTION("throws if incomplete") {
		REQUIRE_NOTHROW(k.handle_action("bind", "a everywhere open"));
		REQUIRE_THROWS_AS(k.handle_action("bind", "a everywhere open --"), ConfigHandlerException);
		REQUIRE_THROWS_AS(k.handle_action("bind", "a everywhere"), ConfigHandlerException);
		REQUIRE_THROWS_AS(k.handle_action("bind", "a"), ConfigHandlerException);
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
		k.set_key(OP_QUIT, KeyCombination("a"), "feedlist");
		k.set_key(OP_QUIT, KeyCombination("b"), "feedlist");

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
		const auto key = KeyCombination("o", ShiftState::Shift);
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
						REQUIRE(description.key.get_key() != "");
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

	std::set<KeyCombination> keys;
	for (const auto& description : descriptions) {
		if (description.cmd == "set-tag" || description.cmd == "select-tag") {
			// Ignore set-tag/select-tag as these have the same operation-enum value
			continue;
		}

		const auto& key = description.key;
		INFO("key: \"" << key.to_bindkey_string() << "\"");

		if (!key.get_key().empty()) {
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

	REQUIRE_NOTHROW(k.handle_action("bind-key", "a " +  operation));
	REQUIRE_NOTHROW(k.handle_action("bind-key", "b " +  operation));
	REQUIRE_NOTHROW(k.handle_action("bind-key", "c " +  operation));

	const auto descriptions = k.get_keymap_descriptions("feedlist");
	for (const auto& description : descriptions) {
		REQUIRE(description.cmd != "");
		REQUIRE(description.desc != "");
	}
}

TEST_CASE("get_keymap_descriptions() includes entries which include different "
	"keys bound to same operation",
	"[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);

	const std::string operation = "open-all-unread-in-browser-and-mark-read";

	REQUIRE_NOTHROW(k.handle_action("bind-key", "a " +  operation));
	REQUIRE_NOTHROW(k.handle_action("bind-key", "b " +  operation));
	REQUIRE_NOTHROW(k.handle_action("bind-key", "c " +  operation));

	const auto descriptions = k.get_keymap_descriptions("feedlist");

	std::set<KeyCombination> keys;
	for (const auto& description : descriptions) {
		if (description.cmd == operation) {
			CHECK(description.key.get_key() != "");
			keys.insert(description.key);
		}
	}

	REQUIRE(keys == std::set<KeyCombination>({KeyCombination("a"), KeyCombination("b"), KeyCombination("c")}));
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
		k.set_key(OP_OPEN, KeyCombination("ENTER"), "feedlist");
		k.set_key(OP_NEXT, KeyCombination("j"), "articlelist");
		k.set_key(OP_PREV, KeyCombination("k"), "articlelist");

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

		k.set_key(OP_OPEN, KeyCombination("ENTER"), "feedlist");
		k.handle_action("macro", "1 open");
		k.handle_action("macro", "2 open ; next");
		k.handle_action("macro", "3 open ; next ; prev");
		k.handle_action("macro", "4 open ; next ; prev ; quit");

		WHEN("calling dump_config()") {
			k.dump_config(dumpOutput);

			THEN("there is one line per configured macro") {
				REQUIRE(dumpOutput.size() == 5);

				REQUIRE(dumpOutput[0] == R"(bind-key "ENTER" open feedlist)");
				REQUIRE(dumpOutput[1] == R"(macro 1 open)");
				REQUIRE(dumpOutput[2] == R"(macro 2 open ; next)");
				REQUIRE(dumpOutput[3] == R"(macro 3 open ; next ; prev)");
				REQUIRE(dumpOutput[4] == R"(macro 4 open ; next ; prev ; quit)");
			}
		}
	}

	GIVEN("a few registered macros with arguments") {
		k.unset_all_keys("all");

		k.handle_action("macro", "1 set \"arg 1\"");
		k.handle_action("macro", "2 set \"arg 1\" ; set \"arg 2\" \"arg 3\"");
		k.handle_action("macro", "x set a \"arg 1\"");
		k.handle_action("macro", "y set m n");
		k.handle_action("macro", "z set var I");

		WHEN("calling dump_config()") {
			k.dump_config(dumpOutput);

			THEN("there is one line per configured macro ; all arguments are included") {
				REQUIRE(dumpOutput.size() == 5);

				REQUIRE(dumpOutput[0] == R"(macro 1 set "arg 1")");
				REQUIRE(dumpOutput[1] == R"(macro 2 set "arg 1" ; set "arg 2" "arg 3")");
				REQUIRE(dumpOutput[2] == R"(macro x set "a" "arg 1")");
				REQUIRE(dumpOutput[3] == R"(macro y set "m" "n")");
				REQUIRE(dumpOutput[4] == R"(macro z set "var" "I")");
			}
		}
	}
}

TEST_CASE("dump_config() stores a description if it is present", "[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);

	GIVEN("a few macros, some with a description") {
		k.unset_all_keys("all");

		k.handle_action("macro", R"(x open -- "first description")");
		k.handle_action("macro", R"(y set m n -- "configure \"m\" \\ ")");
		k.handle_action("macro", R"(z set var I)");

		WHEN("calling dump_config()") {
			std::vector<std::string> dumpOutput;
			k.dump_config(dumpOutput);

			THEN("there is one line per configured macro ; all given descriptions are included") {
				REQUIRE(dumpOutput.size() == 3);

				REQUIRE(dumpOutput[0] == R"(macro x open -- "first description")");
				REQUIRE(dumpOutput[1] == R"(macro y set "m" "n" -- "configure \"m\" \\ ")");
				REQUIRE(dumpOutput[2] == R"(macro z set "var" "I")");
			}
		}
	}
}

TEST_CASE("Regression test for https://github.com/newsboat/newsboat/issues/702",
	"[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);

	SECTION("semicolon following quoted argument") {
		k.handle_action("macro", R"(a set browser "firefox"; open-in-browser)");

		const auto macros = k.get_macro(KeyCombination("a"));
		REQUIRE(macros.size() == 2);
		REQUIRE(macros[0].op == OP_INT_SET);
		REQUIRE(macros[0].args == std::vector<std::string>({"browser", "firefox"}));
		REQUIRE(macros[1].op == OP_OPENINBROWSER);
		REQUIRE(macros[1].args == std::vector<std::string>());
	}

	SECTION("semicolon following unquoted argument") {
		k.handle_action("macro", R"(b set browser firefox; open-in-browser)");

		const auto macros = k.get_macro(KeyCombination("b"));
		REQUIRE(macros.size() == 2);
		REQUIRE(macros[0].op == OP_INT_SET);
		REQUIRE(macros[0].args == std::vector<std::string>({"browser", "firefox"}));
		REQUIRE(macros[1].op == OP_OPENINBROWSER);
		REQUIRE(macros[1].args == std::vector<std::string>());
	}

	SECTION("semicolon following unquoted operation") {
		k.handle_action("macro", R"(c open-in-browser; quit)");

		const auto macros = k.get_macro(KeyCombination("c"));
		REQUIRE(macros.size() == 2);
		REQUIRE(macros[0].op == OP_OPENINBROWSER);
		REQUIRE(macros[0].args == std::vector<std::string>());
		REQUIRE(macros[1].op == OP_QUIT);
		REQUIRE(macros[1].args == std::vector<std::string>());
	}
}

TEST_CASE("Whitespace around semicolons in macros is optional", "[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);

	const auto check = [&k]() {
		const auto macro = k.get_macro(KeyCombination("x"));

		REQUIRE(macro.size() == 3);

		REQUIRE(macro[0].op == OP_OPEN);
		REQUIRE(macro[0].args == std::vector<std::string>());

		REQUIRE(macro[1].op == OP_INT_SET);
		REQUIRE(macro[1].args == std::vector<std::string>({"browser", "firefox --private-window"}));

		REQUIRE(macro[2].op == OP_QUIT);
		REQUIRE(macro[2].args == std::vector<std::string>());
	};

	SECTION("Whitespace not required before the semicolon") {
		k.handle_action("macro",
				R"(x open; set browser "firefox --private-window"; quit)");
		check();
	}

	SECTION("Whitespace not required after the semicolon") {
		k.handle_action("macro",
				R"(x open ;set browser "firefox --private-window" ;quit)");
		check();
	}

	SECTION("Whitespace not required on either side of the semicolon") {
		k.handle_action("macro",
				R"(x open;set browser "firefox --private-window";quit)");
		check();
	}
}

TEST_CASE("It's not an error to have no operations before a semicolon in "
	"a macro",
	"[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);

	const std::vector<std::string> op_lists = {
		"; ;; ; open",
		";;; ;; ; open",
		";;; ;; ; open ;",
		";;; ;; ; open ;; ;",
		";;; ;; ; open ; ;;;;",
		";;; open ; ;;;;",
		"; open ;; ;; ;",
		"open ; ;;; ;;",
	};

	for (const auto& op_list : op_lists) {
		DYNAMIC_SECTION(op_list) {
			k.handle_action("macro", "r " + op_list);

			const auto macro = k.get_macro(KeyCombination("r"));
			REQUIRE(macro.size() == 1);
			REQUIRE(macro[0].op == OP_OPEN);
			REQUIRE(macro[0].args == std::vector<std::string>());
		}
	}
}

TEST_CASE("Semicolons in operation's arguments don't break parsing of a macro",
	"[KeyMap]")
{
	// This is a regression test for https://github.com/newsboat/newsboat/issues/1200

	KeyMap k(KM_NEWSBOAT);

	k.handle_action("macro",
			R"(x set browser "sleep 3; do-something ; echo hi"; open-in-browser)");

	const auto macro = k.get_macro(KeyCombination("x"));
	REQUIRE(macro.size() == 2);
	REQUIRE(macro[0].op == OP_INT_SET);
	REQUIRE(macro[0].args == std::vector<std::string>({"browser", "sleep 3; do-something ; echo hi"}));
	REQUIRE(macro[1].op == OP_OPENINBROWSER);
	REQUIRE(macro[1].args == std::vector<std::string>());
}

TEST_CASE("prepare_keymap_hint() returns a string describing keys to which given operations are bound",
	"[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);

	k.handle_action("bind-key", "w help");
	k.handle_action("bind-key", "x open");
	k.handle_action("bind-key", "< open");
	k.handle_action("unbind-key", "r");
	k.handle_action("bind-key", "O reload");
	// This frees up OP_SEARCH
	k.handle_action("unbind-key", "/");

	const std::vector<KeyMapHintEntry> hints {
		{OP_QUIT, "Get out of <this> dialog"},
		{OP_HELP, "HALP"},
		{OP_OPEN, "Open"},
		{OP_RELOAD, "Reload current entry"},
		{OP_SEARCH, "Go find me"}
	};

	REQUIRE(k.prepare_keymap_hint(hints, "feedlist") ==
		"<key>q</><colon>:</><desc>Get out of <>this> dialog</> "
		"<key>?</><comma>,</><key>w</><colon>:</><desc>HALP</> "
		"<key><></><comma>,</><key>ENTER</><comma>,</><key>x</><colon>:</><desc>Open</> "
		"<key>O</><colon>:</><desc>Reload current entry</> "
		"<key><>none></><colon>:</><desc>Go find me</> ");
}

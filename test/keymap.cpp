#include "keymap.h"

#include <algorithm>
#include <set>

#include "3rd-party/catch.hpp"

#include "config.h"
#include "confighandlerexception.h"
#include "dialog.h"
#include "keycombination.h"

using namespace newsboat;

static const auto contexts = { Dialog::FeedList, Dialog::FileBrowser, Dialog::Help, Dialog::ArticleList,
	       Dialog::Article, Dialog::TagSelection, Dialog::FilterSelection, Dialog::UrlView, Dialog::Podboat,
	       Dialog::DialogList, Dialog::DirBrowser
};

namespace {
Operation check_single_command_binding(KeyMap& keymap,
	const KeyCombination& key_combination, Dialog context)
{
	MultiKeyBindingState binding_state{};
	BindingType binding_type{};
	const auto& cmds = keymap.get_operation({key_combination}, context, binding_state,
			binding_type);
	REQUIRE(binding_state == MultiKeyBindingState::Found);
	REQUIRE(cmds.size() == 1);
	return cmds.at(0).op;
}

void check_unbound(KeyMap& keymap, const KeyCombination& key_combination, Dialog context)
{
	MultiKeyBindingState binding_state{};
	BindingType binding_type{};
	keymap.get_operation({key_combination}, context, binding_state,
		binding_type);
	REQUIRE(binding_state == MultiKeyBindingState::NotFound);
}
}

TEST_CASE("get_operation()", "[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);

	REQUIRE(check_single_command_binding(k, KeyCombination("u"),
			Dialog::Article) == OP_SHOWURLS);
	check_unbound(k, KeyCombination("x", ShiftState::Shift), Dialog::FeedList);
	check_unbound(k, KeyCombination(""), Dialog::FeedList);
	REQUIRE(check_single_command_binding(k, KeyCombination("ENTER"),
			Dialog::FeedList) == OP_OPEN);

	SECTION("Returns OP_NIL after unset_key()") {
		k.unset_key(KeyCombination("ENTER"), AllDialogs());
		check_unbound(k, KeyCombination("ENTER"), Dialog::FeedList);
	}

	GIVEN("A multi-key binding specifying 'a' followed by ENTER") {
		k.handle_action("bind", "a<ENTER> feedlist open");

		const Dialog context = Dialog::FeedList;
		MultiKeyBindingState binding_state{};
		BindingType type{};
		WHEN("only 'a' is provided") {
			const std::vector<KeyCombination> key_sequence = { KeyCombination("a") };
			k.get_operation(key_sequence, context, binding_state, type);

			THEN("more input is required") {
				REQUIRE(binding_state == MultiKeyBindingState::MoreInputNeeded);
			}
		}
		WHEN("'a' followed by ENTER is provided") {
			const std::vector<KeyCombination> key_sequence = { KeyCombination("a"), KeyCombination("ENTER") };
			const auto cmds = k.get_operation(key_sequence, context, binding_state, type);

			THEN("a binding is found") {
				REQUIRE(binding_state == MultiKeyBindingState::Found);
				REQUIRE(type == BindingType::Bind);
				REQUIRE(cmds.size() == 1);
				REQUIRE(cmds[0].op == OP_OPEN);
			}
		}

		WHEN("'a' followed by a different key is provided") {
			const std::vector<KeyCombination> key_sequence = { KeyCombination("a"), KeyCombination("b") };
			k.get_operation(key_sequence, context, binding_state, type);

			THEN("no binding is found") {
				REQUIRE(binding_state == MultiKeyBindingState::NotFound);
			}
		}
	}
}

TEST_CASE("unset_key() and set_key()", "[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);

	REQUIRE(check_single_command_binding(k, KeyCombination("ENTER"),
			Dialog::FeedList) == OP_OPEN);
	REQUIRE(k.get_keys(OP_OPEN, Dialog::FeedList) == std::vector<KeyCombination>({KeyCombination("ENTER")}));

	SECTION("unset_key() removes the mapping") {
		k.unset_key(KeyCombination("ENTER"), AllDialogs());
		check_unbound(k, KeyCombination("ENTER"), Dialog::FeedList);

		SECTION("set_key() sets the mapping") {
			k.set_key(OP_OPEN, KeyCombination("ENTER"), AllDialogs());
			REQUIRE(check_single_command_binding(k, KeyCombination("ENTER"),
					Dialog::FeedList) == OP_OPEN);
			REQUIRE(k.get_keys(OP_OPEN, Dialog::FeedList) == std::vector<KeyCombination>({KeyCombination("ENTER")}));
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
		k.unset_all_keys(AllDialogs());

		for (int i = OP_NB_MIN; i < OP_SK_MAX; ++i) {
			for (const auto& context : contexts) {
				INFO("Operation: " << i);
				INFO("used in context: " << dialog_name(context));
				REQUIRE(k.get_keys(static_cast<Operation>(i),
						context) == std::vector<KeyCombination>());
			}
		}
	}

	SECTION("Clears key bindings just for a given context") {
		KeyMap k(KM_NEWSBOAT);
		k.unset_all_keys(Dialog::ArticleList);

		for (int i = OP_NB_MIN; i < OP_NB_MAX; ++i) {
			REQUIRE(k.get_keys(static_cast<Operation>(i),
					Dialog::ArticleList) == std::vector<KeyCombination>());
		}

		KeyMap default_keys(KM_NEWSBOAT);
		for (int i = OP_QUIT; i < OP_NB_MAX; ++i) {
			const auto op = static_cast<Operation>(i);
			REQUIRE(k.get_keys(op, Dialog::FeedList) == default_keys.get_keys(op, Dialog::FeedList));
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
		REQUIRE(k.get_keys(OP_OPEN, Dialog::FeedList) == std::vector<KeyCombination>({KeyCombination("ENTER")}));
		REQUIRE(k.get_keys(OP_TOGGLEITEMREAD,
				Dialog::ArticleList) == std::vector<KeyCombination>({KeyCombination("n", ShiftState::Shift)}));
	}

	SECTION("Returns context-specific bindings only in that context") {
		k.unset_key(KeyCombination("q"), Dialog::Article);
		k.set_key(OP_QUIT, KeyCombination("o", ShiftState::Shift), Dialog::Article);
		REQUIRE(k.get_keys(OP_QUIT, Dialog::Article) == std::vector<KeyCombination>({KeyCombination("o", ShiftState::Shift)}));
		REQUIRE(k.get_keys(OP_QUIT, Dialog::FeedList) == std::vector<KeyCombination>({KeyCombination("q")}));
	}

	SECTION("Returns all keys bound to an operation (both default and added)") {
		k.set_key(OP_QUIT, KeyCombination("a"), Dialog::Article);
		k.set_key(OP_QUIT, KeyCombination("d"), Dialog::Article);
		REQUIRE(k.get_keys(OP_QUIT, Dialog::Article) == std::vector<KeyCombination>({KeyCombination("a"), KeyCombination("d"), KeyCombination("q")}));
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

		REQUIRE(k.get_keys(OP_SK_PGUP, Dialog::FeedList)
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

	SECTION("throws on invalid dialog name") {
		REQUIRE_THROWS_AS(k.handle_action("bind", R"(a feedlist,notavalidname,itemlist open)"),
			ConfigHandlerException);
	}
}

TEST_CASE("handle_action() for unbind", "[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);

	GIVEN("a few bindings for all contexts") {
		k.handle_action("bind", "a everywhere quit");
		k.handle_action("bind", "go everywhere open");
		k.handle_action("bind", "gd everywhere open");

		WHEN("single key is unbound in a context") {
			k.handle_action("unbind", "a feedlist");

			THEN("binding still exists in other contexts") {
				check_unbound(k, KeyCombination("a"), "feedlist");
				check_single_command_binding(k, KeyCombination("a"), "articlelist");
			}
		}

		WHEN("multi-key sequence is unbound") {
			k.handle_action("unbind", "go everywhere");

			THEN("other bindings starting with the same key are left intact") {
				MultiKeyBindingState binding_state{};
				BindingType binding_type{};

				k.get_operation({KeyCombination("g"), KeyCombination("o")}, "feedlist", binding_state,
					binding_type);
				REQUIRE(binding_state == MultiKeyBindingState::NotFound);

				k.get_operation({KeyCombination("g"), KeyCombination("d")}, "feedlist", binding_state,
					binding_type);
				REQUIRE(binding_state == MultiKeyBindingState::Found);
			}
		}

		WHEN("prefix of multi-key sequence is unbound") {
			k.handle_action("unbind", "g everywhere");

			THEN("bindings starting with the prefix are removed") {
				MultiKeyBindingState binding_state{};
				BindingType binding_type{};

				k.get_operation({KeyCombination("g"), KeyCombination("o")}, "feedlist", binding_state,
					binding_type);
				REQUIRE(binding_state == MultiKeyBindingState::NotFound);

				k.get_operation({KeyCombination("g"), KeyCombination("d")}, "feedlist", binding_state,
					binding_type);
				REQUIRE(binding_state == MultiKeyBindingState::NotFound);
			}
		}
	}

	SECTION("unbinding non-existing binding is allowed (no-op)") {
		REQUIRE_NOTHROW(k.handle_action("unbind", "abc everywhere"));
	}

	SECTION("unbind supports same key-sequence syntax as bind") {
		k.handle_action("bind", "^K<S-a><ENTER> everywhere quit");

		MultiKeyBindingState binding_state{};
		BindingType binding_type{};

		k.get_operation({
			KeyCombination("k", ShiftState::NoShift, ControlState::Control),
			KeyCombination("a", ShiftState::Shift),
			KeyCombination("ENTER"),
		}, "feedlist", binding_state, binding_type);
		REQUIRE(binding_state == MultiKeyBindingState::Found);

		k.handle_action("unbind", "^K<S-a><ENTER> feedlist");

		k.get_operation({
			KeyCombination("k", ShiftState::NoShift, ControlState::Control),
			KeyCombination("a", ShiftState::Shift),
			KeyCombination("ENTER"),
		}, "feedlist", binding_state, binding_type);
		REQUIRE(binding_state == MultiKeyBindingState::NotFound);
	}

	SECTION("throws if incomplete") {
		REQUIRE_THROWS_AS(k.handle_action("unbind", "a"), ConfigHandlerException);
		REQUIRE_THROWS_AS(k.handle_action("unbind", ""), ConfigHandlerException);
	}
}

TEST_CASE("handle_action() for unbind-key", "[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);

	GIVEN("A multi-key binding specifying 'a' followed by ENTER") {
		k.handle_action("bind", "a<ENTER> feedlist open");

		const Dialog context = Dialog::FeedList;
		MultiKeyBindingState binding_state{};
		BindingType type{};

		WHEN("'a' key is unbound") {
			k.handle_action("unbind-key", "a");

			THEN("no binding is found") {
				const std::vector<KeyCombination> key_sequence = { KeyCombination("a"), KeyCombination("ENTER") };
				k.get_operation(key_sequence, context, binding_state, type);

				REQUIRE(binding_state == MultiKeyBindingState::NotFound);
			}
		}

		WHEN("a different key is unbound") {
			k.handle_action("unbind-key", "b");

			THEN("the binding is found") {
				const std::vector<KeyCombination> key_sequence = { KeyCombination("a"), KeyCombination("ENTER") };
				k.get_operation(key_sequence, context, binding_state, type);

				REQUIRE(binding_state == MultiKeyBindingState::Found);
			}
		}
	}

	SECTION("unbind-key uses 'old style key binding' special key syntax") {
		REQUIRE(check_single_command_binding(k, KeyCombination("ENTER"),
				Dialog::FeedList) == OP_OPEN);

		// New style `bind` would specify this as `<ENTER>` instead
		k.handle_action("unbind-key", "ENTER");

		check_unbound(k, KeyCombination("ENTER"), Dialog::FeedList);
	}
}

TEST_CASE("verify get_keymap_descriptions() behavior",
	"[KeyMap]")
{
	WHEN("calling get_keymap_descriptions(\"feedlist\")") {
		KeyMap k(KM_NEWSBOAT);
		const auto descriptions = k.get_keymap_descriptions(Dialog::FeedList);

		THEN("the descriptions do not include any entries with context \"podboat\"") {
			REQUIRE(descriptions.size() > 0);
			for (const auto& description : descriptions) {
				REQUIRE(description.ctx != Dialog::Podboat);
			}
		}

		THEN("the descriptions include only entries with the specified context") {
			REQUIRE(std::all_of(descriptions.begin(), descriptions.end(),
			[](const KeyMapDesc& x) {
				return x.ctx == Dialog::FeedList;
			}));
		}
	}

	WHEN("calling get_keymap_descriptions(KM_PODBOAT)") {
		KeyMap k(KM_PODBOAT);
		const auto descriptions = k.get_keymap_descriptions(Dialog::Podboat);

		THEN("the descriptions only include entries with context \"podboat\"") {
			REQUIRE(descriptions.size() > 0);
			for (const auto& description : descriptions) {
				REQUIRE(description.ctx == Dialog::Podboat);
			}
		}
	}

	WHEN("calling get_keymap_descriptions(\"feedlist\")") {
		KeyMap k(KM_NEWSBOAT);
		const auto descriptions = k.get_keymap_descriptions(Dialog::FeedList);

		THEN("by default it does always set .cmd (command) and .desc (command description)") {
			for (const auto& description : descriptions) {
				REQUIRE(description.cmd != "");
				REQUIRE(description.desc != "");
			}
		}
	}

	GIVEN("that multiple keys are bound to the same operation (\"quit\")") {
		KeyMap k(KM_NEWSBOAT);
		k.set_key(OP_QUIT, KeyCombination("a"), Dialog::FeedList);
		k.set_key(OP_QUIT, KeyCombination("b"), Dialog::FeedList);

		WHEN("calling get_keymap_descriptions(\"feedlist\")") {
			const auto descriptions = k.get_keymap_descriptions(Dialog::FeedList);

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
		k.set_key(OP_OPENALLUNREADINBROWSER_AND_MARK, key, Dialog::FeedList);

		WHEN("calling get_keymap_descriptions(\"feedlist\")") {
			const auto descriptions = k.get_keymap_descriptions(Dialog::FeedList);

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

	const auto descriptions = k.get_keymap_descriptions(Dialog::FeedList);

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

	const auto descriptions = k.get_keymap_descriptions(Dialog::FeedList);
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

	const auto descriptions = k.get_keymap_descriptions(Dialog::FeedList);

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
		k.unset_all_keys(AllDialogs());

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
		k.unset_all_keys(AllDialogs());
		k.set_key(OP_OPEN, KeyCombination("ENTER"), Dialog::FeedList);
		k.set_key(OP_NEXT, KeyCombination("j"), Dialog::ArticleList);
		k.set_key(OP_PREV, KeyCombination("k"), Dialog::ArticleList);

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
		k.unset_all_keys(AllDialogs());

		k.set_key(OP_OPEN, KeyCombination("ENTER"), Dialog::FeedList);
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
		k.unset_all_keys(AllDialogs());

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
		k.unset_all_keys(AllDialogs());

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
		REQUIRE(macros[0].op == OP_SET);
		REQUIRE(macros[0].args == std::vector<std::string>({"browser", "firefox"}));
		REQUIRE(macros[1].op == OP_OPENINBROWSER);
		REQUIRE(macros[1].args == std::vector<std::string>());
	}

	SECTION("semicolon following unquoted argument") {
		k.handle_action("macro", R"(b set browser firefox; open-in-browser)");

		const auto macros = k.get_macro(KeyCombination("b"));
		REQUIRE(macros.size() == 2);
		REQUIRE(macros[0].op == OP_SET);
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

		REQUIRE(macro[1].op == OP_SET);
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
	REQUIRE(macro[0].op == OP_SET);
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

	REQUIRE(k.prepare_keymap_hint(hints, Dialog::FeedList).stfl_quoted() ==
		"<key>q<colon>:<desc>Get out of <>this> dialog</> "
		"<key>?<comma>,<key>w<colon>:<desc>HALP</> "
		"<key>ENTER<comma>,<key><><comma>,<key>x<colon>:<desc>Open</> "
		"<key>O<colon>:<desc>Reload current entry</> "
		"<key><>none><colon>:<desc>Go find me</>");
}

TEST_CASE("get_help_info() returns info about macros, bindings, and unbound actions",
	"[KeyMap]")
{
	KeyMap k(KM_NEWSBOAT);


	GIVEN("all bindings are removed") {
		k.unset_all_keys(Dialog::FeedList);

		WHEN("help info is retrieved") {
			const auto help_info = k.get_help_info(Dialog::FeedList);

			THEN("the list with info about bindings is empty") {
				REQUIRE(help_info.bindings.size() == 0);
			}

			THEN("unused actions list contains the 'open' action with a description") {
				REQUIRE_FALSE(help_info.unused.empty());
				const auto open_it = std::find_if(help_info.unused.begin(),
				help_info.unused.end(), [](const UnboundAction& u) {
					return u.op_name == "open";
				});
				REQUIRE(open_it != help_info.unused.end());
				REQUIRE(open_it->description != "");
			}
		}
	}

	GIVEN("a registered binding without a description") {
		k.handle_action("bind", R"(om feedlist set browser "mpv" ; open-in-browser)");

		WHEN("help info is retrieved") {
			const auto help_info = k.get_help_info(Dialog::FeedList);

			THEN("a description is generated from the list of actions") {
				const auto bind_it = std::find_if(help_info.bindings.begin(),
				help_info.bindings.end(), [](const HelpBindInfo& b) {
					return b.key_sequence == "om";
				});
				REQUIRE(bind_it != help_info.bindings.end());
				REQUIRE(bind_it->description == R"(set browser mpv; open-in-browser)");
			}
		}
	}

	GIVEN("a registered binding with a description") {
		k.handle_action("bind",
			R"(om feedlist set browser "mpv" ; open-in-browser -- "open with mpv")");

		WHEN("help info is retrieved") {
			const auto help_info = k.get_help_info(Dialog::FeedList);

			THEN("the provided description is included") {
				const auto bind_it = std::find_if(help_info.bindings.begin(),
				help_info.bindings.end(), [](const HelpBindInfo& b) {
					return b.key_sequence == "om";
				});
				REQUIRE(bind_it != help_info.bindings.end());
				REQUIRE(bind_it->description == "open with mpv");
			}
		}
	}

	GIVEN("a registered binding with only a single action") {
		k.handle_action("bind", "oo feedlist open");

		WHEN("help info is retrieved") {
			const auto help_info = k.get_help_info(Dialog::FeedList);

			THEN("the action name and description are included in the binding info") {
				const auto bind_it = std::find_if(help_info.bindings.begin(),
				help_info.bindings.end(), [](const HelpBindInfo& b) {
					return b.key_sequence == "oo";
				});
				REQUIRE(bind_it != help_info.bindings.end());
				REQUIRE(bind_it->op_name == "open");
				const char* action_description = _("Open feed/article");
				REQUIRE(bind_it->description == action_description);
			}
		}
	}

	SECTION("help info with no configured macros") {
		const auto help_info = k.get_help_info(Dialog::FeedList);
		REQUIRE(help_info.macros.size() == 0);
	}

	SECTION("help info with configured macros") {
		k.handle_action("macro", "a open");
		k.handle_action("macro", "b open -- \"some description\"");

		const auto help_info = k.get_help_info(Dialog::FeedList);
		REQUIRE(help_info.macros.size() == 2);
		REQUIRE(help_info.macros[0].key_sequence == "<macro-prefix>a");
		REQUIRE(help_info.macros[0].description == "");
		REQUIRE(help_info.macros[1].key_sequence == "<macro-prefix>b");
		REQUIRE(help_info.macros[1].description == "some description");
	}
}

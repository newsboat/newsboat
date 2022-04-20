#include "keymap.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "config.h"
#include "confighandlerexception.h"
#include "configparser.h"
#include "logger.h"
#include "strprintf.h"
#include "utils.h"

#include "libnewsboat-ffi/src/keymap.rs.h"

namespace newsboat {

struct OpDesc {
	const Operation op;
	const std::string opstr;
	const std::string default_key;
	const std::string help_text;
	const unsigned short flags;
};

/*
 * This is the list of operations, defining operation, operation name (for
 * keybindings), default key, description, and where it's valid
 */
static const std::vector<OpDesc> opdescs = {
	{
		OP_OPEN,
		"open",
		"ENTER",
		translatable("Open feed/article"),
		KM_FEEDLIST | KM_FILEBROWSER | KM_ARTICLELIST | KM_TAGSELECT |
		KM_FILTERSELECT | KM_URLVIEW | KM_DIALOGS | KM_DIRBROWSER | KM_SEARCHRESULTSLIST
	},
	{
		OP_SWITCH_FOCUS,
		"switch-focus",
		"TAB",
		translatable("Switch focus between widgets"),
		KM_FILEBROWSER | KM_DIRBROWSER
	},
	{OP_QUIT, "quit", "q", translatable("Return to previous dialog/Quit"), KM_BOTH},
	{
		OP_HARDQUIT,
		"hard-quit",
		"Q",
		translatable("Quit program, no confirmation"),
		KM_BOTH
	},
	{
		OP_RELOAD,
		"reload",
		"r",
		translatable("Reload currently selected feed"),
		KM_FEEDLIST | KM_ARTICLELIST
	},
	{OP_RELOADALL, "reload-all", "R", translatable("Reload all feeds"), KM_FEEDLIST},
	{
		OP_MARKFEEDREAD,
		"mark-feed-read",
		"A",
		translatable("Mark feed read"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_MARKALLFEEDSREAD,
		"mark-all-feeds-read",
		"C",
		translatable("Mark all feeds read"),
		KM_FEEDLIST
	},
	{
		OP_MARKALLABOVEASREAD,
		"mark-all-above-as-read",
		"",
		translatable("Mark all above as read"),
		KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{OP_SAVE, "save", "s", translatable("Save article"), KM_ARTICLELIST | KM_ARTICLE | KM_SEARCHRESULTSLIST},
	{OP_SAVEALL, "save-all", "", translatable("Save articles"), KM_ARTICLELIST | KM_SEARCHRESULTSLIST},
	{
		OP_NEXT,
		"next",
		"J",
		translatable("Go to next entry"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_ARTICLE | KM_DIALOGS | KM_DIRBROWSER | KM_FILEBROWSER | KM_FILTERSELECT | KM_TAGSELECT | KM_URLVIEW | KM_SEARCHRESULTSLIST | KM_PODBOAT
	},
	{
		OP_PREV,
		"prev",
		"K",
		translatable("Go to previous entry"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_ARTICLE | KM_DIALOGS | KM_DIRBROWSER | KM_FILEBROWSER | KM_FILTERSELECT | KM_TAGSELECT | KM_URLVIEW | KM_SEARCHRESULTSLIST | KM_PODBOAT
	},
	{
		OP_NEXTUNREAD,
		"next-unread",
		"n",
		translatable("Go to next unread article"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_ARTICLE
	},
	{
		OP_PREVUNREAD,
		"prev-unread",
		"p",
		translatable("Go to previous unread article"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_ARTICLE
	},
	{
		OP_RANDOMUNREAD,
		"random-unread",
		"^K",
		translatable("Go to a random unread article"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_ARTICLE
	},
	{
		OP_OPENBROWSER_AND_MARK,
		"open-in-browser-and-mark-read",
		"O",
		translatable("Open URL of article, or entry in URL view. Mark read"),
		KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_ARTICLE | KM_URLVIEW
	},
	{
		OP_OPENALLUNREADINBROWSER,
		"open-all-unread-in-browser",
		"",
		translatable("Open all unread items of selected feed in browser"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_OPENALLUNREADINBROWSER_AND_MARK,
		"open-all-unread-in-browser-and-mark-read",
		"",
		translatable("Open all unread items of selected feed in browser and mark "
			"read"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_OPENINBROWSER,
		"open-in-browser",
		"o",
		translatable("Open URL of article, feed, or entry in URL view"),
		KM_FEEDLIST | KM_ARTICLELIST  | KM_SEARCHRESULTSLIST| KM_ARTICLE | KM_URLVIEW
	},
	{
		OP_OPENINBROWSER_NONINTERACTIVE,
		"open-in-browser-noninteractively",
		"",
		translatable("Open URL of article, feed, or entry in a browser, non-interactively"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_ARTICLE | KM_URLVIEW
	},
	{
		OP_HELP,
		"help",
		"?",
		translatable("Open help dialog"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_ARTICLE | KM_PODBOAT | KM_URLVIEW
	},
	{
		OP_TOGGLESOURCEVIEW,
		"toggle-source-view",
		"^U",
		translatable("Toggle source view"),
		KM_ARTICLE
	},
	{
		OP_TOGGLEITEMREAD,
		"toggle-article-read",
		"N",
		translatable("Toggle read status for article"),
		KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_ARTICLE
	},
	{
		OP_TOGGLESHOWREAD,
		"toggle-show-read-feeds",
		"l",
		translatable("Toggle show read feeds/articles"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_SHOWURLS,
		"show-urls",
		"u",
		translatable("Show URLs in current article"),
		KM_ARTICLE | KM_SEARCHRESULTSLIST | KM_ARTICLELIST
	},
	{OP_CLEARTAG, "clear-tag", "^T", translatable("Clear current tag"), KM_FEEDLIST},
	{OP_SETTAG, "set-tag", "t", translatable("Select tag"), KM_FEEDLIST},
	{OP_SETTAG, "select-tag", "t", translatable("Select tag"), KM_FEEDLIST},
	{
		OP_SEARCH,
		"open-search",
		"/",
		translatable("Open search dialog"),
		KM_FEEDLIST | KM_HELP | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_ARTICLE
	},
	{OP_GOTO_URL, "goto-url", "#", translatable("Goto URL #"), KM_ARTICLE},
	{OP_GOTO_TITLE, "goto-title", "", translatable("Goto item with title"), KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST},
	{OP_ENQUEUE, "enqueue", "e", translatable("Add download to queue"), KM_ARTICLE},
	{
		OP_RELOADURLS,
		"reload-urls",
		"^R",
		translatable("Reload the list of URLs from the configuration"),
		KM_FEEDLIST
	},
	{OP_PB_DOWNLOAD, "pb-download", "d", translatable("Download file"), KM_PODBOAT},
	{OP_PB_CANCEL, "pb-cancel", "c", translatable("Cancel download"), KM_PODBOAT},
	{
		OP_PB_DELETE,
		"pb-delete",
		"D",
		translatable("Mark download as deleted"),
		KM_PODBOAT
	},
	{
		OP_PB_PURGE,
		"pb-purge",
		"P",
		translatable("Purge finished and deleted downloads from queue"),
		KM_PODBOAT
	},
	{
		OP_PB_TOGGLE_DLALL,
		"pb-toggle-download-all",
		"a",
		translatable("Toggle automatic download on/off"),
		KM_PODBOAT
	},
	{
		OP_PB_PLAY,
		"pb-play",
		"p",
		translatable("Start player with currently selected download"),
		KM_PODBOAT
	},
	{
		OP_PB_MARK_FINISHED,
		"pb-mark-as-finished",
		"m",
		translatable("Mark file as finished (not played)"),
		KM_PODBOAT
	},
	{
		OP_PB_MOREDL,
		"pb-increase-max-dls",
		"+",
		translatable("Increase the number of concurrent downloads"),
		KM_PODBOAT
	},
	{
		OP_PB_LESSDL,
		"pb-decreate-max-dls",
		"-",
		translatable("Decrease the number of concurrent downloads"),
		KM_PODBOAT
	},
	{OP_REDRAW, "redraw", "^L", translatable("Redraw screen"), KM_SYSKEYS},
	{OP_CMDLINE, "cmdline", ":", translatable("Open the commandline"), KM_NEWSBOAT},
	{
		OP_SETFILTER,
		"set-filter",
		"F",
		translatable("Set a filter"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_SELECTFILTER,
		"select-filter",
		"f",
		translatable("Select a predefined filter"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_CLEARFILTER,
		"clear-filter",
		"^F",
		translatable("Clear currently set filter"),
		KM_FEEDLIST | KM_HELP | KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_BOOKMARK,
		"bookmark",
		"^B",
		translatable("Bookmark current link/article"),
		KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_ARTICLE | KM_URLVIEW
	},
	{
		OP_EDITFLAGS,
		"edit-flags",
		"^E",
		translatable("Edit flags"),
		KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_ARTICLE
	},
	{OP_NEXTFEED, "next-feed", "j", translatable("Go to next feed"), KM_ARTICLELIST},
	{
		OP_PREVFEED,
		"prev-feed",
		"k",
		translatable("Go to previous feed"),
		KM_ARTICLELIST
	},
	{
		OP_NEXTUNREADFEED,
		"next-unread-feed",
		"^N",
		translatable("Go to next unread feed"),
		KM_ARTICLELIST
	},
	{
		OP_PREVUNREADFEED,
		"prev-unread-feed",
		"^P",
		translatable("Go to previous unread feed"),
		KM_ARTICLELIST
	},
	{OP_MACROPREFIX, "macro-prefix", ",", translatable("Call a macro"), KM_NEWSBOAT},
	{
		OP_DELETE,
		"delete-article",
		"D",
		translatable("Delete article"),
		KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_ARTICLE
	},
	{
		OP_DELETE_ALL,
		"delete-all-articles",
		"^D",
		translatable("Delete all articles"),
		KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_PURGE_DELETED,
		"purge-deleted",
		"$",
		translatable("Purge deleted articles"),
		KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_EDIT_URLS,
		"edit-urls",
		"E",
		translatable("Edit subscribed URLs"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_CLOSEDIALOG,
		"close-dialog",
		"^X",
		translatable("Close currently selected dialog"),
		KM_DIALOGS
	},
	{
		OP_VIEWDIALOGS,
		"view-dialogs",
		"v",
		translatable("View list of open dialogs"),
		KM_NEWSBOAT
	},
	{
		OP_NEXTDIALOG,
		"next-dialog",
		"^V",
		translatable("Go to next dialog"),
		KM_NEWSBOAT
	},
	{
		OP_PREVDIALOG,
		"prev-dialog",
		"^G",
		translatable("Go to previous dialog"),
		KM_NEWSBOAT
	},
	{
		OP_PIPE_TO,
		"pipe-to",
		"|",
		translatable("Pipe article to command"),
		KM_ARTICLE | KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_SORT,
		"sort",
		"g",
		translatable("Sort current list"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_REVSORT,
		"rev-sort",
		"G",
		translatable("Sort current list (reverse)"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},

	{
		OP_PREVSEARCHRESULTS,
		"prevsearchresults",
		"z",
		translatable("Return to previous search results (if any)"),
		KM_SEARCHRESULTSLIST
	},

	{OP_OPEN_URL_1, "one", "1", translatable("Open URL 1"), KM_URLVIEW | KM_ARTICLE},
	{OP_OPEN_URL_2, "two", "2", translatable("Open URL 2"), KM_URLVIEW | KM_ARTICLE},
	{OP_OPEN_URL_3, "three", "3", translatable("Open URL 3"), KM_URLVIEW | KM_ARTICLE},
	{OP_OPEN_URL_4, "four", "4", translatable("Open URL 4"), KM_URLVIEW | KM_ARTICLE},
	{OP_OPEN_URL_5, "five", "5", translatable("Open URL 5"), KM_URLVIEW | KM_ARTICLE},
	{OP_OPEN_URL_6, "six", "6", translatable("Open URL 6"), KM_URLVIEW | KM_ARTICLE},
	{OP_OPEN_URL_7, "seven", "7", translatable("Open URL 7"), KM_URLVIEW | KM_ARTICLE},
	{OP_OPEN_URL_8, "eight", "8", translatable("Open URL 8"), KM_URLVIEW | KM_ARTICLE},
	{OP_OPEN_URL_9, "nine", "9", translatable("Open URL 9"), KM_URLVIEW | KM_ARTICLE},
	{OP_OPEN_URL_10, "zero", "0", translatable("Open URL 10"), KM_URLVIEW | KM_ARTICLE},

	{OP_CMD_START_1, "cmd-one", "1", translatable("Start cmdline with 1"), KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_TAGSELECT | KM_FILTERSELECT},
	{OP_CMD_START_2, "cmd-two", "2", translatable("Start cmdline with 2"), KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_TAGSELECT | KM_FILTERSELECT},
	{OP_CMD_START_3, "cmd-three", "3", translatable("Start cmdline with 3"), KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_TAGSELECT | KM_FILTERSELECT},
	{OP_CMD_START_4, "cmd-four", "4", translatable("Start cmdline with 4"), KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_TAGSELECT | KM_FILTERSELECT},
	{OP_CMD_START_5, "cmd-five", "5", translatable("Start cmdline with 5"), KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_TAGSELECT | KM_FILTERSELECT},
	{OP_CMD_START_6, "cmd-six", "6", translatable("Start cmdline with 6"), KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_TAGSELECT | KM_FILTERSELECT},
	{OP_CMD_START_7, "cmd-seven", "7", translatable("Start cmdline with 7"), KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_TAGSELECT | KM_FILTERSELECT},
	{OP_CMD_START_8, "cmd-eight", "8", translatable("Start cmdline with 8"), KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_TAGSELECT | KM_FILTERSELECT},
	{OP_CMD_START_9, "cmd-nine", "9", translatable("Start cmdline with 9"), KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_TAGSELECT | KM_FILTERSELECT},

	{OP_SK_UP, "up", "UP", translatable("Move to the previous entry"), KM_SYSKEYS},
	{OP_SK_DOWN, "down", "DOWN", translatable("Move to the next entry"), KM_SYSKEYS},
	{
		OP_SK_PGUP,
		"pageup",
		"PPAGE",
		translatable("Move to the previous page"),
		KM_SYSKEYS
	},
	{
		OP_SK_PGDOWN,
		"pagedown",
		"NPAGE",
		translatable("Move to the next page"),
		KM_SYSKEYS
	},

	{
		OP_SK_HOME,
		"home",
		"HOME",
		translatable("Move to the start of page/list"),
		KM_SYSKEYS
	},
	{
		OP_SK_END,
		"end",
		"END",
		translatable("Move to the end of page/list"),
		KM_SYSKEYS
	},

	{
		OP_INT_END_QUESTION,
		"XXXNOKEY-end-question",
		"end-question",
		"",
		KM_INTERNAL
	},
	{
		OP_INT_CANCEL_QNA,
		"XXXNOKEY-cancel-qna",
		"cancel-qna",
		"",
		KM_INTERNAL
	},
	{
		OP_INT_QNA_NEXTHIST,
		"XXXNOKEY-qna-next-history",
		"qna-next-history",
		"",
		KM_INTERNAL
	},
	{
		OP_INT_QNA_PREVHIST,
		"XXXNOKEY-qna-prev-history",
		"qna-prev-history",
		"",
		KM_INTERNAL
	},

	{OP_INT_SET, "set", "internal-set", "", KM_INTERNAL},

	{OP_INT_GOTO_URL, "gotourl", "internal-goto-url", "", KM_INTERNAL},
};

static const std::map<std::string, std::uint32_t> contexts = {
	{"feedlist", KM_FEEDLIST},
	{"filebrowser", KM_FILEBROWSER},
	{"help", KM_HELP},
	{"articlelist", KM_ARTICLELIST},
	{"article", KM_ARTICLE},
	{"tagselection", KM_TAGSELECT},
	{"filterselection", KM_FILTERSELECT},
	{"urlview", KM_URLVIEW},
	{"podboat", KM_PODBOAT},
	{"dialogs", KM_DIALOGS},
	{"dirbrowser", KM_DIRBROWSER},
	{"searchresultslist", KM_SEARCHRESULTSLIST},
};

KeyMap::KeyMap(unsigned flags)
{
	/*
	 * At startup, initialize the keymap with the default settings from the
	 * list above.
	 */
	LOG(Level::DEBUG, "KeyMap::KeyMap: flags = %x", flags);
	for (const auto& op_desc : opdescs) {
		if (!(op_desc.flags & (flags | KM_INTERNAL | KM_SYSKEYS))) {
			continue;
		}

		// Skip operations without a default key
		if (op_desc.default_key.empty()) {
			continue;
		}

		for (const auto& ctx : contexts) {
			const std::string& context = ctx.first;
			const std::uint32_t context_flag = ctx.second;
			if ((op_desc.flags & (context_flag | KM_INTERNAL | KM_SYSKEYS))) {
				keymap_[context][op_desc.default_key] = op_desc.op;
			}
		}
	}

	keymap_["help"]["b"] = OP_SK_PGUP;
	keymap_["help"]["SPACE"] = OP_SK_PGDOWN;
	keymap_["article"]["b"] = OP_SK_PGUP;
	keymap_["article"]["SPACE"] = OP_SK_PGDOWN;
}

std::vector<KeyMapDesc> KeyMap::get_keymap_descriptions(std::string context)
{
	std::vector<KeyMapDesc> descs;
	for (const auto& opdesc : opdescs) {
		if (!(opdesc.flags & get_flag_from_context(context))) {
			// Ignore operation if it is not valid in this context
			continue;
		}

		bool bound_to_key = false;
		for (const auto& keymap : keymap_[context]) {
			const std::string& key = keymap.first;
			const Operation op = keymap.second;
			if (opdesc.op == op) {
				descs.push_back({key, opdesc.opstr, _(opdesc.help_text.c_str()), context, opdesc.flags});
				bound_to_key = true;
			}
		}
		if (!bound_to_key) {
			LOG(Level::DEBUG,
				"KeyMap::get_keymap_descriptions: found unbound function: %s context = %s",
				opdesc.opstr,
				context);
			descs.push_back({"", opdesc.opstr, _(opdesc.help_text.c_str()), context, opdesc.flags});
		}
	}
	return descs;
}

const std::map<std::string, MacroBinding>& KeyMap::get_macro_descriptions()
{
	return macros_;
}

KeyMap::~KeyMap() {}

void KeyMap::set_key(Operation op,
	const std::string& key,
	const std::string& context)
{
	LOG(Level::DEBUG, "KeyMap::set_key(%d,%s) called", op, key);
	if (context == "all") {
		for (const auto& ctx : contexts) {
			keymap_[ctx.first][key] = op;
		}
	} else {
		keymap_[context][key] = op;
	}
}

void KeyMap::unset_key(const std::string& key, const std::string& context)
{
	LOG(Level::DEBUG, "KeyMap::unset_key(%s) called", key);
	if (context == "all") {
		for (const auto& ctx : contexts) {
			keymap_[ctx.first][key] = OP_NIL;
		}
	} else {
		keymap_[context][key] = OP_NIL;
	}
}

void KeyMap::unset_all_keys(const std::string& context)
{
	LOG(Level::DEBUG, "KeyMap::unset_all_keys(%s) called", context);
	auto internal_ops_only = get_internal_operations();
	if (context == "all") {
		for (const auto& ctx : contexts) {
			keymap_[ctx.first] = internal_ops_only;
		}
	} else {
		keymap_[context] = std::move(internal_ops_only);
	}
}

Operation KeyMap::get_opcode(const std::string& opstr)
{
	for (const auto& opdesc : opdescs) {
		if (opstr == opdesc.opstr) {
			return opdesc.op;
		}
	}
	return OP_NIL;
}

char KeyMap::get_key(const std::string& keycode)
{
	if (keycode == "ENTER") {
		return '\n';
	} else if (keycode == "ESC") {
		return 27;
	} else if (keycode.length() == 2 && keycode[0] == '^') {
		char chr = keycode[1];
		return chr - '@';
	} else if (keycode.length() == 1) { // TODO: implement more keys
		return keycode[0];
	}
	return 0;
}

Operation KeyMap::get_operation(const std::string& keycode,
	const std::string& context)
{
	std::string key;
	LOG(Level::DEBUG,
		"KeyMap::get_operation: keycode = %s context = %s",
		keycode,
		context);
	if (keycode.length() > 0) {
		key = keycode;
	} else {
		key = "NIL";
	}
	return keymap_[context][key];
}

void KeyMap::dump_config(std::vector<std::string>& config_output) const
{
	for (const auto& ctx : contexts) {
		const std::string& context = ctx.first;
		const auto& x = keymap_.at(context);
		for (const auto& keymap : x) {
			if (keymap.second < OP_INT_MIN) {
				std::string configline = "bind-key ";
				configline.append(utils::quote(keymap.first));
				configline.append(" ");
				configline.append(getopname(keymap.second));
				configline.append(" ");
				configline.append(context);
				config_output.push_back(configline);
			}
		}
	}
	for (const auto& macro : macros_) {
		std::string configline = "macro ";
		configline.append(macro.first);
		configline.append(" ");
		for (unsigned int i = 0; i < macro.second.cmds.size(); ++i) {
			const auto& cmd = macro.second.cmds[i];
			configline.append(getopname(cmd.op));
			for (const auto& arg : cmd.args) {
				configline.append(" ");
				configline.append(utils::quote(arg));
			}
			if (i < (macro.second.cmds.size() - 1)) {
				configline.append(" ; ");
			}
		}
		if (macro.second.description.size() >= 1) {
			const auto escaped_string = utils::replace_all(macro.second.description, {
				{R"(\)", R"(\\)"},
				{R"(")", R"(\")"},
			});
			configline.append(strprintf::fmt(R"( -- "%s")", escaped_string));
		}
		config_output.push_back(configline);
	}
}

std::string KeyMap::getopname(Operation op) const
{
	for (const auto& opdesc : opdescs) {
		if (opdesc.op == op) {
			return opdesc.opstr;
		}
	}
	return "<none>";
}

void KeyMap::handle_action(const std::string& action, const std::string& params)
{
	/*
	 * The keymap acts as ConfigActionHandler so that all the key-related
	 * configuration is immediately handed to it.
	 */
	LOG(Level::DEBUG, "KeyMap::handle_action(%s, ...) called", action);
	if (action == "bind-key") {
		const auto tokens = utils::tokenize_quoted(params);
		if (tokens.size() < 2) {
			throw ConfigHandlerException(ActionHandlerStatus::TOO_FEW_PARAMS);
		}
		std::string context = "all";
		if (tokens.size() >= 3) {
			context = tokens[2];
		}
		if (!is_valid_context(context)) {
			throw ConfigHandlerException(strprintf::fmt(
					_("`%s' is not a valid context"), context));
		}
		const Operation op = get_opcode(tokens[1]);
		if (op == OP_NIL) {
			throw ConfigHandlerException(
				strprintf::fmt(_("`%s' is not a valid "
						"key command"),
					tokens[1]));
		}
		set_key(op, tokens[0], context);
	} else if (action == "unbind-key") {
		const auto tokens = utils::tokenize_quoted(params);
		if (tokens.size() < 1) {
			throw ConfigHandlerException(
				ActionHandlerStatus::TOO_FEW_PARAMS);
		}
		std::string context = "all";
		if (tokens.size() >= 2) {
			context = tokens[1];
		}
		if (tokens[0] == "-a") {
			unset_all_keys(context);
		} else {
			unset_key(tokens[0], context);
		}
	} else if (action == "macro") {
		std::string remaining_params = params;
		const auto token = utils::extract_token_quoted(remaining_params);
		const auto parsed = parse_operation_sequence(remaining_params, action);
		const std::vector<MacroCmd> cmds = parsed.operations;
		const std::string description = parsed.description;
		if (!token.has_value() || cmds.empty()) {
			throw ConfigHandlerException(ActionHandlerStatus::TOO_FEW_PARAMS);
		}
		const std::string macrokey = token.value();

		macros_[macrokey] = {cmds, description};
	} else if (action == "run-on-startup") {
		startup_operations_sequence = parse_operation_sequence(params, action, false).operations;
	} else {
		throw ConfigHandlerException(ActionHandlerStatus::INVALID_PARAMS);
	}
}


ParsedOperations KeyMap::parse_operation_sequence(const std::string& line,
	const std::string& command_name, bool allow_description)
{
	rust::String description;
	bool parsing_failed = false;
	const auto operations = keymap::bridged::tokenize_operation_sequence(line, description,
			allow_description, parsing_failed);
	if (parsing_failed) {
		throw ConfigHandlerException(strprintf::fmt(_("failed to parse operation sequence for %s"),
				command_name));
	}

	std::vector<MacroCmd> cmds;
	for (const auto& operation : operations) {
		const auto& tokens = keymap::bridged::operation_tokens(operation);
		if (tokens.empty()) {
			continue;
		}

		const auto command_name = std::string(tokens[0]);
		const auto arguments = std::vector<std::string>(std::next(std::begin(tokens)),
				std::end(tokens));

		MacroCmd cmd;
		cmd.op = get_opcode(command_name);
		if (cmd.op == OP_NIL) {
			throw ConfigHandlerException(strprintf::fmt(_("`%s' is not a valid operation"),
					command_name));
		}
		cmd.args = arguments;

		cmds.push_back(cmd);
	}

	return ParsedOperations{
		.operations = cmds,
		.description = std::string(description)
	};
}

std::vector<MacroCmd> KeyMap::get_startup_operation_sequence()
{
	return startup_operations_sequence;
}

std::vector<std::string> KeyMap::get_keys(Operation op,
	const std::string& context)
{
	std::vector<std::string> keys;
	for (const auto& keymap : keymap_[context]) {
		if (keymap.second == op) {
			keys.push_back(keymap.first);
		}
	}
	return keys;
}

std::vector<MacroCmd> KeyMap::get_macro(const std::string& key)
{
	if (macros_.count(key) >= 1) {
		return macros_.at(key).cmds;
	}
	return {};
}

bool KeyMap::is_valid_context(const std::string& context)
{
	if (context == "all") {
		return true;
	}

	if (contexts.count(context) >= 1) {
		return true;
	}

	return false;
}

std::map<std::string, Operation> KeyMap::get_internal_operations() const
{
	std::map<std::string, Operation> internal_ops;
	for (const auto& opdesc : opdescs) {
		if (opdesc.flags & KM_INTERNAL) {
			internal_ops[opdesc.default_key] = opdesc.op;
		}
	}
	return internal_ops;
}

unsigned short KeyMap::get_flag_from_context(const std::string& context)
{
	if (contexts.count(context) >= 1) {
		return contexts.at(context) | KM_SYSKEYS;
	}

	return 0; // shouldn't happen
}


std::string KeyMap::prepare_keymap_hint(const std::vector<KeyMapHintEntry>& hints,
	const std::string& context)
{
	std::string keymap_hint;
	for (const auto& hint : hints) {
		std::vector<std::string> keys = get_keys(hint.op, context);

		if (keys.empty()) {
			keys = {"<none>"};
		}

		for (auto& key : keys) {
			key = strprintf::fmt("<key>%s</>", utils::quote_for_stfl(key));
		}

		keymap_hint.append(utils::join(keys, "<comma>,</>"));
		keymap_hint.append("<colon>:</>");
		keymap_hint.append(strprintf::fmt("<desc>%s</>", utils::quote_for_stfl(hint.text)));
		keymap_hint.append(" ");
	}
	return keymap_hint;
}

} // namespace newsboat

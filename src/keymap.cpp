#include "keymap.h"

#include <algorithm>
#include <iterator>
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

namespace {
using namespace newsboat;
std::vector<MacroCmd> convert_operations(const
	rust::Vec<newsboat::keymap::bridged::Operation>& operations)
{
	std::vector<MacroCmd> cmds;
	for (const auto& operation : operations) {
		const auto& tokens = operation.tokens;
		if (tokens.empty()) {
			continue;
		}

		const auto command_name = std::string(tokens[0]);
		const auto arguments = std::vector<std::string>(std::next(std::begin(tokens)),
				std::end(tokens));

		MacroCmd cmd;
		cmd.op = KeyMap::get_opcode(command_name);
		if (cmd.op == OP_NIL) {
			throw ConfigHandlerException(strprintf::fmt(_("`%s' is not a valid operation"),
					command_name));
		}
		cmd.args = arguments;

		cmds.push_back(cmd);
	}

	return cmds;
}
}

namespace newsboat {

struct OpDesc {
	const Operation op;
	const std::string opstr;
	const std::optional<KeyCombination> default_key;
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
		KeyCombination("ENTER"),
		translatable("Open feed/article"),
		KM_FEEDLIST | KM_FILEBROWSER | KM_ARTICLELIST | KM_TAGSELECT |
		KM_FILTERSELECT | KM_URLVIEW | KM_DIALOGS | KM_DIRBROWSER | KM_SEARCHRESULTSLIST
	},
	{
		OP_SWITCH_FOCUS,
		"switch-focus",
		KeyCombination("TAB"),
		translatable("Switch focus between widgets"),
		KM_FILEBROWSER | KM_DIRBROWSER
	},
	{
		OP_QUIT,
		"quit",
		KeyCombination("q"),
		translatable("Return to previous dialog/Quit"),
		KM_BOTH
	},
	{
		OP_HARDQUIT,
		"hard-quit",
		KeyCombination("q", ShiftState::Shift),
		translatable("Quit program, no confirmation"),
		KM_BOTH
	},
	{
		OP_RELOAD,
		"reload",
		KeyCombination("r"),
		translatable("Reload currently selected feed"),
		KM_FEEDLIST | KM_ARTICLELIST
	},
	{
		OP_RELOADALL,
		"reload-all",
		KeyCombination("r", ShiftState::Shift),
		translatable("Reload all feeds"),
		KM_FEEDLIST
	},
	{
		OP_MARKFEEDREAD,
		"mark-feed-read",
		KeyCombination("a", ShiftState::Shift),
		translatable("Mark feed read"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_MARKALLFEEDSREAD,
		"mark-all-feeds-read",
		KeyCombination("c", ShiftState::Shift),
		translatable("Mark all feeds read"),
		KM_FEEDLIST
	},
	{
		OP_MARKALLABOVEASREAD,
		"mark-all-above-as-read",
		{},
		translatable("Mark all above as read"),
		KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_SAVE,
		"save",
		KeyCombination("s"),
		translatable("Save article"),
		KM_ARTICLELIST | KM_ARTICLE | KM_SEARCHRESULTSLIST
	},
	{
		OP_SAVEALL,
		"save-all",
		{},
		translatable("Save articles"),
		KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_NEXT,
		"next",
		KeyCombination("j", ShiftState::Shift),
		translatable("Go to next entry"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_ARTICLE | KM_DIALOGS | KM_DIRBROWSER | KM_FILEBROWSER | KM_FILTERSELECT | KM_TAGSELECT | KM_URLVIEW | KM_SEARCHRESULTSLIST | KM_PODBOAT
	},
	{
		OP_PREV,
		"prev",
		KeyCombination("k", ShiftState::Shift),
		translatable("Go to previous entry"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_ARTICLE | KM_DIALOGS | KM_DIRBROWSER | KM_FILEBROWSER | KM_FILTERSELECT | KM_TAGSELECT | KM_URLVIEW | KM_SEARCHRESULTSLIST | KM_PODBOAT
	},
	{
		OP_NEXTUNREAD,
		"next-unread",
		KeyCombination("n"),
		translatable("Go to next unread article"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_ARTICLE
	},
	{
		OP_PREVUNREAD,
		"prev-unread",
		KeyCombination("p"),
		translatable("Go to previous unread article"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_ARTICLE
	},
	{
		OP_RANDOMUNREAD,
		"random-unread",
		KeyCombination("k", ShiftState::NoShift, ControlState::Control),
		translatable("Go to a random unread article"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_ARTICLE
	},
	{
		OP_OPENBROWSER_AND_MARK,
		"open-in-browser-and-mark-read",
		KeyCombination("o", ShiftState::Shift),
		translatable("Open URL of article, or entry in URL view. Mark read"),
		KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_ARTICLE | KM_URLVIEW
	},
	{
		OP_OPENALLUNREADINBROWSER,
		"open-all-unread-in-browser",
		{},
		translatable("Open all unread items of selected feed in browser"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_OPENALLUNREADINBROWSER_AND_MARK,
		"open-all-unread-in-browser-and-mark-read",
		{},
		translatable("Open all unread items of selected feed in browser and mark "
			"read"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_OPENINBROWSER,
		"open-in-browser",
		KeyCombination("o"),
		translatable("Open URL of article, feed, or entry in URL view"),
		KM_FEEDLIST | KM_ARTICLELIST  | KM_SEARCHRESULTSLIST| KM_ARTICLE | KM_URLVIEW
	},
	{
		OP_OPENINBROWSER_NONINTERACTIVE,
		"open-in-browser-noninteractively",
		{},
		translatable("Open URL of article, feed, or entry in a browser, non-interactively"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_ARTICLE | KM_URLVIEW
	},
	{
		OP_HELP,
		"help",
		KeyCombination("?"),
		translatable("Open help dialog"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_ARTICLE | KM_PODBOAT | KM_URLVIEW
	},
	{
		OP_TOGGLESOURCEVIEW,
		"toggle-source-view",
		KeyCombination("u", ShiftState::NoShift, ControlState::Control),
		translatable("Toggle source view"),
		KM_ARTICLE
	},
	{
		OP_TOGGLEITEMREAD,
		"toggle-article-read",
		KeyCombination("n", ShiftState::Shift),
		translatable("Toggle read status for article"),
		KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_ARTICLE
	},
	{
		OP_TOGGLESHOWREAD,
		"toggle-show-read-feeds",
		KeyCombination("l"),
		translatable("Toggle show read feeds/articles"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_SHOWURLS,
		"show-urls",
		KeyCombination("u"),
		translatable("Show URLs in current article"),
		KM_ARTICLE | KM_SEARCHRESULTSLIST | KM_ARTICLELIST
	},
	{
		OP_CLEARTAG,
		"clear-tag",
		KeyCombination("t", ShiftState::NoShift, ControlState::Control),
		translatable("Clear current tag"),
		KM_FEEDLIST
	},
	{
		OP_SETTAG,
		"set-tag",
		KeyCombination("t"),
		translatable("Select tag"),
		KM_FEEDLIST
	},
	{
		OP_SETTAG,
		"select-tag",
		KeyCombination("t"),
		translatable("Select tag"),
		KM_FEEDLIST
	},
	{
		OP_SEARCH,
		"open-search",
		KeyCombination("/"),
		translatable("Open search dialog"),
		KM_FEEDLIST | KM_HELP | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_ARTICLE
	},
	{
		OP_GOTO_URL,
		"goto-url",
		KeyCombination("#"),
		translatable("Goto URL #"),
		KM_ARTICLE
	},
	{
		OP_GOTO_TITLE,
		"goto-title",
		{},
		translatable("Goto item with title"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_ENQUEUE,
		"enqueue",
		KeyCombination("e"),
		translatable("Add download to queue"),
		KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_ARTICLE
	},
	{
		OP_RELOADURLS,
		"reload-urls",
		KeyCombination("r", ShiftState::NoShift, ControlState::Control),
		translatable("Reload the list of URLs from the configuration"),
		KM_FEEDLIST
	},
	{
		OP_PB_DOWNLOAD,
		"pb-download",
		KeyCombination("d"),
		translatable("Download file"),
		KM_PODBOAT
	},
	{
		OP_PB_CANCEL,
		"pb-cancel",
		KeyCombination("c"),
		translatable("Cancel download"),
		KM_PODBOAT
	},
	{
		OP_PB_DELETE,
		"pb-delete",
		KeyCombination("d", ShiftState::Shift),
		translatable("Mark download as deleted"),
		KM_PODBOAT
	},
	{
		OP_PB_PURGE,
		"pb-purge",
		KeyCombination("p", ShiftState::Shift),
		translatable("Purge finished and deleted downloads from queue"),
		KM_PODBOAT
	},
	{
		OP_PB_TOGGLE_DLALL,
		"pb-toggle-download-all",
		KeyCombination("a"),
		translatable("Toggle automatic download on/off"),
		KM_PODBOAT
	},
	{
		OP_PB_PLAY,
		"pb-play",
		KeyCombination("p"),
		translatable("Start player with currently selected download"),
		KM_PODBOAT
	},
	{
		OP_PB_MARK_FINISHED,
		"pb-mark-as-finished",
		KeyCombination("m"),
		translatable("Mark file as finished (not played)"),
		KM_PODBOAT
	},
	{
		OP_PB_MOREDL,
		"pb-increase-max-dls",
		KeyCombination("+"),
		translatable("Increase the number of concurrent downloads"),
		KM_PODBOAT
	},
	{
		OP_PB_LESSDL,
		"pb-decreate-max-dls",
		KeyCombination("-"),
		translatable("Decrease the number of concurrent downloads"),
		KM_PODBOAT
	},
	{
		OP_REDRAW,
		"redraw",
		KeyCombination("l", ShiftState::NoShift, ControlState::Control),
		translatable("Redraw screen"),
		KM_SYSKEYS
	},
	{
		OP_CMDLINE,
		"cmdline",
		KeyCombination(":"),
		translatable("Open the commandline"),
		KM_NEWSBOAT
	},
	{
		OP_SETFILTER,
		"set-filter",
		KeyCombination("f", ShiftState::Shift),
		translatable("Set a filter"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_SELECTFILTER,
		"select-filter",
		KeyCombination("f"),
		translatable("Select a predefined filter"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_CLEARFILTER,
		"clear-filter",
		KeyCombination("f", ShiftState::NoShift, ControlState::Control),
		translatable("Clear currently set filter"),
		KM_FEEDLIST | KM_HELP | KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_BOOKMARK,
		"bookmark",
		KeyCombination("b", ShiftState::NoShift, ControlState::Control),
		translatable("Bookmark current link/article"),
		KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_ARTICLE | KM_URLVIEW
	},
	{
		OP_EDITFLAGS,
		"edit-flags",
		KeyCombination("e", ShiftState::NoShift, ControlState::Control),
		translatable("Edit flags"),
		KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_ARTICLE
	},
	{
		OP_NEXTFEED,
		"next-feed",
		KeyCombination("j"),
		translatable("Go to next feed"),
		KM_ARTICLELIST
	},
	{
		OP_PREVFEED,
		"prev-feed",
		KeyCombination("k"),
		translatable("Go to previous feed"),
		KM_ARTICLELIST
	},
	{
		OP_NEXTUNREADFEED,
		"next-unread-feed",
		KeyCombination("n", ShiftState::NoShift, ControlState::Control),
		translatable("Go to next unread feed"),
		KM_ARTICLELIST
	},
	{
		OP_PREVUNREADFEED,
		"prev-unread-feed",
		KeyCombination("p", ShiftState::NoShift, ControlState::Control),
		translatable("Go to previous unread feed"),
		KM_ARTICLELIST
	},
	{
		OP_MACROPREFIX,
		"macro-prefix",
		KeyCombination(","),
		translatable("Call a macro"),
		KM_NEWSBOAT
	},
	{
		OP_DELETE,
		"delete-article",
		KeyCombination("d", ShiftState::Shift),
		translatable("Delete article"),
		KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_ARTICLE
	},
	{
		OP_DELETE_ALL,
		"delete-all-articles",
		KeyCombination("d", ShiftState::NoShift, ControlState::Control),
		translatable("Delete all articles"),
		KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_PURGE_DELETED,
		"purge-deleted",
		KeyCombination("$"),
		translatable("Purge deleted articles"),
		KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_EDIT_URLS,
		"edit-urls",
		KeyCombination("e", ShiftState::Shift),
		translatable("Edit subscribed URLs"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_CLOSEDIALOG,
		"close-dialog",
		KeyCombination("x", ShiftState::NoShift, ControlState::Control),
		translatable("Close currently selected dialog"),
		KM_DIALOGS
	},
	{
		OP_VIEWDIALOGS,
		"view-dialogs",
		KeyCombination("v"),
		translatable("View list of open dialogs"),
		KM_NEWSBOAT
	},
	{
		OP_NEXTDIALOG,
		"next-dialog",
		KeyCombination("v", ShiftState::NoShift, ControlState::Control),
		translatable("Go to next dialog"),
		KM_NEWSBOAT
	},
	{
		OP_PREVDIALOG,
		"prev-dialog",
		KeyCombination("g", ShiftState::NoShift, ControlState::Control),
		translatable("Go to previous dialog"),
		KM_NEWSBOAT
	},
	{
		OP_PIPE_TO,
		"pipe-to",
		KeyCombination("|"),
		translatable("Pipe article to command"),
		KM_ARTICLE | KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_SORT,
		"sort",
		KeyCombination("g"),
		translatable("Sort current list"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},
	{
		OP_REVSORT,
		"rev-sort",
		KeyCombination("g", ShiftState::Shift),
		translatable("Sort current list (reverse)"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},

	{
		OP_PREVSEARCHRESULTS,
		"prevsearchresults",
		KeyCombination("z"),
		translatable("Return to previous search results (if any)"),
		KM_SEARCHRESULTSLIST
	},
	{
		OP_ARTICLEFEED,
		"article-feed",
		{},
		translatable("Go to the feed of the article"),
		KM_ARTICLE | KM_ARTICLELIST | KM_SEARCHRESULTSLIST
	},

	{
		OP_OPEN_URL_1,
		"one",
		KeyCombination("1"),
		translatable("Open URL 1"),
		KM_URLVIEW | KM_ARTICLE
	},
	{
		OP_OPEN_URL_2,
		"two",
		KeyCombination("2"),
		translatable("Open URL 2"),
		KM_URLVIEW | KM_ARTICLE
	},
	{
		OP_OPEN_URL_3,
		"three",
		KeyCombination("3"),
		translatable("Open URL 3"),
		KM_URLVIEW | KM_ARTICLE
	},
	{
		OP_OPEN_URL_4,
		"four",
		KeyCombination("4"),
		translatable("Open URL 4"),
		KM_URLVIEW | KM_ARTICLE
	},
	{
		OP_OPEN_URL_5,
		"five",
		KeyCombination("5"),
		translatable("Open URL 5"),
		KM_URLVIEW | KM_ARTICLE
	},
	{
		OP_OPEN_URL_6,
		"six",
		KeyCombination("6"),
		translatable("Open URL 6"),
		KM_URLVIEW | KM_ARTICLE
	},
	{
		OP_OPEN_URL_7,
		"seven",
		KeyCombination("7"),
		translatable("Open URL 7"),
		KM_URLVIEW | KM_ARTICLE
	},
	{
		OP_OPEN_URL_8,
		"eight",
		KeyCombination("8"),
		translatable("Open URL 8"),
		KM_URLVIEW | KM_ARTICLE
	},
	{
		OP_OPEN_URL_9,
		"nine",
		KeyCombination("9"),
		translatable("Open URL 9"),
		KM_URLVIEW | KM_ARTICLE
	},
	{
		OP_OPEN_URL_10,
		"zero",
		KeyCombination("0"),
		translatable("Open URL 10"),
		KM_URLVIEW | KM_ARTICLE
	},

	{
		OP_CMD_START_1,
		"cmd-one",
		KeyCombination("1"),
		translatable("Start cmdline with 1"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_TAGSELECT | KM_FILTERSELECT | KM_DIALOGS
	},
	{
		OP_CMD_START_2,
		"cmd-two",
		KeyCombination("2"),
		translatable("Start cmdline with 2"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_TAGSELECT | KM_FILTERSELECT | KM_DIALOGS
	},
	{
		OP_CMD_START_3,
		"cmd-three",
		KeyCombination("3"),
		translatable("Start cmdline with 3"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_TAGSELECT | KM_FILTERSELECT | KM_DIALOGS
	},
	{
		OP_CMD_START_4,
		"cmd-four",
		KeyCombination("4"),
		translatable("Start cmdline with 4"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_TAGSELECT | KM_FILTERSELECT | KM_DIALOGS
	},
	{
		OP_CMD_START_5,
		"cmd-five",
		KeyCombination("5"),
		translatable("Start cmdline with 5"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_TAGSELECT | KM_FILTERSELECT | KM_DIALOGS
	},
	{
		OP_CMD_START_6,
		"cmd-six",
		KeyCombination("6"),
		translatable("Start cmdline with 6"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_TAGSELECT | KM_FILTERSELECT | KM_DIALOGS
	},
	{
		OP_CMD_START_7,
		"cmd-seven",
		KeyCombination("7"),
		translatable("Start cmdline with 7"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_TAGSELECT | KM_FILTERSELECT | KM_DIALOGS
	},
	{
		OP_CMD_START_8,
		"cmd-eight",
		KeyCombination("8"),
		translatable("Start cmdline with 8"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_TAGSELECT | KM_FILTERSELECT | KM_DIALOGS
	},
	{
		OP_CMD_START_9,
		"cmd-nine",
		KeyCombination("9"),
		translatable("Start cmdline with 9"),
		KM_FEEDLIST | KM_ARTICLELIST | KM_SEARCHRESULTSLIST | KM_TAGSELECT | KM_FILTERSELECT | KM_DIALOGS
	},

	{
		OP_SK_UP,
		"up",
		KeyCombination("UP"),
		translatable("Move to the previous entry"),
		KM_SYSKEYS
	},
	{
		OP_SK_DOWN,
		"down",
		KeyCombination("DOWN"),
		translatable("Move to the next entry"),
		KM_SYSKEYS
	},
	{
		OP_SK_PGUP,
		"pageup",
		KeyCombination("PPAGE"),
		translatable("Move to the previous page"),
		KM_SYSKEYS
	},
	{
		OP_SK_PGDOWN,
		"pagedown",
		KeyCombination("NPAGE"),
		translatable("Move to the next page"),
		KM_SYSKEYS
	},
	{
		OP_SK_HALF_PAGE_UP,
		"halfpageup",
		{},
		translatable("Move half page up"),
		KM_SYSKEYS
	},
	{
		OP_SK_HALF_PAGE_DOWN,
		"halfpagedown",
		{},
		translatable("Move half page down"),
		KM_SYSKEYS
	},

	{
		OP_SK_HOME,
		"home",
		KeyCombination("HOME"),
		translatable("Move to the start of page/list"),
		KM_SYSKEYS
	},
	{
		OP_SK_END,
		"end",
		KeyCombination("END"),
		translatable("Move to the end of page/list"),
		KM_SYSKEYS
	},

	{
		OP_SET,
		"set",
		{},
		"",
		KM_INTERNAL
	},
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
		if (!op_desc.default_key.has_value()) {
			continue;
		}

		for (const auto& ctx : contexts) {
			const std::string& context = ctx.first;
			const std::uint32_t context_flag = ctx.second;
			if ((op_desc.flags & (context_flag | KM_INTERNAL | KM_SYSKEYS))) {
				const auto& default_key = op_desc.default_key;
				apply_bindkey(context_keymaps[context], default_key.value(), op_desc.op);
			}
		}
	}

	apply_bindkey(context_keymaps["help"], KeyCombination("b"), OP_SK_PGUP);
	apply_bindkey(context_keymaps["help"], KeyCombination("SPACE"), OP_SK_PGDOWN);
	apply_bindkey(context_keymaps["article"], KeyCombination("b"), OP_SK_PGUP);
	apply_bindkey(context_keymaps["article"], KeyCombination("SPACE"), OP_SK_PGDOWN);
}

HelpInfo KeyMap::get_help_info(std::string context)
{
	const auto& bindings = context_keymaps.at(context);
	std::set<Operation> unused_actions;
	for (const auto& desc : opdescs) {
		if (!(desc.flags & get_flag_from_context(context))) {
			// Ignore operation if it is not valid in this context
			continue;
		}
		unused_actions.insert(desc.op);
	}

	auto help_info_bindings = get_help_info_bindings(unused_actions, bindings);
	std::stable_sort(
		help_info_bindings.begin(),
		help_info_bindings.end(),
	[](const HelpBindInfo& a, const HelpBindInfo& b) {
		return a.op_order_pos < b.op_order_pos;
	});

	std::vector<UnboundAction> unbound_actions;
	for (const auto& desc : opdescs) {
		if (unused_actions.count(desc.op) > 0) {
			unbound_actions.push_back({
				desc.opstr,
				_(desc.help_text.c_str()),
			});
		}
	}

	return HelpInfo {
		help_info_bindings,
		unbound_actions,
		get_help_info_macros(),
	};
}

std::vector<HelpBindInfo> KeyMap::get_help_info_bindings(std::set<Operation>&
	unused_actions, const Mapping& mapping,
	const std::string& key_sequence_prefix)
{
	std::vector<HelpBindInfo> help_info;
	if (mapping.is_leaf_node) {
		const auto& action = mapping.action;
		std::optional<std::string> action_str;
		std::size_t order_pos = opdescs.size();
		std::string description;
		if (action.cmds.size() == 1 && action.cmds.front().args.empty()) {
			const auto it = std::find_if(opdescs.begin(), opdescs.end(), [&](const OpDesc& desc) {
				return desc.op == action.cmds.front().op;
			});
			if (it != opdescs.end()) {
				action_str = it->opstr;
				order_pos = std::distance(opdescs.begin(), it);
				description = _(it->help_text.c_str());
			}
		}
		if (!action.description.empty()) {
			description = action.description;
		}
		if (description.empty()) {
			description = describe_actions(action.cmds);
		}
		help_info.push_back(HelpBindInfo{ key_sequence_prefix, action_str, order_pos, description});

		for (const auto& cmd : action.cmds) {
			unused_actions.erase(cmd.op);
		}
	} else {
		for (const auto& continuation : mapping.continuations) {
			const auto key_combination = continuation.first;
			const auto recursive_help_info = get_help_info_bindings(unused_actions,
					continuation.second,
					key_sequence_prefix + key_combination.to_bind_string());
			help_info.insert(help_info.end(), recursive_help_info.begin(), recursive_help_info.end());
		}
	}
	return help_info;
}

std::vector<HelpMacroInfo> KeyMap::get_help_info_macros()
{
	std::vector<HelpMacroInfo> help_info;
	for (const auto& macro : macros_) {
		// "macro-prefix" is not translated because it refers to an operation name
		const std::string key_sequence = "<macro-prefix>" + macro.first.to_bind_string();
		help_info.push_back(HelpMacroInfo {
			key_sequence,
			macro.second.description,
		});
	}
	return help_info;
}

std::string KeyMap::describe_actions(const std::vector<MacroCmd>& cmds)
{
	bool first = true;
	std::string description;
	for (const auto& cmd : cmds) {
		if (!first) {
			description += "; ";
		}
		const auto it = std::find_if(opdescs.begin(), opdescs.end(), [&](const OpDesc& desc) {
			return desc.op == cmd.op;
		});
		if (it != opdescs.end()) {
			description += it->opstr;
		} else {
			// Shouldn't be possible given the way we register key bindings
			description += "<?>";
		}
		for (const auto& arg : cmd.args) {
			description += " ";
			description += utils::quote_if_necessary(arg);
		}
		first = false;
	}
	return description;
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
		for (const auto& keymap : context_keymaps[context].continuations) {
			const auto& key = keymap.first;
			auto& mapping = keymap.second;
			if (!mapping.is_leaf_node) {
				continue;
			}
			auto& cmds = mapping.action.cmds;
			if (cmds.size() != 1) {
				continue;
			}
			const Operation op = cmds.front().op;
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
			descs.push_back({KeyCombination(""), opdesc.opstr, _(opdesc.help_text.c_str()), context, opdesc.flags});
		}
	}
	return descs;
}

KeyMap::~KeyMap() {}

void KeyMap::set_key(Operation op,
	const KeyCombination& key,
	const std::string& context)
{
	LOG(Level::DEBUG, "KeyMap::set_key(%d,%s) called", op, key.to_bindkey_string());
	if (context == "all") {
		for (const auto& ctx : contexts) {
			apply_bindkey(context_keymaps[ctx.first], key, op);
		}
	} else {
		apply_bindkey(context_keymaps[context], key, op);
	}
}

void KeyMap::unset_key(const KeyCombination& key, const std::string& context)
{
	LOG(Level::DEBUG, "KeyMap::unset_key(%s) called", key.to_bindkey_string());
	if (context == "all") {
		for (const auto& ctx : contexts) {
			context_keymaps[ctx.first].continuations.erase(key);
		}
	} else {
		context_keymaps[context].continuations.erase(key);
	}
}

void KeyMap::unset_all_keys(const std::string& context)
{
	LOG(Level::DEBUG, "KeyMap::unset_all_keys(%s) called", context);
	auto internal_ops_only = get_internal_operations();
	if (context == "all") {
		for (const auto& ctx : contexts) {
			context_keymaps[ctx.first] = internal_ops_only;
		}
	} else {
		context_keymaps[context] = std::move(internal_ops_only);
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
std::string KeyMap::get_op_name(Operation op)
{
	for (const auto& opdesc : opdescs) {
		if (opdesc.op == op) {
			return opdesc.opstr;
		}
	}
	return "";
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

std::vector<MacroCmd> KeyMap::get_operation(const std::vector<KeyCombination>&
	key_sequence, const std::string& context, MultiKeyBindingState& state, BindingType& type)
{
	return get_operation(context_keymaps[context], key_sequence, state, type);
}

std::vector<MacroCmd> KeyMap::get_operation(const Mapping& mapping,
	const std::vector<KeyCombination>& key_sequence, MultiKeyBindingState& state,
	BindingType& type)
{
	if (key_sequence.empty()) {
		if (mapping.is_leaf_node) {
			state = MultiKeyBindingState::Found;
			type = mapping.binding_type;
			return mapping.action.cmds;
		} else {
			state = MultiKeyBindingState::MoreInputNeeded;
			return {};
		}
	} else {
		const auto key_combination = key_sequence.front();
		const auto remainder_key_sequence = std::vector<KeyCombination>(std::next(
					key_sequence.begin()), key_sequence.end());
		if (mapping.is_leaf_node || mapping.continuations.count(key_combination) == 0) {
			state = MultiKeyBindingState::NotFound;
			return {};
		} else {
			return get_operation(mapping.continuations.at(key_combination), remainder_key_sequence,
					state, type);
		}
	}
}

void KeyMap::dump_config(std::vector<std::string>& config_output) const
{
	for (const auto& ctx : contexts) {
		const std::string& context = ctx.first;
		const auto& x = context_keymaps.at(context);
		for (const auto& keymap : x.continuations) {
			const auto& mapping = keymap.second;
			if (!mapping.is_leaf_node) {
				continue;
			}
			auto& cmds = mapping.action.cmds;
			if (cmds.size() != 1) {
				continue;
			}
			auto op = cmds.front().op;
			std::string configline = "bind-key ";
			configline.append(utils::quote(keymap.first.to_bindkey_string()));
			configline.append(" ");
			configline.append(getopname(op));
			configline.append(" ");
			configline.append(context);
			config_output.push_back(configline);
		}
	}
	for (const auto& macro : macros_) {
		std::string configline = "macro ";
		configline.append(macro.first.to_bindkey_string());
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
		const auto key_combination = KeyCombination::from_bindkey(tokens[0]);
		set_key(op, key_combination, context);
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
			const auto key_combination = KeyCombination::from_bindkey(tokens[0]);
			unset_key(key_combination, context);
		}
	} else if (action == "bind") {
		bool parsing_failed = false;
		const auto binding = keymap::bridged::tokenize_binding(params, parsing_failed);
		if (parsing_failed) {
			throw ConfigHandlerException(strprintf::fmt(_("failed to parse binding")));
		}
		auto bind_contexts = std::vector<std::string>(binding.contexts.begin(),
				binding.contexts.end());
		if (bind_contexts.size() == 1 && bind_contexts[0] == "everywhere") {
			bind_contexts.clear();
			for (const auto& context : contexts) {
				bind_contexts.push_back(context.first);
			}
		}
		const auto key_sequence = KeyCombination::from_bind(std::string(binding.key_sequence));
		LOG(Level::DEBUG, "New binding with key sequence:");
		for (const auto& key : key_sequence) {
			LOG(Level::DEBUG, "- %s (%s)", key.to_bind_string(), key.get_key());
		}
		const auto description = std::string(binding.description);
		const auto cmds = convert_operations(binding.operations);
		for (const auto& context : bind_contexts) {
			if (contexts.count(context) == 0) {
				throw ConfigHandlerException(strprintf::fmt(_("unknown context: %s"), context));
			}
			apply_bind(context_keymaps[context], key_sequence, cmds, description, BindingType::Bind);
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
		const auto macrokey = KeyCombination::from_bindkey(token.value());

		macros_[macrokey] = {cmds, description};
	} else if (action == "run-on-startup") {
		startup_operations_sequence = parse_operation_sequence(params, action, false).operations;
	} else {
		throw ConfigHandlerException(ActionHandlerStatus::INVALID_PARAMS);
	}
}

void KeyMap::apply_bind(Mapping& target, const std::vector<KeyCombination> key_sequence,
	const std::vector<MacroCmd>& cmds, const std::string& description, BindingType type)
{
	if (key_sequence.size() == 0) {
		target.is_leaf_node = true;
		target.binding_type = type;
		target.continuations.clear();
		target.action = MacroBinding { cmds, description };
	} else {
		target.is_leaf_node = false;
		target.action = {};
		const auto key_combination = key_sequence.front();
		const auto remainder_key_sequence = std::vector<KeyCombination>(std::next(
					key_sequence.begin()), key_sequence.end());
		apply_bind(
			target.continuations[key_combination],
			remainder_key_sequence,
			cmds,
			description,
			type);
	}
}

void KeyMap::apply_bindkey(Mapping& target, const KeyCombination& key_combination,
	Operation op)
{
	const auto cmds = std::vector<MacroCmd> {
		{op, {}},
	};
	const std::string description = "";
	apply_bind(
		target,
	{key_combination},
	cmds,
	description,
	BindingType::BindKey);
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

	const auto cmds = convert_operations(operations);

	ParsedOperations result;
	result.operations = cmds;
	result.description = std::string(description);
	return result;
}

std::vector<MacroCmd> KeyMap::get_startup_operation_sequence()
{
	return startup_operations_sequence;
}

std::vector<KeyCombination> KeyMap::get_keys(Operation op,
	const std::string& context)
{
	std::vector<KeyCombination> keys;
	for (const auto& keymap : context_keymaps[context].continuations) {
		const auto& mapping = keymap.second;
		if (!mapping.is_leaf_node) {
			continue;
		}
		auto& cmds = mapping.action.cmds;
		if (cmds.size() != 1) {
			continue;
		}
		if (cmds.front().op == op) {
			keys.push_back(keymap.first);
		}
	}
	return keys;
}

std::vector<MacroCmd> KeyMap::get_macro(const KeyCombination& key_combination)
{
	if (macros_.count(key_combination) >= 1) {
		return macros_.at(key_combination).cmds;
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

Mapping KeyMap::get_internal_operations() const
{
	Mapping internal_ops;
	for (const auto& opdesc : opdescs) {
		if (opdesc.flags & KM_INTERNAL) {
			if (!opdesc.default_key.has_value()) {
				continue;
			}
			const auto& default_key = opdesc.default_key.value();
			const std::string description = "";
			const MacroBinding action {
				{ MacroCmd { opdesc.op, {} } },
				description,
			};
			internal_ops.continuations[default_key].is_leaf_node = true;
			internal_ops.continuations[default_key].binding_type = BindingType::BindKey;
			internal_ops.continuations[default_key].action = action;
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
		const std::vector<KeyCombination> bound_keys = get_keys(hint.op, context);

		std::vector<std::string> key_stfl_strings;
		if (bound_keys.empty()) {
			std::string key_string = utils::quote_for_stfl("<none>");
			key_string = strprintf::fmt("<key>%s</>", key_string);
			key_stfl_strings = {key_string};
		} else {
			for (const auto& key : bound_keys) {
				std::string key_string = utils::quote_for_stfl(key.to_bindkey_string());
				key_string = strprintf::fmt("<key>%s</>", key_string);
				key_stfl_strings.push_back(key_string);
			}
		}

		keymap_hint.append(utils::join(key_stfl_strings, "<comma>,</>"));
		keymap_hint.append("<colon>:</>");
		keymap_hint.append(strprintf::fmt("<desc>%s</>", utils::quote_for_stfl(hint.text)));
		keymap_hint.append(" ");
	}
	return keymap_hint;
}

} // namespace newsboat

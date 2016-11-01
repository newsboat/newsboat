#include <keymap.h>
#include <logger.h>
#include <vector>
#include <iostream>
#include <config.h>
#include <exceptions.h>
#include <utils.h>

namespace newsbeuter {

struct op_desc {
	operation op;
	const char * opstr;
	const char * default_key;
	const char * help_text;
	unsigned short flags;
};

/*
 * This is the list of operations, defining operation, operation name (for keybindings), default key, description, and where it's valid
 */
static op_desc opdescs[] = {
	{ OP_OPEN,				"open",						"ENTER", _("Open feed/article"),				KM_FEEDLIST | KM_FILEBROWSER | KM_HELP | KM_ARTICLELIST | KM_TAGSELECT | KM_FILTERSELECT | KM_URLVIEW | KM_PODBEUTER | KM_DIALOGS },
	{ OP_QUIT,				"quit",						"q",	_("Return to previous dialog/Quit"),	KM_FEEDLIST | KM_FILEBROWSER | KM_HELP | KM_ARTICLELIST | KM_ARTICLE | KM_TAGSELECT | KM_FILTERSELECT | KM_URLVIEW | KM_PODBEUTER | KM_DIALOGS },
	{ OP_HARDQUIT,			"hard-quit",				"Q",	_("Quit program,  no confirmation"),	KM_FEEDLIST | KM_FILEBROWSER | KM_HELP | KM_ARTICLELIST | KM_ARTICLE | KM_TAGSELECT | KM_FILTERSELECT | KM_URLVIEW | KM_PODBEUTER | KM_DIALOGS },
	{ OP_RELOAD,			"reload",					"r",	_("Reload currently selected feed"),	KM_FEEDLIST },
	{ OP_RELOADALL,			"reload-all",				"R",	_("Reload all feeds"),					KM_FEEDLIST },
	{ OP_MARKFEEDREAD,		"mark-feed-read",			"A",	_("Mark feed read"),					KM_FEEDLIST | KM_ARTICLELIST },
	{ OP_MARKALLFEEDSREAD,	"mark-all-feeds-read",		"C",	_("Mark all feeds read"),				KM_FEEDLIST },
	{ OP_SAVE,				"save",						"s",	_("Save article"),						KM_ARTICLELIST | KM_ARTICLE },
	{ OP_NEXT,				"next",						"J",	_("Go to next article"),			KM_FEEDLIST | KM_ARTICLELIST | KM_ARTICLE },
	{ OP_PREV,				"prev",						"K",	_("Go to previous article"),		KM_FEEDLIST | KM_ARTICLELIST | KM_ARTICLE },
	{ OP_NEXTUNREAD,		"next-unread",				"n",	_("Go to next unread article"),			KM_FEEDLIST | KM_ARTICLELIST | KM_ARTICLE },
	{ OP_PREVUNREAD,		"prev-unread",				"p",	_("Go to previous unread article"),		KM_FEEDLIST | KM_ARTICLELIST | KM_ARTICLE },
	{ OP_RANDOMUNREAD,		"random-unread",			"^K",	_("Go to a random unread article"),		KM_FEEDLIST | KM_ARTICLELIST | KM_ARTICLE },
	{ OP_OPENBROWSER_AND_MARK, "open-in-browser-and-mark-read",                   "O",    _("Open article in browser and mark read"),           KM_ARTICLELIST },
	{ OP_OPENINBROWSER,		"open-in-browser",			"o",	_("Open article in browser"),			KM_FEEDLIST | KM_ARTICLELIST | KM_ARTICLE },
	{ OP_OPENALLUNREADINBROWSER, "open-all-unread-in-browser", "",	_("Open all unread items of selected feed in browser"),		KM_FEEDLIST | KM_ARTICLELIST },
	{ OP_OPENALLUNREADINBROWSER_AND_MARK, "open-all-unread-in-browser-and-mark-read", "", _("Open all unread items of selected feed in browser and mark read"),	KM_FEEDLIST | KM_ARTICLELIST },
	{ OP_HELP,				"help",						"?",	_("Open help dialog"),					KM_FEEDLIST | KM_ARTICLELIST | KM_ARTICLE | KM_PODBEUTER },
	{ OP_TOGGLESOURCEVIEW,	"toggle-source-view",		"^U",	_("Toggle source view"),				KM_ARTICLE },
	{ OP_TOGGLEITEMREAD,	"toggle-article-read",		"N",	_("Toggle read status for article"),	KM_ARTICLELIST | KM_ARTICLE },
	{ OP_TOGGLESHOWREAD,	"toggle-show-read-feeds",	"l",	_("Toggle show read feeds/articles"),	KM_FEEDLIST | KM_ARTICLELIST },
	{ OP_SHOWURLS,			"show-urls",				"u",	_("Show URLs in current article"),		KM_ARTICLE | KM_ARTICLELIST },
	{ OP_CLEARTAG,			"clear-tag",				"^T",	_("Clear current tag"),					KM_FEEDLIST },
	{ OP_SETTAG,			"set-tag",					"t",	_("Select tag"),						KM_FEEDLIST },
	{ OP_SETTAG,			"select-tag",				"t",	_("Select tag"),						KM_FEEDLIST },
	{ OP_SEARCH,			"open-search",				"/",	_("Open search dialog"),				KM_FEEDLIST | KM_HELP | KM_ARTICLELIST | KM_ARTICLE },
	{ OP_GOTO_URL,			"goto-url",				"#",	_("Goto URL #"),				KM_ARTICLE },
	{ OP_ENQUEUE,			"enqueue",					"e",	_("Add download to queue"),				KM_ARTICLE },
	{ OP_RELOADURLS,		"reload-urls",				"^R",	_("Reload the list of URLs from the configuration"),	KM_FEEDLIST },
	{ OP_PB_DOWNLOAD,		"pb-download",				"d",	_("Download file"),						KM_PODBEUTER },
	{ OP_PB_CANCEL,			"pb-cancel",				"c",	_("Cancel download"),					KM_PODBEUTER },
	{ OP_PB_DELETE,			"pb-delete",				"D",	_("Mark download as deleted"),			KM_PODBEUTER },
	{ OP_PB_PURGE,			"pb-purge",					"P",	_("Purge finished and deleted downloads from queue"),	KM_PODBEUTER },
	{ OP_PB_TOGGLE_DLALL,	"pb-toggle-download-all",	"a",	_("Toggle automatic download on/off"),	KM_PODBEUTER },
	{ OP_PB_PLAY,			"pb-play",					"p",	_("Start player with currently selected download"),		KM_PODBEUTER },
	{ OP_PB_MARK_FINISHED,  "pb-mark-as-finished",          "m",    _("Mark file as finished (not played)"), KM_PODBEUTER },
	{ OP_PB_MOREDL,			"pb-increase-max-dls",		"+",	_("Increase the number of concurrent downloads"),		KM_PODBEUTER },
	{ OP_PB_LESSDL,			"pb-decreate-max-dls",		"-",	_("Decrease the number of concurrent downloads"),		KM_PODBEUTER },
	{ OP_REDRAW,			"redraw",					"^L",	_("Redraw screen"),						KM_SYSKEYS },
	{ OP_CMDLINE,			"cmdline",					":",	_("Open the commandline"),				KM_NEWSBEUTER },
	{ OP_SETFILTER,			"set-filter",				"F",	_("Set a filter"),						KM_FEEDLIST | KM_ARTICLELIST },
	{ OP_SELECTFILTER,		"select-filter",			"f",	_("Select a predefined filter"),		KM_FEEDLIST | KM_ARTICLELIST },
	{ OP_CLEARFILTER,		"clear-filter",				"^F",	_("Clear currently set filter"),		KM_FEEDLIST | KM_HELP | KM_ARTICLELIST },
	{ OP_BOOKMARK,			"bookmark",					"^B",	_("Bookmark current link/article"),		KM_ARTICLELIST | KM_ARTICLE | KM_URLVIEW },
	{ OP_EDITFLAGS,			"edit-flags",				"^E",	_("Edit flags"),						KM_ARTICLELIST | KM_ARTICLE },
	{ OP_NEXTFEED,			"next-feed",				"j",	_("Go to next feed"),					KM_ARTICLELIST },
	{ OP_PREVFEED,			"prev-feed",				"k",	_("Go to previous feed"),				KM_ARTICLELIST },
	{ OP_NEXTUNREADFEED,	"next-unread-feed",			"^N",	_("Go to next unread feed"),			KM_ARTICLELIST },
	{ OP_PREVUNREADFEED,	"prev-unread-feed",			"^P",	_("Go to previous unread feed"),		KM_ARTICLELIST },
	{ OP_MACROPREFIX,		"macro-prefix",				",",	_("Call a macro"),						KM_NEWSBEUTER  },
	{ OP_DELETE,			"delete-article",			"D",	_("Delete article"),					KM_ARTICLELIST | KM_ARTICLE },
	{ OP_PURGE_DELETED,		"purge-deleted",			"$",	_("Purge deleted articles"),			KM_ARTICLELIST },
	{ OP_EDIT_URLS,			"edit-urls",				"E",	_("Edit subscribed URLs"),				KM_FEEDLIST | KM_ARTICLELIST },
	{ OP_CLOSEDIALOG,		"close-dialog",				"^X",	_("Close currently selected dialog"),	KM_DIALOGS },
	{ OP_VIEWDIALOGS,		"view-dialogs",				"v",	_("View list of open dialogs"),			KM_NEWSBEUTER },
	{ OP_NEXTDIALOG,		"next-dialog",				"^V",	_("Go to next dialog"),					KM_NEWSBEUTER },
	{ OP_PREVDIALOG,		"prev-dialog",				"^G",	_("Go to previous dialog"),				KM_NEWSBEUTER },
	{ OP_PIPE_TO,			"pipe-to",					"|",	_("Pipe article to command"),			KM_ARTICLE | KM_ARTICLELIST },
	{ OP_SORT,				"sort",						"g",	_("Sort current list"),					KM_FEEDLIST | KM_ARTICLELIST },
	{ OP_REVSORT,			"rev-sort",					"G",	_("Sort current list (reverse)"),		KM_FEEDLIST | KM_ARTICLELIST },

	{ OP_0,		"zero",	"0",	_("Open URL 10"), KM_URLVIEW | KM_ARTICLE },
	{ OP_1,		"one",	"1",	_("Open URL 1"), KM_URLVIEW | KM_ARTICLE},
	{ OP_2,		"two",	"2",	_("Open URL 2"), KM_URLVIEW | KM_ARTICLE},
	{ OP_3,		"three","3",	_("Open URL 3"), KM_URLVIEW | KM_ARTICLE},
	{ OP_4,		"four",	"4",	_("Open URL 4"), KM_URLVIEW | KM_ARTICLE},
	{ OP_5,		"five",	"5",	_("Open URL 5"), KM_URLVIEW | KM_ARTICLE},
	{ OP_6,		"six",	"6",	_("Open URL 6"), KM_URLVIEW | KM_ARTICLE},
	{ OP_7,		"seven","7",	_("Open URL 7"), KM_URLVIEW | KM_ARTICLE},
	{ OP_8,		"eight","8",	_("Open URL 8"), KM_URLVIEW | KM_ARTICLE},
	{ OP_9,		"nine",	"9",	_("Open URL 9"), KM_URLVIEW | KM_ARTICLE},

	{ OP_SK_UP, "up", "UP", _("Move to the previous entry"), KM_SYSKEYS },
	{ OP_SK_DOWN, "down", "DOWN", _("Move to the next entry"), KM_SYSKEYS },
	{ OP_SK_PGUP, "pageup", "PAGEUP", _("Move to the previous page"), KM_SYSKEYS },
	{ OP_SK_PGDOWN, "pagedown", "PAGEDOWN", _("Move to the next page"), KM_SYSKEYS },

	{ OP_SK_HOME, "home", "HOME", _("Move to the start of page/list"), KM_SYSKEYS },
	{ OP_SK_END, "end", "END", _("Move to the end of page/list"), KM_SYSKEYS },

	{ OP_INT_END_QUESTION, "XXXNOKEY-end-question", "end-question", nullptr, KM_INTERNAL },
	{ OP_INT_CANCEL_QNA, "XXXNOKEY-cancel-qna", "cancel-qna", nullptr, KM_INTERNAL },
	{ OP_INT_QNA_NEXTHIST, "XXXNOKEY-qna-next-history", "qna-next-history", nullptr, KM_INTERNAL },
	{ OP_INT_QNA_PREVHIST, "XXXNOKEY-qna-prev-history", "qna-prev-history", nullptr, KM_INTERNAL },

	{ OP_INT_RESIZE, "RESIZE", "internal-resize", nullptr, KM_INTERNAL },
	{ OP_INT_SET,    "set",    "internal-set",    nullptr, KM_INTERNAL },

	{ OP_INT_GOTO_URL, "gotourl",    "internal-goto-url",    nullptr, KM_INTERNAL },

	{ OP_NIL, nullptr, nullptr, nullptr, 0 }
};

// "all" must be first, the following positions must be the same as the KM_* flag definitions (get_flag_from_context() relies on this).
static const char * contexts[] = { "all", "feedlist", "filebrowser", "help", "articlelist", "article", "tagselection", "filterselection", "urlview", "podbeuter", "dialogs", nullptr };

keymap::keymap(unsigned flags) {
	/*
	 * At startup, initialize the keymap with the default settings from the list above.
	 */
	LOG(LOG_DEBUG, "keymap::keymap: flags = %x", flags);
	for (unsigned int j=1; contexts[j]!=nullptr; j++) {
		std::string ctx(contexts[j]);
		for (int i=0; opdescs[i].op != OP_NIL; ++i) {
			if (opdescs[i].flags & (flags | KM_INTERNAL | KM_SYSKEYS)) {
				keymap_[ctx][opdescs[i].default_key] = opdescs[i].op;
			}
		}
	}
}

void keymap::get_keymap_descriptions(std::vector<keymap_desc>& descs, unsigned short flags) {
	/*
	 * Here we return the keymap descriptions for the specified application (handed to us via flags)
	 * This is used for the help screen.
	 */
	for (unsigned int i=1; contexts[i]!=nullptr; i++) {
		std::string ctx(contexts[i]);

		if (flags & KM_PODBEUTER && ctx != "podbeuter") {
			continue;
		} else if (flags & KM_NEWSBEUTER && ctx == "podbeuter") {
			continue;
		}

		for (unsigned int j=0; opdescs[j].op != OP_NIL; ++j) {
			bool already_added = false;
			for (auto keymap : keymap_[ctx]) {
				operation op = keymap.second;
				if (op != OP_NIL) {
					if (opdescs[j].op == op && opdescs[j].flags & flags) {
						keymap_desc desc;
						desc.key = keymap.first;
						desc.ctx = ctx;
						if (!already_added) {
							desc.cmd = opdescs[j].opstr;
							if (opdescs[j].help_text)
								desc.desc = gettext(opdescs[j].help_text);
							already_added = true;
						}
						desc.flags = opdescs[j].flags;
						descs.push_back(desc);
					}
				}
			}
			if (!already_added) {
				if (opdescs[j].flags & flags) {
					LOG(LOG_DEBUG, "keymap::get_keymap_descriptions: found unbound function: %s ctx = %s", opdescs[j].opstr, ctx.c_str());
					keymap_desc desc;
					desc.ctx = ctx;
					desc.cmd = opdescs[j].opstr;
					if (opdescs[j].help_text)
						desc.desc = gettext(opdescs[j].help_text);
					desc.flags = opdescs[j].flags;
					descs.push_back(desc);
				}
			}
		}
	}
}

keymap::~keymap() { }


void keymap::set_key(operation op, const std::string& key, const std::string& context) {
	LOG(LOG_DEBUG,"keymap::set_key(%d,%s) called", op, key.c_str());
	if (context == "all") {
		for (unsigned int i=0; contexts[i]!=nullptr; i++) {
			keymap_[contexts[i]][key] = op;
		}
	} else {
		keymap_[context][key] = op;
	}
}

void keymap::unset_key(const std::string& key, const std::string& context) {
	LOG(LOG_DEBUG,"keymap::unset_key(%s) called", key.c_str());
	if (context == "all") {
		for (unsigned int i=0; contexts[i]!=nullptr; i++) {
			keymap_[contexts[i]][key] = OP_NIL;
		}
	} else {
		keymap_[context][key] = OP_NIL;
	}
}

operation keymap::get_opcode(const std::string& opstr) {
	for (int i=0; opdescs[i].opstr; ++i) {
		if (opstr == opdescs[i].opstr) {
			return opdescs[i].op;
		}
	}
	return OP_NIL;
}

char keymap::get_key(const std::string& keycode) {
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

operation keymap::get_operation(const std::string& keycode, const std::string& context) {
	std::string key;
	LOG(LOG_DEBUG, "keymap::get_operation: keycode = %s context = %s", keycode.c_str(), context.c_str());
	if (keycode.length() > 0) {
		key = keycode;
	} else {
		key = "NIL";
	}
	return keymap_[context][key];
}

void keymap::dump_config(std::vector<std::string>& config_output) {
	for (unsigned int i=1; contexts[i]!=nullptr; i++) { // TODO: optimize
		std::map<std::string,operation>& x = keymap_[contexts[i]];
		for (auto keymap : x) {
			if (keymap.second < OP_INT_MIN) {
				std::string configline = "bind-key ";
				configline.append(utils::quote(keymap.first));
				configline.append(" ");
				configline.append(getopname(keymap.second));
				configline.append(" ");
				configline.append(contexts[i]);
				config_output.push_back(configline);
			}
		}
	}
	for (auto macro : macros_) {
		std::string configline = "macro ";
		configline.append(macro.first);
		configline.append(" ");
		unsigned int i=0;
		for (auto cmd : macro.second) {
			configline.append(getopname(cmd.op));
			for (auto arg : cmd.args) {
				configline.append(" ");
				configline.append(utils::quote(arg));
			}
			if (i < (macro.second.size()-1))
				configline.append(" ; ");
		}
		config_output.push_back(configline);
	}
}

std::string keymap::getopname(operation op) {
	for (unsigned int i=0; opdescs[i].op != OP_NIL; i++) {
		if (opdescs[i].op == op)
			return opdescs[i].opstr;
	}
	return "<none>";
}

void keymap::handle_action(const std::string& action, const std::vector<std::string>& params) {
	/*
	 * The keymap acts as config_action_handler so that all the key-related configuration is immediately
	 * handed to it.
	 */
	LOG(LOG_DEBUG,"keymap::handle_action(%s, ...) called",action.c_str());
	if (action == "bind-key") {
		if (params.size() < 2)
			throw confighandlerexception(AHS_TOO_FEW_PARAMS);
		std::string context = "all";
		if (params.size() >= 3)
			context = params[2];
		if (!is_valid_context(context))
			throw confighandlerexception(utils::strprintf(_("`%s' is not a valid context"), context.c_str()));
		operation op = get_opcode(params[1]);
		if (op > OP_SK_MIN && op < OP_SK_MAX)
			unset_key(getkey(op, context), context);
		set_key(op, params[0], context);
	} else if (action == "unbind-key") {
		if (params.size() < 1)
			throw confighandlerexception(AHS_TOO_FEW_PARAMS);
		std::string context = "all";
		if (params.size() >= 2)
			context = params[1];
		unset_key(params[0], context);
	} else if (action == "macro") {
		if (params.size() < 1)
			throw confighandlerexception(AHS_TOO_FEW_PARAMS);
		auto it = params.begin();
		std::string macrokey = *it;
		std::vector<macrocmd> cmds;
		macrocmd tmpcmd;
		tmpcmd.op = OP_NIL;
		bool first = true;
		++it;

		while (it != params.end()) {
			if (first && *it != ";") {
				tmpcmd.op = get_opcode(*it);
				LOG(LOG_DEBUG, "keymap::handle_action: new operation `%s' (op = %u)", it->c_str(), tmpcmd.op);
				if (tmpcmd.op == OP_NIL)
					throw confighandlerexception(utils::strprintf(_("`%s' is not a valid key command"), it->c_str()));
				first = false;
			} else {
				if (*it == ";") {
					if (tmpcmd.op != OP_NIL)
						cmds.push_back(tmpcmd);
					tmpcmd.op = OP_NIL;
					tmpcmd.args.clear();
					first = true;
				} else {
					LOG(LOG_DEBUG, "keymap::handle_action: new parameter `%s' (op = %u)", it->c_str());
					tmpcmd.args.push_back(*it);
				}
			}
			++it;
		}
		if (tmpcmd.op != OP_NIL)
			cmds.push_back(tmpcmd);

		macros_[macrokey] = cmds;
	} else
		throw confighandlerexception(AHS_INVALID_PARAMS);
}

std::string keymap::getkey(operation op, const std::string& context) {
	if (context == "all") {
		for (unsigned int i=0; contexts[i]!=nullptr; i++) {
			std::string ctx(contexts[i]);
			for (auto keymap : keymap_[ctx]) {
				if (keymap.second == op)
					return keymap.first;
			}
		}
	} else {
		for (auto keymap : keymap_[context]) {
			if (keymap.second == op)
				return keymap.first;
		}
	}
	return "<none>";
}

std::vector<macrocmd> keymap::get_macro(const std::string& key) {
	for (auto macro : macros_) {
		if (macro.first == key) {
			return macro.second;
		}
	}
	std::vector<macrocmd> dummyvector;
	return dummyvector;
}

bool keymap::is_valid_context(const std::string& context) {
	for (unsigned int i=0; contexts[i]!=nullptr; i++) {
		if (context == contexts[i])
			return true;
	}
	return false;
}

unsigned short keymap::get_flag_from_context(const std::string& context) {
	for (unsigned int i=1; contexts[i]!=nullptr; i++) {
		if (context == contexts[i])
			return (1<<(i-1)) | KM_SYSKEYS;
	}
	return 0; // shouldn't happen
}

}

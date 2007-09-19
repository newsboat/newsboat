#include <keymap.h>
#include <logger.h>
#include <vector>
#include <iostream>
#include <config.h>

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
	{ OP_OPEN, "open", "ENTER", _("Open feed/article"), KM_NEWSBEUTER },
	{ OP_QUIT, "quit", "q", _("Return to previous dialog/Quit"), KM_BOTH },
	{ OP_RELOAD, "reload", "r", _("Reload currently selected feed"), KM_NEWSBEUTER },
	{ OP_RELOADALL, "reload-all", "R", _("Reload all feeds"), KM_NEWSBEUTER },
	{ OP_MARKFEEDREAD, "mark-feed-read", "A", _("Mark feed read"), KM_NEWSBEUTER },
	{ OP_MARKALLFEEDSREAD, "mark-all-feeds-read", "C", _("Mark all feeds read"), KM_NEWSBEUTER },
	{ OP_SAVE, "save", "s", _("Save article"), KM_NEWSBEUTER },
	{ OP_NEXTUNREAD, "next-unread", "n", _("Go to next unread article"), KM_NEWSBEUTER },
	{ OP_PREVUNREAD, "prev-unread", "p", _("Go to previous unread article"), KM_NEWSBEUTER },
	{ OP_OPENINBROWSER, "open-in-browser", "o", _("Open article in browser"), KM_NEWSBEUTER },
	{ OP_HELP, "help", "?", _("Open help dialog"), KM_BOTH },
	{ OP_TOGGLESOURCEVIEW, "toggle-source-view", "^U", _("Toggle source view"), KM_NEWSBEUTER },
	{ OP_TOGGLEITEMREAD, "toggle-article-read", "N", _("Toggle read status for article"), KM_NEWSBEUTER },
	{ OP_TOGGLESHOWREAD, "toggle-show-read-feeds", "l", _("Toggle show read feeds"), KM_NEWSBEUTER },
	{ OP_SHOWURLS, "show-urls", "u", _("Show URLs in current article"), KM_NEWSBEUTER },
	{ OP_CLEARTAG, "clear-tag", "^T", _("Clear current tag"), KM_NEWSBEUTER },
	{ OP_SETTAG, "set-tag", "t", _("Select tag"), KM_NEWSBEUTER },
	{ OP_SEARCH, "open-search", "/", _("Open search dialog"), KM_NEWSBEUTER },
	{ OP_ENQUEUE, "enqueue", "e", _("Add download to queue"), KM_NEWSBEUTER },
	{ OP_RELOADURLS, "reload-urls", "^R", _("Reload the list of URLs from the configuration"), KM_NEWSBEUTER },
	{ OP_PB_DOWNLOAD, "pb-download", "d", _("Download file"), KM_PODBEUTER },
	{ OP_PB_CANCEL, "pb-cancel", "c", _("Cancel download"), KM_PODBEUTER },
	{ OP_PB_DELETE, "pb-delete", "D", _("Mark download as deleted"), KM_PODBEUTER },
	{ OP_PB_PURGE, "pb-purge", "P", _("Purge finished and deleted downloads from queue"), KM_PODBEUTER },
	{ OP_PB_TOGGLE_DLALL, "pb-toggle-download-all", "a", _("Toggle automatic download on/off"), KM_PODBEUTER },
	{ OP_PB_PLAY, "pb-play", "p", _("Start player with currently selected download"), KM_PODBEUTER },
	{ OP_PB_MOREDL, "pb-increase-max-dls", "+", _("Increase the number of concurrent downloads"), KM_PODBEUTER },
	{ OP_PB_LESSDL, "pb-decreate-max-dls", "-", _("Decrease the number of concurrent downloads"), KM_PODBEUTER },
	{ OP_REDRAW, "redraw", "^L", _("Redraw screen"), KM_BOTH },
	{ OP_CMDLINE, "cmdline", ":", _("Open the commandline"), KM_NEWSBEUTER },
	{ OP_SETFILTER, "set-filter", "F", _("Set a filter"), KM_NEWSBEUTER },
	{ OP_SELECTFILTER, "select-filter", "f", _("Select a predefined filter"), KM_NEWSBEUTER },
	{ OP_CLEARFILTER, "clear-filter", "^F", _("Clear currently set filter"), KM_NEWSBEUTER },
	{ OP_BOOKMARK, "bookmark", "^B", _("Bookmark current link/article"), KM_NEWSBEUTER },
	{ OP_EDITFLAGS, "edit-flags", "^E", _("Edit flags"), KM_NEWSBEUTER },
	{ OP_NEXTFEED, "next-unread-feed", "^N", "Go to next unread feed", KM_NEWSBEUTER },
	{ OP_PREVFEED, "prev-unread-feed", "^P", "Go to previous unread feed", KM_NEWSBEUTER },

	{ OP_SK_UP, "up", "UP", NULL, KM_SYSKEYS },
	{ OP_SK_DOWN, "down", "DOWN", NULL, KM_SYSKEYS },
	{ OP_SK_PGUP, "pageup", "PAGEUP", NULL, KM_SYSKEYS },
	{ OP_SK_PGDOWN, "pagedown", "PAGEDOWN", NULL, KM_SYSKEYS },

	{ OP_INT_END_QUESTION, "XXXNOKEY-end-question", "end-question", NULL, KM_INTERNAL },
	{ OP_INT_CANCEL_QNA, "XXXNOKEY-cancel-qna", "cancel-qna", NULL, KM_INTERNAL },
	{ OP_INT_QNA_NEXTHIST, "XXXNOKEY-qna-next-history", "qna-next-history", NULL, KM_INTERNAL },
	{ OP_INT_QNA_PREVHIST, "XXXNOKEY-qna-prev-history", "qna-prev-history", NULL, KM_INTERNAL },

	{ OP_INT_RESIZE, "RESIZE", "internal-resize", NULL, KM_INTERNAL },

	{ OP_NIL, NULL, NULL, NULL, 0 }
};

keymap::keymap(unsigned flags) { 
	/*
	 * At startup, initialize the keymap with the default settings from the list above.
	 */
	for (int i=0;opdescs[i].op != OP_NIL;++i) {
		if (opdescs[i].flags & (flags | KM_INTERNAL | KM_SYSKEYS)) {
			keymap_[opdescs[i].default_key] = opdescs[i].op;
		}
	}
}

void keymap::get_keymap_descriptions(std::vector<keymap_desc>& descs, unsigned short flags) {
	/*
	 * Here we return the keymap descriptions for the specified application (handed to us via flags)
	 * This is used for the help screen.
	 */
	for (std::map<std::string,operation>::iterator it=keymap_.begin();it!=keymap_.end();++it) {
		operation op = it->second;
		if (op != OP_NIL) {
			std::string helptext;
			std::string opname;
			bool add = false;
			for (int i=0;opdescs[i].help_text;++i) {
				if (opdescs[i].op == op && opdescs[i].flags & flags) {
					helptext = gettext(opdescs[i].help_text);
					opname = opdescs[i].opstr;
					add = true;
				}
			}
			if (add) {
				keymap_desc desc;
				desc.key = it->first;
				desc.cmd = opname;
				desc.desc = helptext;
				descs.push_back(desc);
			}
		}
	}
}

keymap::~keymap() { }


void keymap::set_key(operation op, const std::string& key) {
	GetLogger().log(LOG_DEBUG,"keymap::set_key(%d,%s) called", op, key.c_str());
	keymap_[key] = op;
}

void keymap::unset_key(const std::string& key) {
	GetLogger().log(LOG_DEBUG,"keymap::unset_key(%s) called", key.c_str());
	keymap_[key] = OP_NIL;	
}

operation keymap::get_opcode(const std::string& opstr) {
	for (int i=0;opdescs[i].opstr;++i) {
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
	} else if (keycode[0] == '^') {
		char chr = keycode[1];
		return chr - '@';
	} else { // TODO: implement more keys
		return keycode[0];
	}
	return 0;
}

operation keymap::get_operation(const std::string& keycode) {
	std::string key;
	if (keycode.length() > 0) {
		key = keycode;
	} else {
		key = "NIL";
	}
	return keymap_[key];
}

action_handler_status keymap::handle_action(const std::string& action, const std::vector<std::string>& params) {
	/*
	 * The keymap acts as config_action_handler so that all the key-related configuration is immediately
	 * handed to it.
	 */
	GetLogger().log(LOG_DEBUG,"keymap::handle_action(%s, ...) called",action.c_str());
	if (action == "bind-key") {
		if (params.size() < 2) {
			return AHS_TOO_FEW_PARAMS;
		} else {
			operation op = get_opcode(params[1]);
			if (op > OP_SK_MIN && op < OP_SK_MAX)
				unset_key(getkey(get_opcode(params[1])));
			set_key(get_opcode(params[1]), params[0]);
			// keymap_[params[0]] = get_opcode(params[1]);
			return AHS_OK;
		}
	} else if (action == "unbind-key") {
		if (params.size() < 1) {
			return AHS_TOO_FEW_PARAMS;
		} else {
			unset_key(params[0]);
			return AHS_OK;	
		}
	} else
		return AHS_INVALID_PARAMS;
}

std::string keymap::getkey(operation op) {
	for (std::map<std::string,operation>::iterator it=keymap_.begin(); it!=keymap_.end(); ++it) {
		if (it->second == op)
			return it->first;
	}	
	return "<none>";
}


}

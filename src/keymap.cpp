#include <keymap.h>
#include <logger.h>
#include <vector>
#include <iostream>
#include <config.h>

namespace newsbeuter {
	
struct op_desc {
	operation op;
	char * opstr;
	char * default_key;
	char * help_text;
	unsigned short flags;
};

static op_desc opdescs[] = {
	{ OP_OPEN, "open", "enter", _("Open feed/article"), KM_NEWSBEUTER },
	{ OP_QUIT, "quit", "q", _("Return to previous dialog/Quit"), KM_BOTH },
	{ OP_RELOAD, "reload", "r", _("Reload currently selected feed"), KM_NEWSBEUTER },
	{ OP_RELOADALL, "reload-all", "R", _("Reload all feeds"), KM_NEWSBEUTER },
	{ OP_MARKFEEDREAD, "mark-feed-read", "A", _("Mark feed read"), KM_NEWSBEUTER },
	{ OP_MARKALLFEEDSREAD, "mark-all-feeds-read", "C", _("Mark all feeds read"), KM_NEWSBEUTER },
	{ OP_SAVE, "save", "s", _("Save article"), KM_NEWSBEUTER },
	{ OP_NEXTUNREAD, "next-unread", "n", _("Go to next unread article"), KM_NEWSBEUTER },
	{ OP_OPENINBROWSER, "open-in-browser", "o", _("Open article in browser"), KM_NEWSBEUTER },
	{ OP_HELP, "help", "?", _("Open help dialog"), KM_BOTH },
	{ OP_TOGGLESOURCEVIEW, "toggle-source-view", "^u", _("Toggle source view"), KM_NEWSBEUTER },
	{ OP_TOGGLEITEMREAD, "toggle-article-read", "N", _("Toggle read status for article"), KM_NEWSBEUTER },
	{ OP_TOGGLESHOWREAD, "toggle-show-read-feeds", "l", _("Toggle show read feeds"), KM_NEWSBEUTER },
	{ OP_SHOWURLS, "show-urls", "u", _("Show URLs in current article"), KM_NEWSBEUTER },
	{ OP_CLEARTAG, "clear-tag", "^t", _("Clear current tag"), KM_NEWSBEUTER },
	{ OP_SETTAG, "set-tag", "t", _("Select tag"), KM_NEWSBEUTER },
	{ OP_SEARCH, "open-search", "/", _("Open search dialog"), KM_NEWSBEUTER },
	{ OP_ENQUEUE, "enqueue", "e", _("Add download to queue"), KM_NEWSBEUTER },
	{ OP_PB_DOWNLOAD, "pb-download", "d", _("Download file"), KM_PODBEUTER },
	{ OP_PB_CANCEL, "pb-cancel", "c", _("Cancel download"), KM_PODBEUTER },
	{ OP_PB_DELETE, "pb-delete", "D", _("Mark download as deleted"), KM_PODBEUTER },
	{ OP_PB_PURGE, "pb-purge", "P", _("Purge finished and deleted downloads from queue"), KM_PODBEUTER },
	{ OP_PB_TOGGLE_DLALL, "pb-toggle-download-all", "a", _("Toggle automatic download on/off"), KM_PODBEUTER },
	{ OP_PB_PLAY, "pb-play", "p", _("Start player with currently selected download"), KM_PODBEUTER },
	{ OP_PB_MOREDL, "pb-increase-max-dls", "+", _("Increase the number of concurrent downloads"), KM_PODBEUTER },
	{ OP_PB_LESSDL, "pb-decreate-max-dls", "-", _("Decrease the number of concurrent downloads"), KM_PODBEUTER },
	{ OP_REDRAW, "redraw", "^l", _("Redraw screen"), KM_BOTH },
	{ OP_CMDLINE, "cmdline", ":", _("Open the commandline"), KM_NEWSBEUTER },
	{ OP_NIL, NULL, NULL, NULL, 0 }
};

keymap::keymap() { 
	for (int i=0;opdescs[i].help_text;++i) {
		keymap_[opdescs[i].default_key] = opdescs[i].op;
	}
}

void keymap::get_keymap_descriptions(std::vector<std::pair<std::string,std::string> >& descs, unsigned short flags) {
	for (std::map<std::string,operation>::iterator it=keymap_.begin();it!=keymap_.end();++it) {
		operation op = it->second;
		if (op != OP_NIL) {
			std::string helptext;
			bool add = false;
			for (int i=0;opdescs[i].help_text;++i) {
				if (opdescs[i].op == op && opdescs[i].flags & flags) {
					helptext = gettext(opdescs[i].help_text);
					add = true;
				}
			}
			if (add) {
				descs.push_back(std::pair<std::string,std::string>(it->first, helptext));
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
	if (strncmp(keycode.c_str(),"CHAR(",5)==0) {
		unsigned int x;
		char c;
		sscanf(keycode.c_str(),"CHAR(%u)",&x);
		if (x >= 0 && x <= 126) {
			c = static_cast<char>(x);
			return c;
		}
	} else if (keycode == "ENTER") {
		return '\n';
	} else if (keycode == "ESC") {
		return 27;
	}
	return 0;
}

operation keymap::get_operation(const std::string& keycode) {
	std::string key;
	if (keycode.length() > 0) {
		if (keycode == "ENTER") {
			key = "enter";
		} else if (keycode == "ESC") {
			key = "esc";
		} else if (keycode[0] == 'F') {
			key = keycode;
			key[0] = 'f';
		} else if (strncmp(keycode.c_str(),"CHAR(",5)==0) {
			unsigned int x;
			char c;
			sscanf(keycode.c_str(),"CHAR(%u)",&x);
			// std::cerr << x << std::endl;
			if (32 == x) {
				key.append("space");
			} else if (x > 32 && x <= 126) {
				c = static_cast<char>(x);
				key.append(1,c);
			} else if (x<=26) {
				key.append("^");
				key.append(1,static_cast<char>(0x60 + x));
			} else {
				// TODO: handle special keys
			}
		}
	} else {
		key = "NIL";
	}
	return keymap_[key];
}

action_handler_status keymap::handle_action(const std::string& action, const std::vector<std::string>& params) {
	GetLogger().log(LOG_DEBUG,"keymap::handle_action(%s, ...) called",action.c_str());
	if (action == "bind-key") {
		if (params.size() < 2) {
			return AHS_TOO_FEW_PARAMS;
		} else {
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

#include <keymap.h>
#include <logger.h>
#include <vector>
#include <iostream>
#include <config.h>

namespace newsbeuter {
	
struct op_desc {
	operation op;
	char * help_text;
};

op_desc opdescs[] = {
	{ OP_OPEN, _("Open feed/article") },
	{ OP_QUIT, _("Return to previous dialog/Quit") },
	{ OP_RELOAD, _("Reload currently selected feed") },
	{ OP_RELOADALL, _("Reload all feeds") },
	{ OP_MARKFEEDREAD, _("Mark feed read") },
	{ OP_MARKALLFEEDSREAD, _("Mark all feeds read") },
	{ OP_SAVE, _("Save article") },
	{ OP_NEXTUNREAD, _("Go to next unread article") },
	{ OP_OPENINBROWSER, _("Open article in browser") },
	{ OP_HELP, _("Open help dialog") },
	{ OP_TOGGLESOURCEVIEW, _("Toggle source view") },
	{ OP_TOGGLEITEMREAD, _("Toggle read status for article") },
	{ OP_TOGGLESHOWREAD, _("Toggle show read feeds") },
	{ OP_SHOWURLS, _("Show URLs in current article") },
	{ OP_CLEARTAG, _("Clear current tag") },
	{ OP_SETTAG, _("Select tag") },
	{ OP_SEARCH, _("Open search dialog") },
	{ OP_NIL, NULL }
};

keymap::keymap() { 
	keymap_["enter"] = OP_OPEN;
	keymap_["q"] = OP_QUIT;
	keymap_["r"] = OP_RELOAD;
	keymap_["R"] = OP_RELOADALL;
	keymap_["A"] = OP_MARKFEEDREAD;
	keymap_["C"] = OP_MARKALLFEEDSREAD;
	keymap_["s"] = OP_SAVE;
	keymap_["n"] = OP_NEXTUNREAD;
	keymap_["N"] = OP_TOGGLEITEMREAD;
	keymap_["o"] = OP_OPENINBROWSER;
	keymap_["?"] = OP_HELP;
	keymap_["^u"] = OP_TOGGLESOURCEVIEW;
	keymap_["l"] = OP_TOGGLESHOWREAD;
	keymap_["u"] = OP_SHOWURLS;
	keymap_["t"] = OP_SETTAG;
	keymap_["^t"] = OP_CLEARTAG;
	keymap_["/"] = OP_SEARCH;
	keymap_["NIL"] = OP_NIL;
}

void keymap::get_keymap_descriptions(std::vector<std::pair<std::string,std::string> >& descs) {
	for (std::map<std::string,operation>::iterator it=keymap_.begin();it!=keymap_.end();++it) {
		operation op = it->second;
		if (op != OP_NIL) {
			std::string helptext;
			for (int i=0;opdescs[i].help_text;++i)
				if (opdescs[i].op == op)
					helptext = opdescs[i].help_text;
			descs.push_back(std::pair<std::string,std::string>(it->first, helptext));
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
	struct { char * opstr; operation opcode; } opcode_map[] = {
		{ "quit", OP_QUIT },
		{ "reload", OP_RELOAD },
		{ "reload-all", OP_RELOADALL },
		{ "mark-feed-read", OP_MARKFEEDREAD },
		{ "mark-all-feeds-read", OP_MARKALLFEEDSREAD },
		{ "open", OP_OPEN },
		{ "save", OP_SAVE },
		{ "next-unread", OP_NEXTUNREAD },
		{ "open-in-browser", OP_OPENINBROWSER },
		{ "help", OP_HELP },
		{ "toggle-source-view", OP_TOGGLESOURCEVIEW },
		{ "toggle-article-read", OP_TOGGLEITEMREAD },
		{ "toggle-show-read-feeds", OP_TOGGLESHOWREAD },
		{ "show-urls", OP_SHOWURLS },
		{ "clear-tag", OP_CLEARTAG },
		{ "select-tag", OP_SETTAG },
		{ "open-search", OP_SEARCH },
		{ NULL, OP_NIL }
	};
	for (int i=0;opcode_map[i].opstr;++i) {
		if (opstr == opcode_map[i].opstr) {
			return opcode_map[i].opcode;
		}
	}
	return OP_NIL;
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
			} else if (x >= 0 && x<=26) {
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

#include <keymap.h>
#include <vector>

namespace noos {

keymap::keymap() { 
	keymap_["enter"] = OP_OPEN;
	keymap_["q"] = OP_QUIT;
	keymap_["r"] = OP_RELOAD;
	keymap_["R"] = OP_RELOADALL;
	keymap_["A"] = OP_MARKFEEDREAD;
	keymap_["C"] = OP_MARKALLFEEDSREAD;
	keymap_["s"] = OP_SAVE;
	keymap_["n"] = OP_NEXTUNREAD;
	keymap_["o"] = OP_OPENINBROWSER;
}

keymap::~keymap() { }


void keymap::set_key(operation op, const std::string& key) {
	keymap_[key] = op;
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
	// TODO: decode ctrl combinations
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
			sscanf(keycode.c_str(),"CHAR(%d)",&x);
			c = static_cast<char>(x);
			key.append(1,c);
		}
	} else {
		key = "NIL";
	}
	return keymap_[key];
}

action_handler_status keymap::handle_action(const std::string& action, const std::vector<std::string>& params) {
	if (action == "bind-key") {
		if (params.size() < 2) {
			return AHS_TOO_FEW_PARAMS;
		} else {
			keymap_[params[0]] = get_opcode(params[1]);
			return AHS_OK;
		}
	} else
		return AHS_INVALID_PARAMS;
}


}

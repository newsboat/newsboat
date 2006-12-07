#ifndef NOOS_KEYMAP__H
#define NOOS_KEYMAP__H

#include <string>
#include <map>

#include <configparser.h>

// in configuration: bind-key <key> <operation>

namespace noos {

	enum operation { OP_NIL = 0, OP_QUIT, OP_RELOAD, OP_RELOADALL, OP_MARKFEEDREAD, OP_MARKALLFEEDSREAD, OP_OPEN, OP_SAVE, OP_NEXTUNREAD, OP_OPENINBROWSER };

	class keymap : public config_action_handler {
		public:
			keymap();
			~keymap();
			void set_key(operation op, const std::string& key);
			void unset_key(const std::string& key);
			operation get_opcode(const std::string& opstr);
			operation get_operation(const std::string& keycode);
			std::string getkey(operation );
			virtual action_handler_status handle_action(const std::string& action, const std::vector<std::string>& params);
		private:
			std::map<std::string,operation> keymap_;
	};

}


#endif

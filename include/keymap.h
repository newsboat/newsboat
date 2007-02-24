#ifndef NEWSBEUTER_KEYMAP__H
#define NEWSBEUTER_KEYMAP__H

#include <string>
#include <map>
#include <utility>
#include <vector>

#include <configparser.h>

// in configuration: bind-key <key> <operation>

namespace newsbeuter {

	enum operation {	OP_NIL = 0, 
						// general and newsbeuter-specific operations:
						OP_NB_MIN,
						OP_QUIT, 
						OP_RELOAD, 
						OP_RELOADALL, 
						OP_MARKFEEDREAD, 
						OP_MARKALLFEEDSREAD, 
						OP_OPEN, 
						OP_SAVE, 
						OP_NEXTUNREAD, 
						OP_OPENINBROWSER, 
						OP_HELP, 
						OP_TOGGLESOURCEVIEW, 
						OP_TOGGLEITEMREAD, 
						OP_TOGGLESHOWREAD, 
						OP_SHOWURLS,
						OP_CLEARTAG, 
						OP_SETTAG, 
						OP_SEARCH, 
						OP_ENQUEUE,
						OP_NB_MAX,

						// podbeuter-specific operations:
						OP_PB_MIN, 
						OP_PB_DOWNLOAD, 
						OP_PB_CANCEL, 
						OP_PB_DELETE,
						OP_PB_PURGE, 
						OP_PB_TOGGLE_DLALL, 
						OP_PB_MOREDL, 
						OP_PB_LESSDL, 
						OP_PB_MAX };

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
			void get_keymap_descriptions(std::vector<std::pair<std::string,std::string> >& descs, bool is_newsbeuter);
		private:
			std::map<std::string,operation> keymap_;
	};

}


#endif

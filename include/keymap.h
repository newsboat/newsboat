#ifndef NEWSBEUTER_KEYMAP__H
#define NEWSBEUTER_KEYMAP__H

#include <string>
#include <map>
#include <utility>
#include <vector>

#include <configparser.h>

// in configuration: bind-key <key> <operation>


enum {
	KM_FEEDLIST		= 1<<0,
	KM_FILEBROWSER	= 1<<1,
	KM_HELP			= 1<<2,
	KM_ARTICLELIST	= 1<<3,
	KM_ARTICLE		= 1<<4,
	KM_TAGSELECT	= 1<<5,
	KM_FILTERSELECT	= 1<<6,
	KM_URLVIEW		= 1<<7,
	KM_PODBEUTER	= 1<<8,
	KM_SYSKEYS		= 1<<9,
	KM_INTERNAL		= 1<<10,
	KM_DIALOGS		= 1<<11,
	KM_NEWSBEUTER	= KM_FEEDLIST | KM_FILEBROWSER | KM_HELP | KM_ARTICLELIST | KM_ARTICLE | KM_TAGSELECT | KM_FILTERSELECT | KM_URLVIEW | KM_DIALOGS,
	KM_BOTH			= KM_NEWSBEUTER | KM_PODBEUTER
};

namespace newsbeuter {

enum operation {	OP_NIL = 0,
                    // general and newsbeuter-specific operations:
                    OP_NB_MIN,
                    OP_QUIT,
                    OP_HARDQUIT,
                    OP_RELOAD,
                    OP_RELOADALL,
                    OP_MARKFEEDREAD,
                    OP_MARKALLFEEDSREAD,
                    OP_OPEN,
                    OP_SAVE,
                    OP_NEXTUNREAD,
                    OP_PREVUNREAD,
                    OP_NEXT,
                    OP_PREV,
                    OP_OPENINBROWSER,
                    OP_OPENBROWSER_AND_MARK,
                    OP_OPENALLUNREADINBROWSER,
                    OP_OPENALLUNREADINBROWSER_AND_MARK,
                    OP_HELP,
                    OP_TOGGLESOURCEVIEW,
                    OP_TOGGLEITEMREAD,
                    OP_TOGGLESHOWREAD,
                    OP_SHOWURLS,
                    OP_CLEARTAG,
                    OP_SETTAG,
                    OP_SEARCH,
                    OP_GOTO_URL,
                    OP_ENQUEUE,
                    OP_REDRAW,
                    OP_CMDLINE,
                    OP_SETFILTER,
                    OP_CLEARFILTER,
                    OP_SELECTFILTER,
                    OP_RELOADURLS,
                    OP_BOOKMARK,
                    OP_EDITFLAGS,
                    OP_NEXTUNREADFEED,
                    OP_PREVUNREADFEED,
                    OP_NEXTFEED,
                    OP_PREVFEED,
                    OP_MACROPREFIX,
                    OP_DELETE,
                    OP_PURGE_DELETED,
                    OP_EDIT_URLS,
                    OP_CLOSEDIALOG,
                    OP_VIEWDIALOGS,
                    OP_NEXTDIALOG,
                    OP_PREVDIALOG,
                    OP_PIPE_TO,
                    OP_RANDOMUNREAD,
                    OP_SORT,
                    OP_REVSORT,
                    OP_NB_MAX,

                    // podbeuter-specific operations:
                    OP_PB_MIN = 1000,
                    OP_PB_DOWNLOAD,
                    OP_PB_CANCEL,
                    OP_PB_DELETE,
                    OP_PB_PURGE,
                    OP_PB_TOGGLE_DLALL,
                    OP_PB_MOREDL,
                    OP_PB_LESSDL,
                    OP_PB_PLAY,
                    OP_PB_MARK_FINISHED,
                    OP_PB_MAX,

                    OP_SK_MIN = 1500,
                    OP_SK_UP,
                    OP_SK_DOWN,
                    OP_SK_PGUP,
                    OP_SK_PGDOWN,
                    /* TODO: add more user-defined keys here */
                    OP_SK_HOME,
                    OP_SK_END,

                    OP_SK_MAX,

                    OP_INT_MIN = 2000,

                    OP_INT_END_CMDLINE,
                    OP_INT_END_SETFILTER,
                    OP_INT_BM_END,
                    OP_INT_EDITFLAGS_END,
                    OP_INT_START_SEARCH,

                    OP_INT_GOTO_URL,

                    OP_INT_END_QUESTION,
                    OP_INT_CANCEL_QNA,
                    OP_INT_QNA_NEXTHIST,
                    OP_INT_QNA_PREVHIST,

                    OP_INT_RESIZE,
                    OP_INT_SET,

                    OP_INT_MAX,
                    OP_1 = 3001,
                    OP_2,
                    OP_3,
                    OP_4,
                    OP_5,
                    OP_6,
                    OP_7,
                    OP_8,
                    OP_9,
                    OP_0
               };

struct keymap_desc {
	std::string key;
	std::string cmd;
	std::string desc;
	std::string ctx;
	unsigned short flags;
};

struct macrocmd {
	operation op;
	std::vector<std::string> args;
};

class keymap : public config_action_handler {
	public:
		keymap(unsigned int flags);
		~keymap();
		void set_key(operation op, const std::string& key, const std::string& context);
		void unset_key(const std::string& key, const std::string& context);
		operation get_opcode(const std::string& opstr);
		operation get_operation(const std::string& keycode, const std::string& context);
		std::vector<macrocmd> get_macro(const std::string& key);
		char get_key(const std::string& keycode);
		std::string getkey(operation op, const std::string& context);
		virtual void handle_action(const std::string& action, const std::vector<std::string>& params);
		virtual void dump_config(std::vector<std::string>& config_output);
		void get_keymap_descriptions(std::vector<keymap_desc>& descs, unsigned short flags);
		unsigned short get_flag_from_context(const std::string& context);
	private:
		bool is_valid_context(const std::string& context);
		std::string getopname(operation op);
		std::map<std::string, std::map<std::string, operation>> keymap_;
		std::map<std::string,std::vector<macrocmd>> macros_;
};

}


#endif

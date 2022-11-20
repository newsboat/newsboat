#ifndef NEWSBOAT_KEYMAP_H_
#define NEWSBOAT_KEYMAP_H_

#include <map>
#include <utility>
#include <vector>

#include "configactionhandler.h"
#include "utf8string.h"

// in configuration: bind-key <key> <operation>

enum { KM_FEEDLIST = 1 << 0,
	KM_FILEBROWSER = 1 << 1,
	KM_HELP = 1 << 2,
	KM_ARTICLELIST = 1 << 3,
	KM_ARTICLE = 1 << 4,
	KM_TAGSELECT = 1 << 5,
	KM_FILTERSELECT = 1 << 6,
	KM_URLVIEW = 1 << 7,
	KM_PODBOAT = 1 << 8,
	KM_DIALOGS = 1 << 9,
	KM_DIRBROWSER = 1 << 10,
	KM_SYSKEYS = 1 << 11,
	KM_INTERNAL = 1 << 12,
	KM_SEARCHRESULTSLIST = 1 << 13,
	KM_NEWSBOAT = KM_FEEDLIST | KM_FILEBROWSER | KM_HELP | KM_ARTICLELIST |
		KM_ARTICLE | KM_TAGSELECT | KM_FILTERSELECT | KM_URLVIEW |
		KM_DIALOGS | KM_DIRBROWSER | KM_SEARCHRESULTSLIST,
	KM_BOTH = KM_NEWSBOAT | KM_PODBOAT
};

namespace newsboat {

enum Operation {
	OP_NIL = 0,
	// general and newsboat-specific operations:
	OP_NB_MIN,
	OP_QUIT,
	OP_HARDQUIT,
	OP_RELOAD,
	OP_RELOADALL,
	OP_MARKFEEDREAD,
	OP_MARKALLFEEDSREAD,
	OP_MARKALLABOVEASREAD,
	OP_OPEN,
	OP_SWITCH_FOCUS,
	OP_SAVE,
	OP_SAVEALL,
	OP_NEXTUNREAD,
	OP_PREVUNREAD,
	OP_NEXT,
	OP_PREV,
	OP_OPENINBROWSER,
	OP_OPENINBROWSER_NONINTERACTIVE,
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
	OP_GOTO_TITLE,
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
	OP_DELETE_ALL,
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
	OP_PREVSEARCHRESULTS,
	OP_ARTICLEFEED,

	// podboat-specific operations:
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
	OP_INT_GOTO_TITLE,

	OP_INT_GOTO_URL,

	OP_INT_END_QUESTION,
	OP_INT_CANCEL_QNA,
	OP_INT_QNA_NEXTHIST,
	OP_INT_QNA_PREVHIST,

	OP_INT_SET,

	OP_INT_MAX,
	OP_OPEN_URL_1 = 3001,
	OP_OPEN_URL_2,
	OP_OPEN_URL_3,
	OP_OPEN_URL_4,
	OP_OPEN_URL_5,
	OP_OPEN_URL_6,
	OP_OPEN_URL_7,
	OP_OPEN_URL_8,
	OP_OPEN_URL_9,
	OP_OPEN_URL_10,

	OP_CMD_START_1,
	OP_CMD_START_2,
	OP_CMD_START_3,
	OP_CMD_START_4,
	OP_CMD_START_5,
	OP_CMD_START_6,
	OP_CMD_START_7,
	OP_CMD_START_8,
	OP_CMD_START_9,
};

struct KeyMapDesc {
	Utf8String key;
	Utf8String cmd;
	Utf8String desc;
	Utf8String ctx;
	unsigned short flags;
};

struct MacroCmd {
	Operation op;
	std::vector<Utf8String> args;
};

struct MacroBinding {
	std::vector<MacroCmd> cmds;
	Utf8String description;
};

struct ParsedOperations {
	std::vector<MacroCmd> operations;
	Utf8String description;
};

struct KeyMapHintEntry {
	Operation op;
	Utf8String text;
};

class KeyMap : public ConfigActionHandler {
public:
	explicit KeyMap(unsigned int flags);
	~KeyMap() override;
	void set_key(Operation op,
		const Utf8String& key,
		const Utf8String& context);
	void unset_key(const Utf8String& key, const Utf8String& context);
	void unset_all_keys(const Utf8String& context);
	Operation get_opcode(const Utf8String& opstr);
	Operation get_operation(const Utf8String& keycode,
		const Utf8String& context);
	std::vector<MacroCmd> get_macro(const Utf8String& key);
	char get_key(const Utf8String& keycode);
	std::vector<Utf8String> get_keys(Operation op, const Utf8String& context);
	void handle_action(const Utf8String& action,
		const Utf8String& params) override;
	void dump_config(std::vector<Utf8String>& config_output) const override;
	std::vector<KeyMapDesc> get_keymap_descriptions(Utf8String context);
	const std::map<Utf8String, MacroBinding>& get_macro_descriptions();

	std::vector<MacroCmd> get_startup_operation_sequence();

	Utf8String prepare_keymap_hint(const std::vector<KeyMapHintEntry>& hints,
		const Utf8String& context);

private:
	bool is_valid_context(const Utf8String& context);
	unsigned short get_flag_from_context(const Utf8String& context);
	std::map<Utf8String, Operation> get_internal_operations() const;
	Utf8String getopname(Operation op) const;
	ParsedOperations parse_operation_sequence(const Utf8String& line,
		const Utf8String& command_name, bool allow_description = true);

	std::map<Utf8String, std::map<Utf8String, Operation>> keymap_;
	std::map<Utf8String, MacroBinding> macros_;
	std::vector<MacroCmd> startup_operations_sequence;
};

} // namespace newsboat

#endif /* NEWSBOAT_KEYMAP_H_ */

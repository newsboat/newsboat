#ifndef NEWSBOAT_KEYMAP_H_
#define NEWSBOAT_KEYMAP_H_

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "configactionhandler.h"
#include "keycombination.h"

enum class BindingType {
	BindKey,
	Macro,
	Bind,
};

enum class MultiKeyBindingState {
	Found,
	NotFound,
	MoreInputNeeded,
};

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

namespace Newsboat {

enum Operation {
	OP_NIL = 0,
	// general and Newsboat-specific operations:
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
	OP_SET,

	// Podboat-specific operations:
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
	OP_SK_HALF_PAGE_UP,
	OP_SK_HALF_PAGE_DOWN,
	OP_SK_HOME,
	OP_SK_END,

	OP_SK_MAX,

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
	KeyCombination key;
	std::string cmd;
	std::string desc;
	std::string ctx;
	unsigned short flags;
};

struct MacroCmd {
	Operation op;
	std::vector<std::string> args;
};

struct MacroBinding {
	std::vector<MacroCmd> cmds;
	std::string description;
};

struct ParsedOperations {
	std::vector<MacroCmd> operations;
	std::string description;
};

struct KeyMapHintEntry {
	Operation op;
	std::string text;
};

struct Mapping {
	bool is_leaf_node = false;
	BindingType binding_type = BindingType::Bind;
	std::map<KeyCombination, Mapping> continuations = {};
	MacroBinding action = {};
};

struct HelpBindInfo {
	std::string key_sequence;
	std::optional<std::string> op_name;
	std::size_t op_order_pos;
	std::string description;
};

struct HelpMacroInfo {
	std::string key_sequence;
	std::string description;
};

struct UnboundAction {
	std::string op_name;
	std::string description;
};

struct HelpInfo {
	std::vector<HelpBindInfo> bindings;
	std::vector<UnboundAction> unused;
	std::vector<HelpMacroInfo> macros;
};

class KeyMap : public ConfigActionHandler {
public:
	explicit KeyMap(unsigned int flags);
	~KeyMap() override;
	void set_key(Operation op,
		const KeyCombination& key,
		const std::string& context);
	void unset_key(const KeyCombination& key, const std::string& context);
	void unset_all_keys(const std::string& context);
	static Operation get_opcode(const std::string& opstr);
	static std::string get_op_name(Operation op);
	std::vector<MacroCmd> get_operation(const std::vector<KeyCombination>& key_sequence,
		const std::string& context, MultiKeyBindingState& state, BindingType& type);
	std::vector<MacroCmd> get_macro(const KeyCombination& key_combination);
	char get_key(const std::string& keycode);
	std::vector<KeyCombination> get_keys(Operation op, const std::string& context);
	void handle_action(const std::string& action,
		const std::string& params) override;
	void dump_config(std::vector<std::string>& config_output) const override;
	HelpInfo get_help_info(std::string context);
	std::vector<KeyMapDesc> get_keymap_descriptions(std::string context);

	ParsedOperations parse_operation_sequence(const std::string& line,
		const std::string& command_name, bool allow_description = true);
	std::vector<MacroCmd> get_startup_operation_sequence();

	std::string prepare_keymap_hint(const std::vector<KeyMapHintEntry>& hints,
		const std::string& context);

private:
	std::vector<HelpBindInfo> get_help_info_bindings(std::set<Operation>& unused_actions,
		const Mapping& mapping,
		const std::string& key_sequence_prefix = "");
	std::vector<HelpMacroInfo> get_help_info_macros();
	std::string describe_actions(const std::vector<MacroCmd>& cmds);
	std::vector<MacroCmd> get_operation(const Mapping& mapping,
		const std::vector<KeyCombination>& key_sequence, MultiKeyBindingState& state,
		BindingType& type);
	void apply_bind(Mapping& target, const std::vector<KeyCombination> key_sequence,
		const std::vector<MacroCmd>& cmds, const std::string& description, BindingType type);
	void apply_bindkey(Mapping& target, const KeyCombination& key_combination, Operation op);
	bool is_valid_context(const std::string& context);
	unsigned short get_flag_from_context(const std::string& context);
	Mapping get_internal_operations() const;
	std::string getopname(Operation op) const;
	std::map<std::string, Mapping> context_keymaps;
	std::map<KeyCombination, MacroBinding> macros_;
	std::vector<MacroCmd> startup_operations_sequence;
};

} // namespace Newsboat

#endif /* NEWSBOAT_KEYMAP_H_ */

#ifndef NEWSBOAT_FORMACTION_H_
#define NEWSBOAT_FORMACTION_H_

#include <memory>
#include <string>
#include <vector>

#include "history.h"
#include "keymap.h"
#include "lineedit.h"
#include "lineview.h"
#include "listwidget.h"
#include "stflpp.h"

namespace newsboat {

class ConfigContainer;
class RssFeed;
class TextviewWidget;
class View;

typedef std::pair<std::string, std::string> QnaPair;

enum class CommandType {
	QUIT,
	SAVE,
	GOTO,
	TAG,
	SET,
	SOURCE,
	DUMPCONFIG,
	EXEC,
	UNKNOWN,	/// Unknown/non-existing command. Tokenized input is stored in Command.args
	INVALID, 	/// differs from UNKNOWN in that no input was parsed
};

struct Command {
	CommandType type;
	std::vector<std::string> args;

	Command(CommandType type, std::vector<std::string> args = {})
		: type(type)
		, args(args)
	{}
};

enum class QnaFinishAction {
	None,
	Bookmark,
	RunCmdLine,
	GotoUrlByNumber,
	UpdateFlags,
	Search,
	PipeItemIntoProgram,
	GotoTitle,
	SetFilter,
};

class FormAction {
public:
	FormAction(View&, std::string formstr, ConfigContainer* cfg);
	virtual ~FormAction() = default;
	virtual void prepare() = 0;
	virtual void init() = 0;
	virtual void set_redraw(bool b)
	{
		do_redraw = b;
	}

	virtual std::vector<KeyMapHintEntry> get_keymap_hint() const = 0;

	virtual std::string id() const = 0;

	std::string get_value(const std::string& name);
	void set_value(const std::string& name, const std::string& value);
	void set_status(const std::string& text);

	void draw_form();
	std::string draw_form_wait_for_event(unsigned int timeout);
	void recalculate_widget_dimensions();

	virtual void handle_cmdline(const std::string& cmd);

	bool process_op(Operation op,
		const std::vector<std::string>& args,
		BindingType bindingType = BindingType::BindKey);

	virtual void finished_qna(QnaFinishAction op);

	void start_cmdline(std::string default_value = "");

	void start_qna(const std::vector<QnaPair>& prompts,
		QnaFinishAction finish_op,
		History* h = nullptr);
	void finish_qna_question();
	void cancel_qna();
	void qna_next_history();
	void qna_previous_history();
	void clear_line();
	void clear_eol();
	void delete_word();
	void handle_cmdline_completion();

	void handle_qna_event(std::string event, bool inside_cmd);

	void set_parent_formaction(std::shared_ptr<FormAction> fa)
	{
		parent_formaction = fa;
	}
	std::shared_ptr<FormAction> get_parent_formaction() const
	{
		return parent_formaction;
	}

	virtual std::string title() = 0;

	virtual std::vector<std::string> get_suggestions(
		const std::string& fragment);

	static void load_histories(const Filepath& searchfile,
		const Filepath& cmdlinefile);
	static void save_histories(const Filepath& searchfile,
		const Filepath& cmdlinefile,
		unsigned int limit);

	std::string bookmark(const std::string& url,
		const std::string& title,
		const std::string& description,
		const std::string& feed_title);

protected:
	virtual bool process_operation(Operation op,
		const std::vector<std::string>& args,
		BindingType bindingType = BindingType::BindKey) = 0;
	void report_unhandled_operation(Operation op);
	void set_keymap_hints();

	/// The name of the "main" STFL widget, i.e. the one that should be focused
	/// by default.
	virtual std::string main_widget() const = 0;

	void set_title(const std::string& title);

	void start_bookmark_qna(const std::string& default_title,
		const std::string& default_url,
		const std::string& default_feed_title);

	static Command parse_command(const std::string& input,
		std::string delimiters = " \r\n\t");

	void handle_parsed_command(const Command& command);

	bool handle_list_operations(ListWidget& list, Operation op);
	bool handle_textview_operations(TextviewWidget& textview, Operation op);

	View& v;
	ConfigContainer* cfg;
	Stfl::Form f;
	bool do_redraw;

	std::vector<std::string> qna_responses;

	static History searchhistory;
	static History cmdlinehistory;

	std::vector<std::string> valid_cmds;

private:
	void start_next_question();
	bool handle_single_argument_set(std::string argument);
	void handle_set(const std::vector<std::string>& args);
	void handle_quit();
	void handle_source(const std::vector<std::string>& args);
	void handle_dumpconfig(const std::vector<std::string>& args);
	void handle_exec(const std::vector<std::string>& args);

	LineView head_line;
	LineView msg_line;
	LineView qna_prompt_line;
	LineEdit qna_input;
	std::vector<QnaPair> qna_prompts;
	QnaFinishAction qna_finish_operation;
	History* qna_history;
	std::shared_ptr<FormAction> parent_formaction;

	// cmdline completion
	std::string last_fragment;
	std::vector<std::string> suggestions;
	unsigned int tab_count;
};

} // namespace newsboat

#endif /* NEWSBOAT_FORMACTION_H_ */

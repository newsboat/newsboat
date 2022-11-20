#ifndef NEWSBOAT_FORMACTION_H_
#define NEWSBOAT_FORMACTION_H_

#include <memory>
#include <vector>

#include "history.h"
#include "keymap.h"
#include "listwidget.h"
#include "stflpp.h"
#include "utf8string.h"

namespace newsboat {

class ConfigContainer;
class RssFeed;
class View;

using QnaPair = std::pair<Utf8String, Utf8String>;

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
	INVALID,	/// differs from UNKNOWN in that no input was parsed
};

struct Command {
	CommandType type;
	std::vector<Utf8String> args;
};

class FormAction {
public:
	FormAction(View*, Utf8String formstr, ConfigContainer* cfg);
	virtual ~FormAction();
	virtual void prepare() = 0;
	virtual void init() = 0;
	virtual void set_redraw(bool b)
	{
		do_redraw = b;
	}

	virtual const std::vector<KeyMapHintEntry>& get_keymap_hint() const = 0;

	virtual Utf8String id() const = 0;

	Utf8String get_value(const Utf8String& name);
	void set_value(const Utf8String& name, const Utf8String& value);

	void draw_form();
	Utf8String draw_form_wait_for_event(unsigned int timeout);
	void recalculate_widget_dimensions();

	virtual void handle_cmdline(const Utf8String& cmd);

	bool process_op(Operation op,
		bool automatic = false,
		std::vector<Utf8String>* args = nullptr);

	virtual void finished_qna(Operation op);

	void start_cmdline(Utf8String default_value = "");

	void start_qna(const std::vector<QnaPair>& prompts,
		Operation finish_op,
		History* h = nullptr);

	void set_parent_formaction(std::shared_ptr<FormAction> fa)
	{
		parent_formaction = fa;
	}
	std::shared_ptr<FormAction> get_parent_formaction() const
	{
		return parent_formaction;
	}

	virtual Utf8String title() = 0;

	virtual std::vector<Utf8String> get_suggestions(
		const Utf8String& fragment);

	static void load_histories(const Utf8String& searchfile,
		const Utf8String& cmdlinefile);
	static void save_histories(const Utf8String& searchfile,
		const Utf8String& cmdlinefile,
		unsigned int limit);

	Utf8String bookmark(const Utf8String& url,
		const Utf8String& title,
		const Utf8String& description,
		const Utf8String& feed_title);

protected:
	virtual bool process_operation(Operation op,
		bool automatic = false,
		std::vector<Utf8String>* args = nullptr) = 0;
	virtual void set_keymap_hints();

	void start_bookmark_qna(const Utf8String& default_title,
		const Utf8String& default_url,
		const Utf8String& default_feed_title);

	static Command parse_command(const Utf8String& input,
		Utf8String delimiters = " \r\n\t");

	void handle_parsed_command(const Command& command);

	bool handle_list_operations(ListWidget& list, Operation op);

	View* v;
	ConfigContainer* cfg;
	Stfl::Form f;
	bool do_redraw;

	std::vector<Utf8String> qna_responses;

	static History searchhistory;
	static History cmdlinehistory;

	std::vector<Utf8String> valid_cmds;

private:
	void start_next_question();
	bool handle_single_argument_set(Utf8String argument);
	void handle_set(const std::vector<Utf8String>& args);
	void handle_quit();
	void handle_source(const std::vector<Utf8String>& args);
	void handle_dumpconfig(const std::vector<Utf8String>& args);
	void handle_exec(const std::vector<Utf8String>& args);

	std::vector<QnaPair> qna_prompts;
	Operation finish_operation;
	History* qna_history;
	std::shared_ptr<FormAction> parent_formaction;
};

} // namespace newsboat

#endif /* NEWSBOAT_FORMACTION_H_ */

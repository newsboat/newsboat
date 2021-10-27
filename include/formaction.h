#ifndef NEWSBOAT_FORMACTION_H_
#define NEWSBOAT_FORMACTION_H_

#include <memory>
#include <string>
#include <vector>

#include "history.h"
#include "keymap.h"
#include "stflpp.h"

namespace newsboat {

class ConfigContainer;
class RssFeed;
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
	UNKNOWN,	/// Unknown/non-existing command. Input is stored in Command.args[0] of 
	INVALID, 	/// differs from UNKNOWN in that no input was parsed
};

struct Command {
	CommandType type;
	std::vector<std::string> args;
};

class FormAction {
public:
	FormAction(View*, std::string formstr, ConfigContainer* cfg);
	virtual ~FormAction();
	virtual void prepare() = 0;
	virtual void init() = 0;
	virtual void set_redraw(bool b)
	{
		do_redraw = b;
	}

	virtual const std::vector<KeyMapHintEntry>& get_keymap_hint() const = 0;

	virtual std::string id() const = 0;

	std::string get_value(const std::string& name);
	void set_value(const std::string& name, const std::string& value);

	void draw_form();
	std::string draw_form_wait_for_event(unsigned int timeout);
	void recalculate_widget_dimensions();

	virtual void handle_parsed_command(const Command& command);
	virtual void handle_cmdline(const std::string& cmd);
	bool handle_single_argument_set(std::string argument);

	void handle_set(const std::vector<std::string>& args);
	void handle_quit();
	void handle_source(const std::vector<std::string>& args);
	void handle_dumpconfig(const std::vector<std::string>& args);
	void handle_exec(const std::vector<std::string>& args);

	bool process_op(Operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr);

	virtual void finished_qna(Operation op);

	void start_cmdline(std::string default_value = "");

	std::string get_qna_response(unsigned int i)
	{
		return (qna_responses.size() >= (i + 1)) ? qna_responses[i]
			: "";
	}
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

	virtual std::string title() = 0;

	virtual std::vector<std::string> get_suggestions(
		const std::string& fragment);

	static void load_histories(const std::string& searchfile,
		const std::string& cmdlinefile);
	static void save_histories(const std::string& searchfile,
		const std::string& cmdlinefile,
		unsigned int limit);

	std::string bookmark(const std::string& url,
		const std::string& title,
		const std::string& description,
		const std::string& feed_title);

protected:
	virtual bool process_operation(Operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr) = 0;
	virtual void set_keymap_hints();

	void start_bookmark_qna(const std::string& default_title,
		const std::string& default_url,
		const std::string& default_feed_title);

	static Command parse_command(const std::string& input,
		std::string delimiters = " \r\n\t");

	View* v;
	ConfigContainer* cfg;
	Stfl::Form f;
	bool do_redraw;

	std::vector<std::string> qna_responses;

	static History searchhistory;
	static History cmdlinehistory;

	std::vector<std::string> valid_cmds;

private:
	void start_next_question();

	std::vector<QnaPair> qna_prompts;
	Operation finish_operation;
	History* qna_history;
	std::shared_ptr<FormAction> parent_formaction;
};

} // namespace newsboat

#endif /* NEWSBOAT_FORMACTION_H_ */

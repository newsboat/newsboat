#ifndef NEWSBOAT_FORMACTION_H_
#define NEWSBOAT_FORMACTION_H_

#include <string>
#include <vector>

#include "history.h"
#include "keymap.h"
#include "rss.h"
#include "stflpp.h"

namespace newsboat {

class View;

struct KeyMapHintEntry {
	Operation op;
	char* text;
};

typedef std::pair<std::string, std::string> QnaPair;

class FormAction {
public:
	FormAction(View*, std::string formstr, ConfigContainer* cfg);
	virtual ~FormAction();
	virtual void prepare() = 0;
	virtual void init() = 0;
	std::shared_ptr<Stfl::Form> get_form();
	virtual void set_redraw(bool b)
	{
		do_redraw = b;
	}

	virtual KeyMapHintEntry* get_keymap_hint() = 0;

	virtual std::string id() const = 0;

	virtual std::string get_value(const std::string& value);

	virtual void handle_cmdline(const std::string& cmd);

	void process_op(Operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr);

	virtual void finished_qna(Operation op);

	void start_cmdline(std::string default_value = "");

	virtual void recalculate_form();

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
	virtual void process_operation(Operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr) = 0;
	virtual void set_keymap_hints();

	void start_bookmark_qna(const std::string& default_title,
		const std::string& default_url,
		const std::string& default_desc,
		const std::string& default_feed_title);
	void open_unread_items_in_browser(std::shared_ptr<RssFeed> feed,
		bool markread);

	View* v;
	ConfigContainer* cfg;
	std::shared_ptr<Stfl::Form> f;
	bool do_redraw;

	std::vector<std::string> qna_responses;

	static History searchhistory;
	static History cmdlinehistory;

	std::vector<std::string> valid_cmds;

private:
	std::string prepare_keymap_hint(KeyMapHintEntry* hints);
	void start_next_question();

	std::vector<QnaPair> qna_prompts;
	Operation finish_operation;
	History* qna_history;
	std::shared_ptr<FormAction> parent_formaction;
};

} // namespace newsboat

#endif /* NEWSBOAT_FORMACTION_H_ */

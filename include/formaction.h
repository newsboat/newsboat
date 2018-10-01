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

struct keymap_hint_entry {
	operation op;
	char* text;
};

typedef std::pair<std::string, std::string> qna_pair;

class Formaction {
public:
	Formaction(View*, std::string formstr);
	virtual ~Formaction();
	virtual void prepare() = 0;
	virtual void init() = 0;
	std::shared_ptr<Stfl::Form> get_form();
	virtual void set_redraw(bool b)
	{
		do_redraw = b;
	}

	virtual keymap_hint_entry* get_keymap_hint() = 0;

	virtual std::string id() const = 0;

	virtual std::string get_value(const std::string& value);

	virtual void handle_cmdline(const std::string& cmd);

	void process_op(operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr);

	virtual void finished_qna(operation op);

	void start_cmdline(std::string default_value = "");

	virtual void recalculate_form();

	std::string get_qna_response(unsigned int i)
	{
		return (qna_responses.size() >= (i + 1)) ? qna_responses[i]
							 : "";
	}
	void start_qna(const std::vector<qna_pair>& prompts,
		operation finish_op,
		History* h = nullptr);

	void set_parent_formaction(std::shared_ptr<Formaction> fa)
	{
		parent_formaction = fa;
	}
	std::shared_ptr<Formaction> get_parent_formaction() const
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
	virtual void process_operation(operation op,
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
	std::shared_ptr<Stfl::Form> f;
	bool do_redraw;

	std::vector<std::string> qna_responses;

	static History searchhistory;
	static History cmdlinehistory;

	std::vector<std::string> valid_cmds;

private:
	std::string prepare_keymap_hint(keymap_hint_entry* hints);
	void start_next_question();

	std::vector<qna_pair> qna_prompts;
	operation finish_operation;
	History* qna_history;
	std::shared_ptr<Formaction> parent_formaction;
};

} // namespace newsboat

#endif /* NEWSBOAT_FORMACTION_H_ */

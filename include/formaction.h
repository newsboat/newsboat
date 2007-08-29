#ifndef NEWSBEUTER_FORMACTION__H
#define NEWSBEUTER_FORMACTION__H

#include <stflpp.h>
#include <keymap.h>
#include <rss.h>
#include <history.h>

#include <vector>
#include <string>

namespace newsbeuter {

class view;

struct keymap_hint_entry {
	operation op; 
	char * text;
};

class formaction {
	public:
		formaction(view *, std::string formstr);
		virtual ~formaction();
		virtual void prepare() = 0;
		virtual void init() = 0;
		stfl::form * get_form();
		inline void set_redraw(bool b) { do_redraw = b; }

		virtual keymap_hint_entry * get_keymap_hint() = 0;

		virtual std::string get_value(const std::string& value);

		virtual void handle_cmdline(const std::string& cmd);

		void process_op(operation op);

		virtual void finished_qna(operation op);


	protected:
		virtual void process_operation(operation op) = 0;
		virtual void set_keymap_hints();

		void start_qna(const std::vector<std::pair<std::string, std::string> >& prompts, operation finish_op);

		void start_bookmark_qna(const std::string& default_title, const std::string& default_url, const std::string& default_desc);

		view * v;
		stfl::form * f;
		bool do_redraw;

	private:
		std::string prepare_keymap_hint(keymap_hint_entry * hints);
		void start_next_question();

		history cmdlinehistory;

		std::vector<std::pair<std::string, std::string> > qna_prompts;
		std::vector<std::string> qna_responses;
		operation finish_operation;
};


}



#endif

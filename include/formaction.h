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

typedef std::pair<std::string,std::string> qna_pair;

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

		void start_cmdline();


	protected:
		virtual void process_operation(operation op) = 0;
		virtual void set_keymap_hints();

		void start_qna(const std::vector<qna_pair>& prompts, operation finish_op, history * h = NULL);

		void start_bookmark_qna(const std::string& default_title, const std::string& default_url, const std::string& default_desc);

		view * v;
		stfl::form * f;
		bool do_redraw;

		std::vector<std::string> qna_responses;

		static history searchhistory;
		static history cmdlinehistory;

	private:
		std::string prepare_keymap_hint(keymap_hint_entry * hints);
		void start_next_question();

		std::vector<qna_pair> qna_prompts;
		operation finish_operation;
		history * qna_history;
};


}



#endif

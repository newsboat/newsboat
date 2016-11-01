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
		std::shared_ptr<stfl::form> get_form();
		virtual void set_redraw(bool b) {
			do_redraw = b;
		}

		virtual keymap_hint_entry * get_keymap_hint() = 0;

		virtual std::string id() const = 0;

		virtual std::string get_value(const std::string& value);

		virtual void handle_cmdline(const std::string& cmd);

		void process_op(operation op, bool automatic = false, std::vector<std::string> * args = nullptr);

		virtual void finished_qna(operation op);

		void start_cmdline();

		virtual void recalculate_form();

		inline std::string get_qna_response(unsigned int i) {
			return (qna_responses.size() >= (i + 1)) ? qna_responses[i] : "";
		}
		void start_qna(const std::vector<qna_pair>& prompts, operation finish_op, history * h = nullptr);

		inline void set_parent_formaction(std::shared_ptr<formaction> fa) {
			parent_formaction = fa;
		}
		inline std::shared_ptr<formaction> get_parent_formaction() const {
			return parent_formaction;
		}

		virtual std::string title() = 0;

		virtual std::vector<std::string> get_suggestions(const std::string& fragment);

		static void load_histories(const std::string& searchfile, const std::string& cmdlinefile);
		static void save_histories(const std::string& searchfile, const std::string& cmdlinefile, unsigned int limit);

	protected:
		virtual void process_operation(operation op, bool automatic = false, std::vector<std::string> * args = nullptr) = 0;
		virtual void set_keymap_hints();


		void start_bookmark_qna(const std::string& default_title, const std::string& default_url, const std::string& default_desc, const std::string& default_feed_title);
		void open_unread_items_in_browser(std::shared_ptr<rss_feed> feed , bool markread);

		view * v;
		std::shared_ptr<stfl::form> f;
		bool do_redraw;

		std::vector<std::string> qna_responses;

		static history searchhistory;
		static history cmdlinehistory;

		std::vector<std::string> valid_cmds;

	private:
		std::string prepare_keymap_hint(keymap_hint_entry * hints);
		void start_next_question();
		std::string make_title(const std::string& url);

		std::vector<qna_pair> qna_prompts;
		operation finish_operation;
		history * qna_history;
		std::shared_ptr<formaction> parent_formaction;
};


}



#endif

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

	protected:
		virtual void process_operation(operation op) = 0;
		virtual void set_keymap_hints();

		view * v;
		stfl::form * f;
		bool do_redraw;

		std::string bookmark_url;
		std::string bookmark_title;
		std::string bookmark_desc;

	private:
		std::string prepare_keymap_hint(keymap_hint_entry * hints);

		history cmdlinehistory;
};


}



#endif

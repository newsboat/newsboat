#ifndef NEWSBEUTER_FORMACTION__H
#define NEWSBEUTER_FORMACTION__H

#include <stflpp.h>
#include <keymap.h>
#include <rss.h>

#include <vector>
#include <string>

namespace newsbeuter {

class view;

class formaction {
	public:
		formaction(view *, std::string formstr);
		virtual ~formaction();
		virtual void process_operation(operation op) = 0;
		virtual void prepare() = 0;
		virtual void init() = 0;
		stfl::form& get_form();
		inline void set_redraw(bool b) { do_redraw = b; }

	protected:
		view * v;
		stfl::form * f;
		bool do_redraw;
};


}



#endif

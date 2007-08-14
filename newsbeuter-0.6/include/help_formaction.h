#ifndef NEWSBEUTER_HELP_FORMACTION__H
#define NEWSBEUTER_HELP_FORMACTION__H

#include <formaction.h>

namespace newsbeuter {

class help_formaction : public formaction {
	public:
		help_formaction(view *, std::string formstr);
		virtual ~help_formaction();
		virtual void prepare();
		virtual void init();
		keymap_hint_entry * get_keymap_hint();
	private:
		virtual void process_operation(operation op);
		bool quit;
};

}

#endif

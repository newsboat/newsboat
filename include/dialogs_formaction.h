#ifndef NEWSBEUTER_DIALOGS_FORMACTION__H
#define NEWSBEUTER_DIALOGS_FORMACTION__H

#include <formaction.h>

namespace newsbeuter {

class dialogs_formaction : public formaction {
	public:
		dialogs_formaction(view *, std::string formstr);
		virtual ~dialogs_formaction();
		virtual void prepare();
		virtual void init();
		virtual keymap_hint_entry * get_keymap_hint();
		virtual std::string id() const { return "dialogs"; }

	private:
		virtual void process_operation(operation op, bool automatic = false, std::vector<std::string> * args = NULL);
		bool update_list;
};

}

#endif

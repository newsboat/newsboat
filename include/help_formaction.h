#ifndef NEWSBEUTER_HELP_FORMACTION__H
#define NEWSBEUTER_HELP_FORMACTION__H

#include <formaction.h>

namespace newsboat {

class help_formaction : public formaction {
	public:
		help_formaction(view *, std::string formstr);
		virtual ~help_formaction();
		virtual void prepare();
		virtual void init();
		virtual keymap_hint_entry * get_keymap_hint();
		virtual std::string id() const {
			return "help";
		}
		virtual std::string title();

		virtual void finished_qna(operation op);
		void set_context(const std::string& ctx);
	private:
		virtual void process_operation(operation op, bool automatic = false, std::vector<std::string> * args = nullptr);
		std::string make_colorstring(const std::vector<std::string>& colors);
		bool quit;
		bool apply_search;
		std::string searchphrase;
		std::string context;

};

}

#endif

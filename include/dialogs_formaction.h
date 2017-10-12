#ifndef NEWSBEUTER_DIALOGS_FORMACTION__H
#define NEWSBEUTER_DIALOGS_FORMACTION__H

#include <formaction.h>

namespace newsbeuter {

class dialogs_formaction : public formaction {
	public:
		dialogs_formaction(view *, std::string formstr);
		~dialogs_formaction() override;
		void prepare() override;
		void init() override;
		keymap_hint_entry * get_keymap_hint() override;
		std::string id() const override {
			return "dialogs";
		}
		std::string title() override;
		void handle_cmdline(const std::string& cmd) override;

	private:
		void process_operation(operation op, bool automatic = false, std::vector<std::string> * args = nullptr) override;
		bool update_list;
};

}

#endif

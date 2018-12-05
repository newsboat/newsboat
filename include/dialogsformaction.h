#ifndef NEWSBOAT_DIALOGSFORMACTION_H_
#define NEWSBOAT_DIALOGSFORMACTION_H_

#include "formaction.h"

namespace newsboat {

class DialogsFormAction : public FormAction {
public:
	DialogsFormAction(View*, std::string formstr, ConfigContainer* cfg);
	~DialogsFormAction() override;
	void prepare() override;
	void init() override;
	KeyMapHintEntry* get_keymap_hint() override;
	std::string id() const override
	{
		return "dialogs";
	}
	std::string title() override;
	void handle_cmdline(const std::string& cmd) override;

private:
	void process_operation(Operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr) override;
	bool update_list;
};

} // namespace newsboat

#endif /* NEWSBOAT_DIALOGSFORMACTION_H_ */

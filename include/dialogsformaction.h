#ifndef NEWSBOAT_DIALOGSFORMACTION_H_
#define NEWSBOAT_DIALOGSFORMACTION_H_

#include "formaction.h"
#include "listwidget.h"
#include "utf8string.h"

namespace newsboat {

class DialogsFormAction : public FormAction {
public:
	DialogsFormAction(View*, Utf8String formstr, ConfigContainer* cfg);
	~DialogsFormAction() override;
	void prepare() override;
	void init() override;
	const std::vector<KeyMapHintEntry>& get_keymap_hint() const override;
	Utf8String id() const override
	{
		return "dialogs";
	}
	Utf8String title() override;
	void handle_cmdline(const Utf8String& cmd) override;

private:
	bool process_operation(Operation op,
		bool automatic = false,
		std::vector<Utf8String>* args = nullptr) override;
	void update_heading();

	ListWidget dialogs_list;
};

} // namespace newsboat

#endif /* NEWSBOAT_DIALOGSFORMACTION_H_ */

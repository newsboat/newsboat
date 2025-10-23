#ifndef NEWSBOAT_DIALOGSFORMACTION_H_
#define NEWSBOAT_DIALOGSFORMACTION_H_

#include "listformaction.h"
#include "regexmanager.h"

namespace newsboat {

class DialogsFormAction : public ListFormAction {
public:
	DialogsFormAction(View&, std::string formstr, ConfigContainer* cfg, RegexManager& r);
	~DialogsFormAction() override = default;
	void prepare() override;
	void init() override;
	std::vector<KeyMapHintEntry> get_keymap_hint() const override;
	Dialog id() const override
	{
		return Dialog::DialogList;
	}
	std::string title() override;
	void handle_cmdline(const std::string& cmd) override;

protected:
	std::string main_widget() const override
	{
		return "dialogs";
	}

private:
	bool process_operation(Operation op,
		const std::vector<std::string>& args,
		BindingType bindingType = BindingType::BindKey) override;
	void update_heading();
};

} // namespace newsboat

#endif /* NEWSBOAT_DIALOGSFORMACTION_H_ */

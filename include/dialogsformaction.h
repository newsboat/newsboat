#ifndef NEWSBOAT_DIALOGSFORMACTION_H_
#define NEWSBOAT_DIALOGSFORMACTION_H_

#include "listformaction.h"
#include "regexmanager.h"

namespace Newsboat {

class DialogsFormAction : public ListFormAction {
public:
	DialogsFormAction(View&, std::string formstr, ConfigContainer* cfg, RegexManager& r);
	~DialogsFormAction() override = default;
	void prepare() override;
	void init() override;
	std::vector<KeyMapHintEntry> get_keymap_hint() const override;
	std::string id() const override
	{
		return "dialogs";
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

} // namespace Newsboat

#endif /* NEWSBOAT_DIALOGSFORMACTION_H_ */

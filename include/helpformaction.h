#ifndef NEWSBOAT_HELPFORMACTION_H_
#define NEWSBOAT_HELPFORMACTION_H_

#include "dialog.h"
#include "formaction.h"
#include "textviewwidget.h"

namespace newsboat {

class HelpFormAction : public FormAction {
public:
	HelpFormAction(View&, std::string formstr, ConfigContainer* cfg, Dialog ctx);
	~HelpFormAction() override = default;
	void prepare() override;
	void init() override;
	std::vector<KeyMapHintEntry> get_keymap_hint() const override;
	Dialog id() const override
	{
		return Dialog::Help;
	}
	std::string title() override;

	void finished_qna(QnaFinishAction op) override;

protected:
	std::string main_widget() const override
	{
		return "helptext";
	}

private:
	bool process_operation(Operation op,
		const std::vector<std::string>& args,
		BindingType bindingType = BindingType::BindKey) override;
	std::string make_colorstring(const std::vector<std::string>& colors);
	bool apply_search;
	std::string searchphrase;
	const Dialog context;
	TextviewWidget textview;
};

} // namespace newsboat

#endif /* NEWSBOAT_HELPFORMACTION_H_ */

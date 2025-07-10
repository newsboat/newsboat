#ifndef NEWSBOAT_HELPFORMACTION_H_
#define NEWSBOAT_HELPFORMACTION_H_

#include "formaction.h"
#include "textviewwidget.h"

namespace Newsboat {

class HelpFormAction : public FormAction {
public:
	HelpFormAction(View&, std::string formstr, ConfigContainer* cfg,
		const std::string& ctx);
	~HelpFormAction() override = default;
	void prepare() override;
	void init() override;
	std::vector<KeyMapHintEntry> get_keymap_hint() const override;
	std::string id() const override
	{
		return "help";
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
	const std::string context;
	TextviewWidget textview;
};

} // namespace Newsboat

#endif /* NEWSBOAT_HELPFORMACTION_H_ */

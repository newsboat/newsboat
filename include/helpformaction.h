#ifndef NEWSBOAT_HELPFORMACTION_H_
#define NEWSBOAT_HELPFORMACTION_H_

#include "formaction.h"
#include "textviewwidget.h"

namespace newsboat {

class HelpFormAction : public FormAction {
public:
	HelpFormAction(View*, std::string formstr, ConfigContainer* cfg,
		const std::string& ctx);
	~HelpFormAction() override;
	void prepare() override;
	void init() override;
	const std::vector<KeyMapHintEntry>& get_keymap_hint() const override;
	std::string id() const override
	{
		return "help";
	}
	std::string title() override;

	void finished_qna(Operation op) override;

protected:
	std::string main_widget() const override
	{
		return "helptext";
	}

private:
	bool process_operation(Operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr) override;
	std::string make_colorstring(const std::vector<std::string>& colors);
	bool quit;
	bool apply_search;
	std::string searchphrase;
	const std::string context;
	TextviewWidget textview;
};

} // namespace newsboat

#endif /* NEWSBOAT_HELPFORMACTION_H_ */

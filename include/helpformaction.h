#ifndef NEWSBOAT_HELPFORMACTION_H_
#define NEWSBOAT_HELPFORMACTION_H_

#include "formaction.h"
#include "textviewwidget.h"
#include "utf8string.h"

namespace newsboat {

class HelpFormAction : public FormAction {
public:
	HelpFormAction(View*, std::string formstr, ConfigContainer* cfg,
		const std::string& ctx);
	~HelpFormAction() override;
	void prepare() override;
	void init() override;
	KeyMapHintEntry* get_keymap_hint() override;
	std::string id() const override
	{
		return "help";
	}
	std::string title() override;

	void finished_qna(Operation op) override;

private:
	bool process_operation(Operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr) override;
	std::string make_colorstring(const std::vector<std::string>& colors);
	bool quit;
	bool apply_search;
	Utf8String searchphrase;
	const Utf8String context;
	TextviewWidget textview;
};

} // namespace newsboat

#endif /* NEWSBOAT_HELPFORMACTION_H_ */

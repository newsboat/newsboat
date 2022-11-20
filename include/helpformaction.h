#ifndef NEWSBOAT_HELPFORMACTION_H_
#define NEWSBOAT_HELPFORMACTION_H_

#include "formaction.h"
#include "textviewwidget.h"
#include "utf8string.h"

namespace newsboat {

class HelpFormAction : public FormAction {
public:
	HelpFormAction(View*, Utf8String formstr, ConfigContainer* cfg,
		const Utf8String& ctx);
	~HelpFormAction() override;
	void prepare() override;
	void init() override;
	const std::vector<KeyMapHintEntry>& get_keymap_hint() const override;
	Utf8String id() const override
	{
		return "help";
	}
	Utf8String title() override;

	void finished_qna(Operation op) override;

private:
	bool process_operation(Operation op,
		bool automatic = false,
		std::vector<Utf8String>* args = nullptr) override;
	Utf8String make_colorstring(const std::vector<Utf8String>& colors);
	bool quit;
	bool apply_search;
	Utf8String searchphrase;
	const Utf8String context;
	TextviewWidget textview;
};

} // namespace newsboat

#endif /* NEWSBOAT_HELPFORMACTION_H_ */

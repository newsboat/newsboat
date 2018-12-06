#ifndef NEWSBOAT_HELPFORMACTION_H_
#define NEWSBOAT_HELPFORMACTION_H_

#include "formaction.h"

namespace newsboat {

class HelpFormAction : public FormAction {
public:
	HelpFormAction(View*, std::string formstr, ConfigContainer* cfg);
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
	void set_context(const std::string& ctx);

private:
	void process_operation(Operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr) override;
	std::string make_colorstring(const std::vector<std::string>& colors);
	bool quit;
	bool apply_search;
	std::string searchphrase;
	std::string context;
};

} // namespace newsboat

#endif /* NEWSBOAT_HELPFORMACTION_H_ */

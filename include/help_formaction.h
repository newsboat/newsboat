#ifndef NEWSBOAT_HELP_FORMACTION_H_
#define NEWSBOAT_HELP_FORMACTION_H_

#include "formaction.h"

namespace newsboat {

class help_formaction : public formaction {
public:
	help_formaction(view*, std::string formstr);
	~help_formaction() override;
	void prepare() override;
	void init() override;
	keymap_hint_entry* get_keymap_hint() override;
	std::string id() const override
	{
		return "help";
	}
	std::string title() override;

	void finished_qna(operation op) override;
	void set_context(const std::string& ctx);

private:
	void process_operation(operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr) override;
	std::string make_colorstring(const std::vector<std::string>& colors);
	bool quit;
	bool apply_search;
	std::string searchphrase;
	std::string context;
};

} // namespace newsboat

#endif /* NEWSBOAT_HELP_FORMACTION_H_ */

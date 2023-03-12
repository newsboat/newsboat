#ifndef NEWSBOAT_EMPTYFORMACTION_H_
#define NEWSBOAT_EMPTYFORMACTION_H_

#include "formaction.h"

namespace newsboat {

class EmptyFormAction : public FormAction {
public:
	EmptyFormAction(View* v, const std::string& formstr, ConfigContainer* cfg);
	virtual ~EmptyFormAction() = default;

	std::string id() const override;
	std::string title() override;

	void init() override;
	void prepare() override;

	const std::vector<KeyMapHintEntry>& get_keymap_hint() const override;

protected:
	bool process_operation(Operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr) override;
	std::string main_widget() const override
	{
		return "lastline";
	}
};

} // namespace newsboat

#endif /* NEWSBOAT_EMPTYFORMACTION_H_ */

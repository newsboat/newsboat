#ifndef NEWSBOAT_EMPTYFORMACTION_H_
#define NEWSBOAT_EMPTYFORMACTION_H_

#include "formaction.h"
#include "utf8string.h"

namespace newsboat {

class EmptyFormAction : public FormAction {
public:
	EmptyFormAction(View* v, const Utf8String& formstr, ConfigContainer* cfg);
	virtual ~EmptyFormAction() = default;

	Utf8String id() const override;
	Utf8String title() override;

	void init() override;
	void prepare() override;

	const std::vector<KeyMapHintEntry>& get_keymap_hint() const override;

protected:
	bool process_operation(Operation op,
		bool automatic = false,
		std::vector<Utf8String>* args = nullptr) override;
};

} // namespace newsboat

#endif /* NEWSBOAT_EMPTYFORMACTION_H_ */

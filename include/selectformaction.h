#ifndef NEWSBOAT_SELECTFORMACTION_H_
#define NEWSBOAT_SELECTFORMACTION_H_

#include "filtercontainer.h"
#include "formaction.h"

namespace newsboat {

class SelectFormAction : public FormAction {
public:
	enum class SelectionType { TAG, FILTER };

	SelectFormAction(View*, std::string formstr, ConfigContainer* cfg);
	~SelectFormAction() override;
	void prepare() override;
	void init() override;
	KeyMapHintEntry* get_keymap_hint() override;
	std::string get_selected_value()
	{
		return value;
	}
	void set_tags(const std::vector<std::string>& t)
	{
		tags = t;
	}
	void set_filters(const std::vector<FilterNameExprPair>& ff)
	{
		filters = ff;
	}
	void set_type(SelectionType t)
	{
		type = t;
	}
	void handle_cmdline(const std::string& cmd) override;
	std::string id() const override
	{
		return (type == SelectionType::TAG) ? "tagselection"
						     : "filterselection";
	}
	std::string title() override;

private:
	void process_operation(Operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr) override;
	bool quit;
	SelectionType type;
	std::string value;
	std::vector<std::string> tags;
	std::vector<FilterNameExprPair> filters;
};

} // namespace newsboat

#endif /* NEWSBOAT_SELECTFORMACTION_H_ */

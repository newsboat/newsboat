#ifndef NEWSBOAT_SELECTFORMACTION_H_
#define NEWSBOAT_SELECTFORMACTION_H_

#include "filtercontainer.h"
#include "formaction.h"

namespace newsboat {

class SelectFormaction : public Formaction {
public:
	enum class SelectionType { TAG, FILTER };

	SelectFormaction(View*, std::string formstr);
	~SelectFormaction() override;
	void prepare() override;
	void init() override;
	keymap_hint_entry* get_keymap_hint() override;
	std::string get_selected_value()
	{
		return value;
	}
	void set_tags(const std::vector<std::string>& t)
	{
		tags = t;
	}
	void set_filters(const std::vector<filter_name_expr_pair>& ff)
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
	void process_operation(operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr) override;
	bool quit;
	SelectionType type;
	std::string value;
	std::vector<std::string> tags;
	std::vector<filter_name_expr_pair> filters;
};

} // namespace newsboat

#endif /* NEWSBOAT_SELECTFORMACTION_H_ */

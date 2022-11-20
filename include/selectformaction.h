#ifndef NEWSBOAT_SELECTFORMACTION_H_
#define NEWSBOAT_SELECTFORMACTION_H_

#include "filtercontainer.h"
#include "formaction.h"
#include "listwidget.h"
#include "utf8string.h"

namespace newsboat {

class SelectFormAction : public FormAction {
public:
	enum class SelectionType { TAG, FILTER };

	SelectFormAction(View*, Utf8String formstr, ConfigContainer* cfg);
	~SelectFormAction() override;
	void prepare() override;
	void init() override;
	const std::vector<KeyMapHintEntry>& get_keymap_hint() const override;
	void set_selected_value(const Utf8String& new_value)
	{
		value = new_value;
	}
	Utf8String get_selected_value()
	{
		return value;
	}
	void set_tags(const std::vector<Utf8String>& t)
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
	void handle_cmdline(const Utf8String& cmd) override;
	Utf8String id() const override
	{
		if (type == SelectionType::TAG) {
			return "tagselection";
		} else {
			return "filterselection";
		}
	}
	Utf8String title() override;

private:
	bool process_operation(Operation op,
		bool automatic = false,
		std::vector<Utf8String>* args = nullptr) override;
	bool quit;
	bool is_first_draw;
	SelectionType type;
	Utf8String value;
	std::vector<Utf8String> tags;
	std::vector<FilterNameExprPair> filters;

	Utf8String format_line(const Utf8String& selecttag_format,
		const Utf8String& tag,
		unsigned int pos,
		unsigned int width);
	void update_heading();

	ListWidget tags_list;
};

} // namespace newsboat

#endif /* NEWSBOAT_SELECTFORMACTION_H_ */

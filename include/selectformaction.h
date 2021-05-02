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

	SelectFormAction(View*, std::string formstr, ConfigContainer* cfg);
	~SelectFormAction() override;
	void prepare() override;
	void init() override;
	KeyMapHintEntry* get_keymap_hint() override;
	std::string get_selected_value()
	{
		return value.to_utf8();
	}
	void set_tags(const std::vector<std::string>& t)
	{
		tags.clear();
		for (const auto& tag : t) {
			tags.push_back(Utf8String::from_utf8(tag));
		}
	}
	void set_filters(const std::vector<FilterNameExprPair>& ff)
	{
		filters.clear();
		for (const auto& filter : ff) {
			InternalFilterNameExprPair entry;
			entry.name = Utf8String::from_utf8(filter.name);
			entry.expr = Utf8String::from_utf8(filter.expr);
			filters.push_back(entry);
		}
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
	bool process_operation(Operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr) override;
	bool quit;
	SelectionType type;
	Utf8String value;
	std::vector<Utf8String> tags;
	std::vector<InternalFilterNameExprPair> filters;

	std::string format_line(const std::string& selecttag_format,
		const std::string& tag,
		unsigned int pos,
		unsigned int width);
	void update_heading();

	ListWidget tags_list;
};

} // namespace newsboat

#endif /* NEWSBOAT_SELECTFORMACTION_H_ */

#ifndef NEWSBOAT_URLVIEWFORMACTION_H_
#define NEWSBOAT_URLVIEWFORMACTION_H_

#include "formaction.h"
#include "links.h"
#include "listwidget.h"

namespace Newsboat {

class UrlViewFormAction : public FormAction {
public:
	UrlViewFormAction(View&,
		std::shared_ptr<RssFeed>& feed,
		std::string formstr,
		ConfigContainer* cfg);
	~UrlViewFormAction() override = default;
	void prepare() override;
	void init() override;
	std::vector<KeyMapHintEntry> get_keymap_hint() const override;
	void set_links(const Links& l)
	{
		links = l;
	}
	std::string id() const override
	{
		return "urlview";
	}
	std::string title() override;
	void handle_cmdline(const std::string& cmd) override;

protected:
	std::string main_widget() const override
	{
		return "urls";
	}

private:
	bool process_operation(Operation op,
		const std::vector<std::string>& args,
		BindingType bindingType = BindingType::BindKey) override;
	void open_current_position_in_browser(bool interactive);
	void update_heading();

	Links links;
	std::shared_ptr<RssFeed> feed;
	ListWidget urls_list;
};

} // namespace Newsboat

#endif /* NEWSBOAT_URLVIEWFORMACTION_H_ */

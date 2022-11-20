#ifndef NEWSBOAT_URLVIEWFORMACTION_H_
#define NEWSBOAT_URLVIEWFORMACTION_H_

#include "formaction.h"
#include "htmlrenderer.h"
#include "listwidget.h"
#include "utf8string.h"

namespace newsboat {

class UrlViewFormAction : public FormAction {
public:
	UrlViewFormAction(View*,
		std::shared_ptr<RssFeed>& feed,
		Utf8String formstr,
		ConfigContainer* cfg);
	~UrlViewFormAction() override;
	void prepare() override;
	void init() override;
	const std::vector<KeyMapHintEntry>& get_keymap_hint() const override;
	void set_links(const std::vector<LinkPair>& l)
	{
		links = l;
	}
	Utf8String id() const override
	{
		return "urlview";
	}
	Utf8String title() override;
	void handle_cmdline(const Utf8String& cmd) override;

private:
	bool process_operation(Operation op,
		bool automatic = false,
		std::vector<Utf8String>* args = nullptr) override;
	void open_current_position_in_browser(bool interactive);
	void update_heading();

	std::vector<LinkPair> links;
	bool quit;
	std::shared_ptr<RssFeed> feed;
	ListWidget urls_list;
};

} // namespace newsboat

#endif /* NEWSBOAT_URLVIEWFORMACTION_H_ */

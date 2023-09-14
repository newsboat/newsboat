#ifndef NEWSBOAT_ITEMVIEWFORMACTION_H_
#define NEWSBOAT_ITEMVIEWFORMACTION_H_

#include "formaction.h"
#include "links.h"
#include "regexmanager.h"
#include "textformatter.h"
#include "textviewwidget.h"
#include "filepath.h"

namespace newsboat {

class Cache;
class ItemListFormAction;
class RssItem;

class ItemViewFormAction : public FormAction {
public:
	ItemViewFormAction(View&,
		std::shared_ptr<ItemListFormAction> il,
		std::string formstr,
		Cache* cc,
		ConfigContainer* cfg,
		RegexManager& r);
	~ItemViewFormAction() override = default;
	void prepare() override;
	void init() override;
	void set_guid(const std::string& guid_)
	{
		guid = guid_;
	}
	void set_feed(std::shared_ptr<RssFeed> fd)
	{
		feed = fd;
	}
	void set_highlightphrase(const std::string& text);
	std::vector<KeyMapHintEntry> get_keymap_hint() const override;
	void handle_cmdline(const std::string& cmd) override;

	std::string id() const override
	{
		return "article";
	}
	std::string title() override;

	void finished_qna(QnaFinishAction op) override;

	void render_html(
		const std::string& source,
		std::vector<std::pair<LineType, std::string>>& lines,
		Links& links,
		const std::string& url);

	void update_percent();

protected:
	std::string main_widget() const override
	{
		return "article";
	}

private:
	void register_format_styles();

	bool process_operation(Operation op,
		const std::vector<std::string>& args,
		BindingType bindingType = BindingType::BindKey) override;

	bool open_link_in_browser(const std::string& link, const std::string& type,
		const std::string& title, bool interactive) const;

	void update_head(RssItem& item);
	void set_head(const std::string& s,
		const std::string& feedtitle,
		unsigned int unread,
		unsigned int total);
	void highlight_text(const std::string& searchphrase);

	void render_source(std::vector<std::pair<LineType, std::string>>& lines,
		std::string source);

	void do_search();
	void handle_save(const Filepath& filename_param);

	std::string guid;
	std::shared_ptr<RssFeed> feed;
	std::shared_ptr<RssItem> item;
	bool show_source;
	Links links;
	RegexManager& rxman;
	unsigned int num_lines;
	std::shared_ptr<ItemListFormAction> itemlist;
	bool in_search;
	Cache* rsscache;
	TextviewWidget textview;
};

} // namespace newsboat

#endif /* NEWSBOAT_ITEMVIEWFORMACTION_H_ */

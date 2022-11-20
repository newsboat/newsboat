#ifndef NEWSBOAT_ITEMVIEWFORMACTION_H_
#define NEWSBOAT_ITEMVIEWFORMACTION_H_

#include "formaction.h"
#include "htmlrenderer.h"
#include "regexmanager.h"
#include "textformatter.h"
#include "textviewwidget.h"
#include "utf8string.h"

namespace newsboat {

class Cache;
class ItemListFormAction;
class RssItem;

class ItemViewFormAction : public FormAction {
public:
	ItemViewFormAction(View*,
		std::shared_ptr<ItemListFormAction> il,
		Utf8String formstr,
		Cache* cc,
		ConfigContainer* cfg,
		RegexManager& r);
	~ItemViewFormAction() override;
	void prepare() override;
	void init() override;
	void set_guid(const Utf8String& guid_)
	{
		guid = guid_;
	}
	void set_feed(std::shared_ptr<RssFeed> fd)
	{
		feed = fd;
	}
	void set_highlightphrase(const Utf8String& text);
	const std::vector<KeyMapHintEntry>& get_keymap_hint() const override;
	void handle_cmdline(const Utf8String& cmd) override;

	Utf8String id() const override
	{
		return "article";
	}
	Utf8String title() override;

	void finished_qna(Operation op) override;

	void render_html(
		const Utf8String& source,
		std::vector<std::pair<LineType, Utf8String>>& lines,
		std::vector<LinkPair>& thelinks,
		const Utf8String& url);

	void update_percent();

private:
	void register_format_styles();

	bool process_operation(Operation op,
		bool automatic = false,
		std::vector<Utf8String>* args = nullptr) override;

	bool open_link_in_browser(const Utf8String& link, const Utf8String& type,
		bool interactive) const;

	void update_head(const std::shared_ptr<RssItem>& item);
	void set_head(const Utf8String& s,
		const Utf8String& feedtitle,
		unsigned int unread,
		unsigned int total);
	void highlight_text(const Utf8String& searchphrase);

	void render_source(std::vector<std::pair<LineType, Utf8String>>& lines,
		Utf8String source);

	void do_search();
	void handle_save(const Utf8String& filename_param);

	Utf8String guid;
	std::shared_ptr<RssFeed> feed;
	std::shared_ptr<RssItem> item;
	bool show_source;
	std::vector<LinkPair> links;
	RegexManager& rxman;
	unsigned int num_lines;
	std::shared_ptr<ItemListFormAction> itemlist;
	bool in_search;
	Cache* rsscache;
	TextviewWidget textview;
};

} // namespace newsboat

#endif /* NEWSBOAT_ITEMVIEWFORMACTION_H_ */

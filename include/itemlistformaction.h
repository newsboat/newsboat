#ifndef NEWSBOAT_ITEMLISTFORMACTION_H_
#define NEWSBOAT_ITEMLISTFORMACTION_H_

#include <assert.h>

#include "3rd-party/optional.hpp"

#include "configcontainer.h"
#include "fmtstrformatter.h"
#include "history.h"
#include "listformaction.h"
#include "listformatter.h"
#include "regexmanager.h"
#include "view.h"
#include "utf8string.h"

namespace newsboat {

class RssItem;

typedef std::pair<std::shared_ptr<RssItem>, unsigned int> ItemPtrPosPair;

enum class InvalidationMode { NONE, PARTIAL, COMPLETE };

class ItemListFormAction : public ListFormAction {
public:
	ItemListFormAction(View*,
		Utf8String formstr,
		Cache* cc,
		FilterContainer& f,
		ConfigContainer* cfg,
		RegexManager& r);
	~ItemListFormAction() override;
	void prepare() override;
	void init() override;

	void set_feed(std::shared_ptr<RssFeed> fd);

	Utf8String id() const override
	{
		return "articlelist";
	}
	Utf8String title() override;

	std::shared_ptr<RssFeed> get_feed()
	{
		return feed;
	}
	void set_pos(unsigned int p)
	{
		pos = p;
	}
	Utf8String get_guid();
	const std::vector<KeyMapHintEntry>& get_keymap_hint() const override;

	bool jump_to_next_unread_item(bool start_with_first);
	bool jump_to_previous_unread_item(bool start_with_last);
	bool jump_to_next_item(bool start_with_first);
	bool jump_to_previous_item(bool start_with_last);
	bool jump_to_random_unread_item();

	void handle_cmdline(const Utf8String& cmd) override;

	void finished_qna(Operation op) override;

	void invalidate_list()
	{
		invalidation_mode = InvalidationMode::COMPLETE;
	}

	void restore_selected_position();

protected:
	bool process_operation(Operation op,
		bool automatic = false,
		std::vector<Utf8String>* args = nullptr) override;

	void invalidate(const unsigned int invalidated_pos)
	{
		if (invalidation_mode == InvalidationMode::COMPLETE) {
			return;
		}

		invalidation_mode = InvalidationMode::PARTIAL;
		invalidated_itempos.push_back(invalidated_pos);
	}

	virtual FmtStrFormatter setup_head_formatter(const Utf8String& s,
		unsigned int unread,
		unsigned int total,
		const Utf8String& url);

	std::vector<ItemPtrPosPair> visible_items;
	int old_itempos;
	std::shared_ptr<RssFeed> feed;

	Matcher matcher;
	bool filter_active;
	void apply_filter(const Utf8String& filter_query);

private:
	void register_format_styles();

	void do_update_visible_items();
	void draw_items();

	bool open_position_in_browser(unsigned int pos,
		bool interactive) const;

	virtual void set_head(const Utf8String& s,
		unsigned int unread,
		unsigned int total,
		const Utf8String& url);

	int get_pos(unsigned int idx);

	void save_article(const Utf8String& filename,
		std::shared_ptr<RssItem> item);

	void handle_save(const std::vector<Utf8String>& cmd_args);

	void save_filterpos();

	void qna_end_setfilter();
	void qna_end_editflags();
	void qna_start_search();

	void handle_cmdline_num(unsigned int idx);

	Utf8String gen_flags(std::shared_ptr<RssItem> item);

	void prepare_set_filterpos();

	Utf8String item2formatted_line(const ItemPtrPosPair& item,
		const unsigned int width,
		const Utf8String& itemlist_format,
		const Utf8String& datetime_format);

	void goto_item(const Utf8String& title);

	void handle_op_saveall();

	unsigned int pos;

	History filterhistory;

	std::mutex redraw_mtx;

	bool set_filterpos;
	unsigned int filterpos;

	RegexManager& rxman;

	unsigned int old_width;
	nonstd::optional<ArticleSortStrategy> old_sort_strategy;

	InvalidationMode invalidation_mode;
	std::vector<unsigned int> invalidated_itempos;

	ListFormatter listfmt;
	Cache* rsscache;
	FilterContainer& filter_container;
};

} // namespace newsboat

#endif /* NEWSBOAT_ITEMLISTFORMACTION_H_ */

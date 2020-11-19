#ifndef NEWSBOAT_ITEMLISTFORMACTION_H_
#define NEWSBOAT_ITEMLISTFORMACTION_H_

#include <assert.h>

#include "3rd-party/optional.hpp"

#include "history.h"
#include "listformaction.h"
#include "listformatter.h"
#include "regexmanager.h"
#include "view.h"

namespace newsboat {

typedef std::pair<std::shared_ptr<RssItem>, unsigned int> ItemPtrPosPair;

enum class InvalidationMode { NONE, PARTIAL, COMPLETE };

class ItemListFormAction : public ListFormAction {
public:
	ItemListFormAction(View*,
		std::string formstr,
		Cache* cc,
		FilterContainer& f,
		ConfigContainer* cfg,
		RegexManager& r);
	~ItemListFormAction() override;
	void prepare() override;
	void init() override;

	void set_feed(std::shared_ptr<RssFeed> fd);

	std::string id() const override
	{
		return "articlelist";
	}
	std::string title() override;

	std::shared_ptr<RssFeed> get_feed()
	{
		return feed;
	}
	void set_pos(unsigned int p)
	{
		pos = p;
	}
	std::string get_guid();
	KeyMapHintEntry* get_keymap_hint() override;

	bool jump_to_next_unread_item(bool start_with_first);
	bool jump_to_previous_unread_item(bool start_with_last);
	bool jump_to_next_item(bool start_with_first);
	bool jump_to_previous_item(bool start_with_last);
	bool jump_to_random_unread_item();

	void handle_cmdline(const std::string& cmd) override;

	void finished_qna(Operation op) override;

	void set_show_searchresult(bool b)
	{
		show_searchresult = b;
	}
	void set_searchphrase(const std::string& s)
	{
		search_phrase = s;
	}

	void invalidate_list()
	{
		invalidation_mode = InvalidationMode::COMPLETE;
	}

	void restore_selected_position();

private:
	void register_format_styles();

	void do_update_visible_items();
	void draw_items();

	bool process_operation(Operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr) override;
	void set_head(const std::string& s,
		unsigned int unread,
		unsigned int total,
		const std::string& url);
	int get_pos(unsigned int idx);

	void save_article(const std::string& filename,
		std::shared_ptr<RssItem> item);

	void save_filterpos();

	void qna_end_setfilter();
	void qna_end_editflags();
	void qna_start_search();

	void handle_cmdline_num(unsigned int idx);

	std::string gen_flags(std::shared_ptr<RssItem> item);

	void prepare_set_filterpos();

	void invalidate(const unsigned int invalidated_pos)
	{
		if (invalidation_mode == InvalidationMode::COMPLETE) {
			return;
		}

		invalidation_mode = InvalidationMode::PARTIAL;
		invalidated_itempos.push_back(invalidated_pos);
	}

	std::string item2formatted_line(const ItemPtrPosPair& item,
		const unsigned int width,
		const std::string& itemlist_format,
		const std::string& datetime_format);

	void goto_item(const std::string& title);

	unsigned int pos;
	std::shared_ptr<RssFeed> feed;
	bool apply_filter;
	Matcher matcher;
	std::vector<ItemPtrPosPair> visible_items;
	bool show_searchresult;
	std::string search_phrase;

	History filterhistory;

	std::mutex redraw_mtx;

	bool set_filterpos;
	unsigned int filterpos;

	RegexManager& rxman;

	unsigned int old_width;
	int old_itempos;
	nonstd::optional<ArticleSortStrategy> old_sort_strategy;

	InvalidationMode invalidation_mode;
	std::vector<unsigned int> invalidated_itempos;

	ListFormatter listfmt;
	Cache* rsscache;
	FilterContainer& filters;

	void handle_op_saveall();
};

} // namespace newsboat

#endif /* NEWSBOAT_ITEMLISTFORMACTION_H_ */

#ifndef NEWSBOAT_ITEMLISTFORMACTION_H_
#define NEWSBOAT_ITEMLISTFORMACTION_H_

#include <assert.h>

#include "history.h"
#include "listformaction.h"
#include "listformatter.h"
#include "regexmanager.h"
#include "view.h"

namespace newsboat {

typedef std::pair<std::shared_ptr<rss_item>, unsigned int> itemptr_pos_pair;

enum class InvalidationMode { PARTIAL, COMPLETE };

class itemlist_formaction : public list_formaction {
public:
	itemlist_formaction(view*, std::string formstr);
	~itemlist_formaction() override;
	void prepare() override;
	void init() override;

	void set_redraw(bool b) override
	{
		formaction::set_redraw(b);
		apply_filter = !(v->get_cfg()->get_configvalue_as_bool(
			"show-read-articles"));
		invalidate(InvalidationMode::COMPLETE);
	}

	void set_feed(std::shared_ptr<rss_feed> fd);

	std::string id() const override
	{
		return "articlelist";
	}
	std::string title() override;

	std::shared_ptr<rss_feed> get_feed()
	{
		return feed;
	}
	void set_pos(unsigned int p)
	{
		pos = p;
	}
	std::string get_guid();
	keymap_hint_entry* get_keymap_hint() override;

	bool jump_to_next_unread_item(bool start_with_first);
	bool jump_to_previous_unread_item(bool start_with_last);
	bool jump_to_next_item(bool start_with_first);
	bool jump_to_previous_item(bool start_with_last);
	bool jump_to_random_unread_item();

	void handle_cmdline(const std::string& cmd) override;

	void do_update_visible_items();

	void finished_qna(operation op) override;

	void set_show_searchresult(bool b)
	{
		show_searchresult = b;
	}
	void set_searchphrase(const std::string& s)
	{
		searchphrase = s;
	}

	void recalculate_form() override;

	void set_regexmanager(regexmanager* r);

private:
	void process_operation(operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr) override;
	void set_head(const std::string& s,
		unsigned int unread,
		unsigned int total,
		const std::string& url);
	int get_pos(unsigned int idx);

	void save_article(const std::string& filename,
		std::shared_ptr<rss_item> item);

	void save_filterpos();

	void qna_end_setfilter();
	void qna_end_editflags();
	void qna_start_search();

	void handle_cmdline_num(unsigned int idx);

	std::string gen_flags(std::shared_ptr<rss_item> item);
	std::string gen_datestr(time_t t, const std::string& datetimeformat);

	void prepare_set_filterpos();

	void invalidate(InvalidationMode m)
	{
		assert(m == InvalidationMode::COMPLETE);

		invalidated = true;
		invalidation_mode = InvalidationMode::COMPLETE;
	}

	void invalidate(const unsigned int pos)
	{
		if (invalidated == true &&
			invalidation_mode == InvalidationMode::COMPLETE)
			return;

		invalidated = true;
		invalidation_mode = InvalidationMode::PARTIAL;
		invalidated_itempos.push_back(pos);
	}

	std::string item2formatted_line(const itemptr_pos_pair& item,
		const unsigned int width,
		const std::string& itemlist_format,
		const std::string& datetime_format);

	unsigned int pos;
	std::shared_ptr<rss_feed> feed;
	bool apply_filter;
	matcher m;
	std::vector<itemptr_pos_pair> visible_items;
	bool show_searchresult;
	std::string searchphrase;

	history filterhistory;

	std::shared_ptr<rss_feed> search_dummy_feed;

	std::mutex redraw_mtx;

	bool set_filterpos;
	unsigned int filterpos;

	regexmanager* rxman;

	unsigned int old_width;
	int old_itempos;
	ArticleSortStrategy old_sort_strategy;

	bool invalidated;
	InvalidationMode invalidation_mode;
	std::vector<unsigned int> invalidated_itempos;

	listformatter listfmt;
};

} // namespace newsboat

#endif /* NEWSBOAT_ITEMLISTFORMACTION_H_ */

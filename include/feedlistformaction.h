#ifndef NEWSBOAT_FEEDLISTFORMACTION_H_
#define NEWSBOAT_FEEDLISTFORMACTION_H_

#include "history.h"
#include "listformaction.h"
#include "listwidget.h"
#include "matcher.h"
#include "regexmanager.h"
#include "view.h"

namespace newsboat {

typedef std::pair<std::shared_ptr<RssFeed>, unsigned int> FeedPtrPosPair;

class FeedListFormAction : public ListFormAction {
public:
	FeedListFormAction(View*,
		std::string formstr,
		Cache* cc,
		FilterContainer& f,
		ConfigContainer* cfg,
		RegexManager& r);
	~FeedListFormAction() override;
	void prepare() override;
	void init() override;
	void set_feedlist(std::vector<std::shared_ptr<RssFeed>>& feeds);
	void update_visible_feeds(std::vector<std::shared_ptr<RssFeed>>& feeds);
	KeyMapHintEntry* get_keymap_hint() override;
	std::shared_ptr<RssFeed> get_feed();

	std::string id() const override
	{
		return "feedlist";
	}
	std::string title() override;

	bool jump_to_next_unread_feed(unsigned int& feedpos);
	bool jump_to_previous_unread_feed(unsigned int& feedpos);
	bool jump_to_next_feed(unsigned int& feedpos);
	bool jump_to_previous_feed(unsigned int& feedpos);
	bool jump_to_random_unread_feed(unsigned int& feedpos);

	void handle_cmdline(const std::string& cmd) override;

	void finished_qna(Operation op) override;

	void mark_pos_if_visible(unsigned int pos);

private:
	void register_format_styles();

	void update_form_title(unsigned int width);

	unsigned int count_unread_feeds();

	int get_pos(unsigned int realidx);
	bool process_operation(Operation op,
		bool automatic = false,
		std::vector<std::string>* args = nullptr) override;

	void goto_feed(const std::string& str);

	void save_filterpos();

	void op_end_setfilter();
	void op_start_search();

	void handle_cmdline_num(unsigned int idx);

	void set_pos();

	std::string get_title(std::shared_ptr<RssFeed> feed);

	std::string format_line(const std::string& feedlist_format,
		std::shared_ptr<RssFeed> feed,
		unsigned int pos,
		unsigned int width);

	bool zero_feedpos;
	std::vector<FeedPtrPosPair> visible_feeds;
	std::string tag;

	Matcher matcher;
	bool apply_filter;

	History filterhistory;

	std::shared_ptr<RssFeed> search_dummy_feed;

	unsigned int filterpos;
	bool set_filterpos;

	RegexManager& rxman;

	FilterContainer& filters;

	std::string old_sort_order;

	ListWidget feeds_list;
};

} // namespace newsboat

#endif /* NEWSBOAT_FEEDLISTFORMACTION_H_ */

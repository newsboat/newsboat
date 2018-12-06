#ifndef NEWSBOAT_FEEDLISTFORMACTION_H_
#define NEWSBOAT_FEEDLISTFORMACTION_H_

#include "history.h"
#include "listformaction.h"
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
		FilterContainer* f,
		ConfigContainer* cfg);
	~FeedListFormAction() override;
	void prepare() override;
	void init() override;
	void set_feedlist(std::vector<std::shared_ptr<RssFeed>>& feeds);
	void update_visible_feeds(std::vector<std::shared_ptr<RssFeed>>& feeds);
	void set_tags(const std::vector<std::string>& t);
	KeyMapHintEntry* get_keymap_hint() override;
	std::shared_ptr<RssFeed> get_feed();

	void set_redraw(bool b) override
	{
		FormAction::set_redraw(b);
		apply_filter =
			!(cfg->get_configvalue_as_bool("show-read-feeds"));
	}

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

	void set_regexmanager(RegexManager* r);

private:
	int get_pos(unsigned int realidx);
	void process_operation(Operation op,
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
	unsigned int feeds_shown;
	bool quit;
	std::vector<FeedPtrPosPair> visible_feeds;
	std::string tag;
	std::vector<std::string> tags;

	Matcher m;
	bool apply_filter;

	History filterhistory;

	std::shared_ptr<RssFeed> search_dummy_feed;

	unsigned int filterpos;
	bool set_filterpos;

	RegexManager* rxman;

	unsigned int old_width;

	unsigned int unread_feeds;
	unsigned int total_feeds;

	FilterContainer* filters;

	std::string old_sort_order;
};

} // namespace newsboat

#endif /* NEWSBOAT_FEEDLISTFORMACTION_H_ */

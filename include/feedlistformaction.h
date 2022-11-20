#ifndef NEWSBOAT_FEEDLISTFORMACTION_H_
#define NEWSBOAT_FEEDLISTFORMACTION_H_

#include "3rd-party/optional.hpp"

#include "configcontainer.h"
#include "history.h"
#include "listformaction.h"
#include "matcher.h"
#include "regexmanager.h"
#include "view.h"
#include "utf8string.h"

namespace newsboat {

using FeedPtrPosPair = std::pair<std::shared_ptr<RssFeed>, unsigned int>;

class FeedListFormAction : public ListFormAction {
public:
	FeedListFormAction(View*,
		Utf8String formstr,
		Cache* cc,
		FilterContainer& f,
		ConfigContainer* cfg,
		RegexManager& r);
	~FeedListFormAction() override;
	void prepare() override;
	void init() override;
	void set_feedlist(std::vector<std::shared_ptr<RssFeed>>& feeds);
	void update_visible_feeds(std::vector<std::shared_ptr<RssFeed>>& feeds);
	const std::vector<KeyMapHintEntry>& get_keymap_hint() const override;
	std::shared_ptr<RssFeed> get_feed();

	Utf8String id() const override
	{
		return "feedlist";
	}
	Utf8String title() override;

	bool jump_to_next_unread_feed(unsigned int& feedpos);
	bool jump_to_previous_unread_feed(unsigned int& feedpos);
	bool jump_to_next_feed(unsigned int& feedpos);
	bool jump_to_previous_feed(unsigned int& feedpos);
	bool jump_to_random_unread_feed(unsigned int& feedpos);

	void handle_cmdline(const Utf8String& cmd) override;

	void finished_qna(Operation op) override;

	void mark_pos_if_visible(unsigned int pos);

private:
	void register_format_styles();

	void update_form_title(unsigned int width);

	unsigned int count_unread_feeds();
	unsigned int count_unread_articles();

	int get_pos(unsigned int realidx);
	bool process_operation(Operation op,
		bool automatic = false,
		std::vector<Utf8String>* args = nullptr) override;

	bool open_position_in_browser(unsigned int pos, bool interactive) const;

	void goto_feed(const Utf8String& str);

	void save_filterpos();

	void op_end_setfilter();
	void op_start_search();

	void handle_cmdline_num(unsigned int idx);
	void handle_tag(const Utf8String& params);
	void handle_goto(const Utf8String& param);
	void set_pos();

	Utf8String get_title(std::shared_ptr<RssFeed> feed);

	Utf8String format_line(const Utf8String& feedlist_format,
		std::shared_ptr<RssFeed> feed,
		unsigned int pos,
		unsigned int width);

	bool zero_feedpos;
	std::vector<FeedPtrPosPair> visible_feeds;
	Utf8String tag;

	Matcher matcher;
	bool filter_active;
	void apply_filter(const Utf8String& filter_query);

	History filterhistory;

	unsigned int filterpos;
	bool set_filterpos;

	RegexManager& rxman;

	FilterContainer& filter_container;

	nonstd::optional<FeedSortStrategy> old_sort_strategy;

	Cache* cache;
};

} // namespace newsboat

#endif /* NEWSBOAT_FEEDLISTFORMACTION_H_ */

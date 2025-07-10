#ifndef NEWSBOAT_FEEDLISTFORMACTION_H_
#define NEWSBOAT_FEEDLISTFORMACTION_H_

#include <optional>

#include "configcontainer.h"
#include "history.h"
#include "listformaction.h"
#include "matcher.h"
#include "regexmanager.h"
#include "stflrichtext.h"
#include "view.h"

namespace Newsboat {

typedef std::pair<std::shared_ptr<RssFeed>, unsigned int> FeedPtrPosPair;

class FeedListFormAction : public ListFormAction {
public:
	FeedListFormAction(View&,
		std::string formstr,
		Cache* cc,
		FilterContainer& f,
		ConfigContainer* cfg,
		RegexManager& r);
	~FeedListFormAction() override = default;
	void prepare() override;
	void init() override;
	void set_feedlist(std::vector<std::shared_ptr<RssFeed>>& feeds);
	void update_visible_feeds(std::vector<std::shared_ptr<RssFeed>>& feeds);
	std::vector<KeyMapHintEntry> get_keymap_hint() const override;
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

	void finished_qna(QnaFinishAction op) override;

	void mark_pos_if_visible(unsigned int pos);

protected:
	std::string main_widget() const override
	{
		return "feeds";
	}

private:
	void register_format_styles();

	void update_form_title(unsigned int width);

	unsigned int count_unread_feeds();
	unsigned int count_unread_articles();

	int get_pos(unsigned int realidx);
	bool process_operation(Operation op,
		const std::vector<std::string>& args,
		BindingType bindingType = BindingType::BindKey) override;

	bool open_position_in_browser(unsigned int pos, bool interactive) const;

	void goto_feed(const std::string& str);

	void save_filterpos();

	void op_end_setfilter();
	void op_start_search();

	void handle_cmdline_num(unsigned int idx);
	void handle_tag(const std::string& params);
	void handle_goto(const std::string& param);
	void set_pos();

	std::string get_title(std::shared_ptr<RssFeed> feed);

	StflRichText format_line(const std::string& feedlist_format,
		std::shared_ptr<RssFeed> feed,
		unsigned int pos,
		unsigned int width);

	bool zero_feedpos;
	std::vector<FeedPtrPosPair> visible_feeds;
	std::string tag;

	Matcher matcher;
	bool filter_active;
	void apply_filter(const std::string& filter_query);

	History filterhistory;

	unsigned int filterpos;
	bool set_filterpos;

	RegexManager& rxman;

	FilterContainer& filter_container;

	std::optional<FeedSortStrategy> old_sort_strategy;

	Cache* cache;
};

} // namespace Newsboat

#endif /* NEWSBOAT_FEEDLISTFORMACTION_H_ */

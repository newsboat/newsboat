#ifndef NEWSBOAT_FEEDLIST_FORMACTION_H_
#define NEWSBOAT_FEEDLIST_FORMACTION_H_

#include "list_formaction.h"
#include "matcher.h"
#include "history.h"
#include "regexmanager.h"
#include "view.h"

namespace newsboat {

typedef std::pair<std::shared_ptr<rss_feed>, unsigned int> feedptr_pos_pair;

class feedlist_formaction : public list_formaction {
	public:
		feedlist_formaction(view *, std::string formstr);
		~feedlist_formaction() override;
		void prepare() override;
		void init() override;
		void set_feedlist(std::vector<std::shared_ptr<rss_feed>>& feeds);
		void update_visible_feeds(std::vector<std::shared_ptr<rss_feed>>& feeds);
		void set_tags(const std::vector<std::string>& t);
		keymap_hint_entry * get_keymap_hint() override;
		std::shared_ptr<rss_feed> get_feed();

		void set_redraw(bool b) override {
			formaction::set_redraw(b);
			apply_filter = !(v->get_cfg()->get_configvalue_as_bool("show-read-feeds"));
		}

		std::string id() const override {
			return "feedlist";
		}
		std::string title() override;

		bool jump_to_next_unread_feed(unsigned int& feedpos);
		bool jump_to_previous_unread_feed(unsigned int& feedpos);
		bool jump_to_next_feed(unsigned int& feedpos);
		bool jump_to_previous_feed(unsigned int& feedpos);
		bool jump_to_random_unread_feed(unsigned int& feedpos);

		void handle_cmdline(const std::string& cmd) override;

		void finished_qna(operation op) override;

		void mark_pos_if_visible(unsigned int pos);

		void set_regexmanager(regexmanager * r);

	private:

		int get_pos(unsigned int realidx);
		void process_operation(operation op, bool automatic = false, std::vector<std::string> * args = nullptr) override;

		void goto_feed(const std::string& str);

		void save_filterpos();

		void op_end_setfilter();
		void op_start_search();

		void handle_cmdline_num(unsigned int idx);

		void set_pos();

		std::string get_title(std::shared_ptr<rss_feed> feed);

		std::string format_line(const std::string& feedlist_format, std::shared_ptr<rss_feed> feed, unsigned int pos, unsigned int width);

		bool zero_feedpos;
		unsigned int feeds_shown;
		bool quit;
		std::vector<feedptr_pos_pair> visible_feeds;
		std::string tag;
		std::vector<std::string> tags;

		matcher m;
		bool apply_filter;

		history filterhistory;

		std::shared_ptr<rss_feed> search_dummy_feed;

		unsigned int filterpos;
		bool set_filterpos;

		regexmanager * rxman;

		unsigned int old_width;

		unsigned int unread_feeds;
		unsigned int total_feeds;

		std::string old_sort_order;
};

}

#endif /* NEWSBOAT_FEEDLIST_FORMACTION_H_ */

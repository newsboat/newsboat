#ifndef NEWSBEUTER_FEEDLIST_FORMACTION__H
#define NEWSBEUTER_FEEDLIST_FORMACTION__H

#include <formaction.h>
#include <matcher.h>
#include <history.h>

namespace newsbeuter {

typedef std::pair<rss_feed *, unsigned int> feedptr_pos_pair;

class feedlist_formaction : public formaction {
	public:
		feedlist_formaction(view *, std::string formstr);
		virtual ~feedlist_formaction();
		virtual void prepare();
		virtual void init();
		void set_feedlist(std::vector<rss_feed>& feeds);
		void set_tags(const std::vector<std::string>& t);
		virtual keymap_hint_entry * get_keymap_hint();
		rss_feed * get_feed();

		bool jump_to_next_unread_feed(unsigned int& feedpos);
		bool jump_to_previous_unread_feed(unsigned int& feedpos);

		virtual void handle_cmdline(const std::string& cmd);

		virtual void finished_qna(operation op);

	private:

		int get_pos(unsigned int realidx);
		virtual void process_operation(operation op);

		bool zero_feedpos;
		unsigned int feeds_shown;
		bool auto_open;
		bool quit;
		std::vector<feedptr_pos_pair> visible_feeds;
		std::string tag;
		std::vector<std::string> tags;

		matcher m;
		bool apply_filter;

		history filterhistory;

		rss_feed search_dummy_feed;
};

}


#endif

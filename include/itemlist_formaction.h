#ifndef NEWSBEUTER_ITEMLIST_FORMACTION__H
#define NEWSBEUTER_ITEMLIST_FORMACTION__H

#include <formaction.h>
#include <history.h>

namespace newsbeuter {

typedef std::pair<rss_item *, unsigned int> itemptr_pos_pair;

class itemlist_formaction : public formaction {
	public:
		itemlist_formaction(view *, std::string formstr);
		virtual ~itemlist_formaction();
		virtual void prepare();
		virtual void init();

		inline void set_feed(rss_feed * fd) { 
			feed = fd; 
			update_visible_items = true; 
			do_update_visible_items();
			apply_filter = false;
		}

		inline rss_feed * get_feed() { return feed; }
		inline void set_pos(unsigned int p) { pos = p; }
		std::string get_guid();
		virtual keymap_hint_entry * get_keymap_hint();

		bool jump_to_next_unread_item(bool start_with_first);
		bool jump_to_previous_unread_item(bool start_with_last);

		virtual void handle_cmdline(const std::string& cmd);

		inline void set_update_visible_items(bool b) { update_visible_items = b; }

		void do_update_visible_items();

		virtual void finished_qna(operation op);

		inline void set_show_searchresult(bool b) { show_searchresult = b; }

	private:
		virtual void process_operation(operation op);
		void set_head(const std::string& s, unsigned int unread, unsigned int total, const std::string &url);
		int get_pos(unsigned int idx);

		unsigned int pos;
		rss_feed * feed;
		bool rebuild_list;
		bool apply_filter;
		matcher m;
		std::vector<itemptr_pos_pair> visible_items;
		bool update_visible_items;
		bool show_searchresult;

		history filterhistory;

		rss_feed search_dummy_feed;
};

}

#endif

#ifndef NEWSBEUTER_ITEMLIST_FORMACTION__H
#define NEWSBEUTER_ITEMLIST_FORMACTION__H

#include <formaction.h>
#include <history.h>
#include <mutex.h>
#include <logger.h>

namespace newsbeuter {

class itemlist_formaction : public formaction {
	public:
		itemlist_formaction(view *, std::string formstr);
		virtual ~itemlist_formaction();
		virtual void prepare();
		virtual void init();

		inline void set_feed(refcnt_ptr<rss_feed> fd) { 
			feed = fd; 
			update_visible_items = true; 
			do_update_visible_items();
			apply_filter = false;
		}

		inline refcnt_ptr<rss_feed> get_feed() { return feed; }
		inline void set_pos(unsigned int p) { 
			pos = p; 
			GetLogger().log(LOG_DEBUG, "itemlist_formaction::set_pos = p = %u", p);
		}
		std::string get_guid();
		virtual keymap_hint_entry * get_keymap_hint();

		bool jump_to_next_unread_item(bool start_with_first);
		bool jump_to_previous_unread_item(bool start_with_last);

		virtual void handle_cmdline(const std::string& cmd);

	private:
		virtual void process_operation(operation op);
		void set_head(const std::string& s, unsigned int unread, unsigned int total, const std::string &url);
		int get_pos(unsigned int idx);
		void do_update_visible_items();

		refcnt_ptr<rss_feed> feed;
		unsigned int pos;
		bool rebuild_list;
		bool apply_filter;
		matcher m;
		std::vector<std::pair<refcnt_ptr<rss_item>, unsigned int> > visible_items;
		bool update_visible_items;

		history filterhistory;

		mutex vimtx;
};

}

#endif

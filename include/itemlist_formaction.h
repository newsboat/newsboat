#ifndef NEWSBEUTER_ITEMLIST_FORMACTION__H
#define NEWSBEUTER_ITEMLIST_FORMACTION__H

#include <formaction.h>

namespace newsbeuter {

class itemlist_formaction : public formaction {
	public:
		itemlist_formaction(view *, std::string formstr);
		virtual ~itemlist_formaction();
		virtual void process_operation(operation op);
		virtual void prepare();
		virtual void init();
		inline void set_feed(rss_feed * f) { feed = f; }
		inline rss_feed * get_feed() { return feed; }
		inline void set_pos(unsigned int p) { pos = p; }
		std::string get_guid();
		virtual keymap_hint_entry * get_keymap_hint();

		bool jump_to_next_unread_item();

	private:
		void set_head(const std::string& s, unsigned int unread, unsigned int total, const std::string &url);

		rss_feed * feed;
		unsigned int pos;
		bool rebuild_list;
};

}

#endif

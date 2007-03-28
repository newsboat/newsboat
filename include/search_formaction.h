#ifndef NEWSBEUTER_SEARCH_FORMACTION__H
#define NEWSBEUTER_SEARCH_FORMACTION__H

#include <formaction.h>

namespace newsbeuter {

class search_formaction : public formaction {
	public:
		search_formaction(view *, std::string formstr);
		virtual ~search_formaction();
		virtual void process_operation(operation op);
		virtual void prepare();
		virtual void init();
		virtual keymap_hint_entry * get_keymap_hint();
		inline void set_feedurl(const std::string& f) { feedurl = f; }
	private:
		bool set_listfocus;
		std::string feedurl;
		std::vector<rss_item> items;

};

}

#endif

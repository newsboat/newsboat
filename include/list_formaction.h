#ifndef NEWSBEUTER_LIST_FORMACTION__H
#define NEWSBEUTER_LIST_FORMACTION__H

#include "formaction.h"

namespace newsbeuter {

class list_formaction : public formaction {
	public:
		list_formaction(view *, std::string formstr);
	protected:
		void open_unread_items_in_browser(std::shared_ptr<rss_feed> feed , bool markread);

};


}

#endif

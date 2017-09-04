#include <list_formaction.h>
#include <view.h>

namespace newsbeuter {

list_formaction::list_formaction(view * v, std::string formstr)
:formaction(v, formstr)
{}

void list_formaction::open_unread_items_in_browser(std::shared_ptr<rss_feed> feed , bool markread){
	int tabcount = 0;
	for (auto item : feed->items()) {
		if (tabcount < v->get_cfg()->get_configvalue_as_int("max-browser-tabs")) {
			if (item->unread()) {
				v->open_in_browser(item->link());
				tabcount += 1;
				item->set_unread(!markread);
			}
		} else {
			break;
		}
	}
}

}

#include "list_formaction.h"

#include "view.h"

namespace newsboat {

list_formaction::list_formaction(view* v, std::string formstr)
	: formaction(v, formstr)
{
}

void list_formaction::process_operation(
	operation op,
	bool,
	std::vector<std::string>*)
{
	switch (op) {
	case OP_1:
		formaction::start_cmdline("1");
		break;
	case OP_2:
		formaction::start_cmdline("2");
		break;
	case OP_3:
		formaction::start_cmdline("3");
		break;
	case OP_4:
		formaction::start_cmdline("4");
		break;
	case OP_5:
		formaction::start_cmdline("5");
		break;
	case OP_6:
		formaction::start_cmdline("6");
		break;
	case OP_7:
		formaction::start_cmdline("7");
		break;
	case OP_8:
		formaction::start_cmdline("8");
		break;
	case OP_9:
		formaction::start_cmdline("9");
		break;
	default:
		break;
	}
}

void list_formaction::open_unread_items_in_browser(
	std::shared_ptr<rss_feed> feed,
	bool markread)
{
	int tabcount = 0;
	for (auto item : feed->items()) {
		if (tabcount < v->get_cfg()->get_configvalue_as_int(
				       "max-browser-tabs")) {
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

} // namespace newsboat

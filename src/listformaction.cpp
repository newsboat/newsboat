#include "listformaction.h"

#include "view.h"

namespace newsboat {

ListFormaction::ListFormaction(View* v, std::string formstr)
	: Formaction(v, formstr)
{
}

void ListFormaction::process_operation(operation op,
	bool,
	std::vector<std::string>*)
{
	switch (op) {
	case OP_1:
		Formaction::start_cmdline("1");
		break;
	case OP_2:
		Formaction::start_cmdline("2");
		break;
	case OP_3:
		Formaction::start_cmdline("3");
		break;
	case OP_4:
		Formaction::start_cmdline("4");
		break;
	case OP_5:
		Formaction::start_cmdline("5");
		break;
	case OP_6:
		Formaction::start_cmdline("6");
		break;
	case OP_7:
		Formaction::start_cmdline("7");
		break;
	case OP_8:
		Formaction::start_cmdline("8");
		break;
	case OP_9:
		Formaction::start_cmdline("9");
		break;
	default:
		break;
	}
}

void ListFormaction::open_unread_items_in_browser(
	std::shared_ptr<RssFeed> feed,
	bool markread)
{
	int tabcount = 0;
	for (const auto& item : feed->items()) {
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

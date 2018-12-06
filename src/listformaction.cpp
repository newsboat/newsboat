#include "listformaction.h"

#include "view.h"

namespace newsboat {

ListFormAction::ListFormAction(View* v,
	std::string formstr,
	ConfigContainer* cfg)
	: FormAction(v, formstr, cfg)
{
}

void ListFormAction::process_operation(Operation op,
	bool,
	std::vector<std::string>*)
{
	switch (op) {
	case OP_1:
		FormAction::start_cmdline("1");
		break;
	case OP_2:
		FormAction::start_cmdline("2");
		break;
	case OP_3:
		FormAction::start_cmdline("3");
		break;
	case OP_4:
		FormAction::start_cmdline("4");
		break;
	case OP_5:
		FormAction::start_cmdline("5");
		break;
	case OP_6:
		FormAction::start_cmdline("6");
		break;
	case OP_7:
		FormAction::start_cmdline("7");
		break;
	case OP_8:
		FormAction::start_cmdline("8");
		break;
	case OP_9:
		FormAction::start_cmdline("9");
		break;
	default:
		break;
	}
}

void ListFormAction::open_unread_items_in_browser(std::shared_ptr<RssFeed> feed,
	bool markread)
{
	int tabcount = 0;
	for (const auto& item : feed->items()) {
		if (tabcount <
			cfg->get_configvalue_as_int("max-browser-tabs")) {
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

#include "listformaction.h"

#include "rssfeed.h"
#include "view.h"

namespace newsboat {

ListFormAction::ListFormAction(View* v,
	std::string formstr,
	ConfigContainer* cfg)
	: FormAction(v, formstr, cfg)
{
}

bool ListFormAction::process_operation(Operation op,
	bool,
	std::vector<std::string>*)
{
	switch (op) {
	case OP_CMD_START_1:
		FormAction::start_cmdline("1");
		break;
	case OP_CMD_START_2:
		FormAction::start_cmdline("2");
		break;
	case OP_CMD_START_3:
		FormAction::start_cmdline("3");
		break;
	case OP_CMD_START_4:
		FormAction::start_cmdline("4");
		break;
	case OP_CMD_START_5:
		FormAction::start_cmdline("5");
		break;
	case OP_CMD_START_6:
		FormAction::start_cmdline("6");
		break;
	case OP_CMD_START_7:
		FormAction::start_cmdline("7");
		break;
	case OP_CMD_START_8:
		FormAction::start_cmdline("8");
		break;
	case OP_CMD_START_9:
		FormAction::start_cmdline("9");
		break;
	default:
		break;
	}
	return true;
}

int ListFormAction::open_unread_items_in_browser(std::shared_ptr<RssFeed> feed,
	bool markread)
{
	int tabcount = 0;
	for (const auto& item : feed->items()) {
		if (tabcount <
			cfg->get_configvalue_as_int("max-browser-tabs")) {
			if (item->unread()) {
				if (int err = v->open_in_browser(item->link())) {
					return err;
				}
				tabcount += 1;
				item->set_unread(!markread);
			}
		} else {
			break;
		}
	}
	return 0;
}

} // namespace newsboat

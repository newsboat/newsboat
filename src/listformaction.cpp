#include "listformaction.h"

#include "controller.h"
#include "rssfeed.h"
#include "view.h"

namespace Newsboat {

ListFormAction::ListFormAction(View& v,
	const std::string& context,
	std::string formstr,
	std::string list_name,
	ConfigContainer* cfg, RegexManager& r)
	: FormAction(v, formstr, cfg)
	, list(list_name, context, FormAction::f, r, cfg->get_configvalue_as_int("scrolloff"))
{
}

bool ListFormAction::process_operation(Operation op,
	const std::vector<std::string>& /* args */,
	BindingType /*bindingType*/)
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
		return handle_list_operations(list, op);
	}
	return true;
}

std::optional<std::uint8_t> ListFormAction::open_unread_items_in_browser(
	std::shared_ptr<RssFeed> feed,
	bool markread)
{
	int tabcount = 0;
	std::optional<std::uint8_t> return_value = 0;
	std::vector<std::string> guids_of_read_articles;
	for (const auto& item : feed->items()) {
		if (tabcount <
			cfg->get_configvalue_as_int("max-browser-tabs")) {
			if (item->unread()) {
				const bool interactive = true;
				const auto exit_code = v.open_in_browser(item->link(), item->feedurl(),
						"article", item->title(), interactive);
				if (!exit_code.has_value() || *exit_code != 0) {
					return_value = exit_code;
					break;
				}

				tabcount += 1;
				if (markread) {
					item->set_unread(false);
					guids_of_read_articles.push_back(item->guid());
				}
			}
		} else {
			break;
		}
	}

	if (guids_of_read_articles.size() > 0) {
		v.get_ctrl().mark_all_read(guids_of_read_articles);
	}

	return return_value;
}

} // namespace Newsboat

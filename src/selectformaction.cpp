#include "selectformaction.h"

#include <cassert>
#include <sstream>
#include <string>

#include "config.h"
#include "fmtstrformatter.h"
#include "listformatter.h"
#include "strprintf.h"
#include "utils.h"
#include "view.h"

namespace newsboat {

/*
 * The SelectFormAction is used both for the "select tag" dialog
 * and the "select filter", as they do practically the same. That's
 * why there is the decision between SELECTTAG and SELECTFILTER on
 * a few places.
 */

SelectFormAction::SelectFormAction(View* vv,
	std::string formstr,
	ConfigContainer* cfg)
	: FormAction(vv, formstr, cfg)
	, quit(false)
	, type(SelectionType::TAG)
	, tags_list("taglist", FormAction::f, cfg->get_configvalue_as_int("scrolloff"))
{
}

SelectFormAction::~SelectFormAction() {}

void SelectFormAction::handle_cmdline(const std::string& cmd)
{
	unsigned int idx = 0;
	if (1 == sscanf(cmd.c_str(), "%u", &idx)) {
		if (idx > 0 &&
			idx <= ((type == SelectionType::TAG)
				? tags.size()
				: filters.size())) {
			tags_list.set_position(idx - 1);
		}
	} else {
		FormAction::handle_cmdline(cmd);
	}
}

bool SelectFormAction::process_operation(Operation op,
	bool /* automatic */,
	std::vector<std::string>* /* args */)
{
	bool hardquit = false;
	switch (op) {
	case OP_PREV:
	case OP_SK_UP:
		tags_list.move_up(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_NEXT:
	case OP_SK_DOWN:
		tags_list.move_down(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_SK_HOME:
		tags_list.move_to_first();
		break;
	case OP_SK_END:
		tags_list.move_to_last();
		break;
	case OP_SK_PGUP:
		tags_list.move_page_up(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_SK_PGDOWN:
		tags_list.move_page_down(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
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
	case OP_QUIT:
		value = "";
		quit = true;
		break;
	case OP_HARDQUIT:
		value = "";
		hardquit = true;
		break;
	case OP_OPEN: {
		switch (type) {
		case SelectionType::TAG: {
			if (tags.size() >= 1) {
				const auto pos = tags_list.get_position();
				value = tags[pos];
				quit = true;
			}
		}
		break;
		case SelectionType::FILTER: {
			if (filters.size() >= 1) {
				const auto pos = tags_list.get_position();
				value = filters[pos].expr;
				quit = true;
			}
		}
		break;
		default:
			assert(0); // should never happen
		}
	}
	break;
	default:
		break;
	}

	if (hardquit) {
		while (v->formaction_stack_size() > 0) {
			v->pop_current_formaction();
		}
	} else if (quit) {
		v->pop_current_formaction();
	}
	return true;
}

void SelectFormAction::prepare()
{
	if (do_redraw) {
		update_heading();

		ListFormatter listfmt;
		unsigned int i = 0;
		const auto selecttag_format = cfg->get_configvalue("selecttag-format");
		const auto width = tags_list.get_width();

		switch (type) {
		case SelectionType::TAG:
			for (const auto& tag : tags) {
				listfmt.add_line(
					utils::quote_for_stfl(
						format_line(selecttag_format,
							tag,
							i + 1,
							width)));
				i++;
			}
			break;
		case SelectionType::FILTER:
			for (const auto& filter : filters) {
				std::string tagstr = strprintf::fmt(
						"%4u  %s", i + 1, filter.name);
				listfmt.add_line(utils::quote_for_stfl(tagstr));
				i++;
			}
			break;
		default:
			assert(0);
		}
		tags_list.stfl_replace_lines(listfmt);

		do_redraw = false;
	}
}

void SelectFormAction::init()
{
	do_redraw = true;
	quit = false;
	value = "";

	recalculate_widget_dimensions();

	set_keymap_hints();
}

std::string SelectFormAction::format_line(const std::string& selecttag_format,
	const std::string& tag,
	unsigned int pos,
	unsigned int width)
{
	FmtStrFormatter fmt;

	const auto feedcontainer = v->get_ctrl()->get_feedcontainer();

	const auto total_feeds = feedcontainer->get_feed_count_per_tag(tag);
	const auto unread_feeds =
		feedcontainer->get_unread_feed_count_per_tag(tag);
	const auto unread_articles =
		feedcontainer->get_unread_item_count_per_tag(tag);

	fmt.register_fmt('i', strprintf::fmt("%u", pos));
	fmt.register_fmt('T', tag);
	fmt.register_fmt('f', std::to_string(unread_feeds));
	fmt.register_fmt('n', std::to_string(unread_articles));
	fmt.register_fmt('u', std::to_string(total_feeds));

	auto formattedLine = fmt.do_format(selecttag_format, width);

	return formattedLine;
}

void SelectFormAction::update_heading()
{
	std::string title;
	const unsigned int width = tags_list.get_width();

	FmtStrFormatter fmt;
	fmt.register_fmt('N', PROGRAM_NAME);
	fmt.register_fmt('V', utils::program_version());

	switch (type) {
	case SelectionType::TAG:
		title = fmt.do_format(
				cfg->get_configvalue("selecttag-title-format"), width);
		break;
	case SelectionType::FILTER:
		title = fmt.do_format(
				cfg->get_configvalue("selectfilter-title-format"),
				width);
		break;
	default:
		assert(0); // should never happen
	}
	set_value("head", title);
}

KeyMapHintEntry* SelectFormAction::get_keymap_hint()
{
	static KeyMapHintEntry hints_tag[] = {{OP_QUIT, _("Cancel")},
		{OP_OPEN, _("Select Tag")},
		{OP_NIL, nullptr}
	};
	static KeyMapHintEntry hints_filter[] = {{OP_QUIT, _("Cancel")},
		{OP_OPEN, _("Select Filter")},
		{OP_NIL, nullptr}
	};
	switch (type) {
	case SelectionType::TAG:
		return hints_tag;
	case SelectionType::FILTER:
		return hints_filter;
	}
	return nullptr;
}

std::string SelectFormAction::title()
{
	switch (type) {
	case SelectionType::TAG:
		return _("Select Tag");
	case SelectionType::FILTER:
		return _("Select Filter");
	default:
		return "";
	}
}

} // namespace newsboat

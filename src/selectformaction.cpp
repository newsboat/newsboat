#include "selectformaction.h"

#include <algorithm>
#include <cassert>
#include <string>

#include "config.h"
#include "controller.h"
#include "fmtstrformatter.h"
#include "stflrichtext.h"
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

SelectFormAction::SelectFormAction(View& vv,
	std::string formstr,
	ConfigContainer* cfg)
	: FormAction(vv, formstr, cfg)
	, is_first_draw(true)
	, type(SelectionType::TAG)
	, value("")
	, tags_list("taglist", FormAction::f, cfg->get_configvalue_as_int("scrolloff"))
{
}

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
	const std::vector<std::string>& /* args */,
	BindingType /*bindingType*/)
{
	bool quit = false;
	bool hardquit = false;
	switch (op) {
	case OP_PREV:
		tags_list.move_up(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_NEXT:
		tags_list.move_down(cfg->get_configvalue_as_bool("wrap-scroll"));
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
		return handle_list_operations(tags_list, op);
	}

	if (hardquit) {
		while (v.formaction_stack_size() > 0) {
			v.pop_current_formaction();
		}
	} else if (quit) {
		v.pop_current_formaction();
	}
	return true;
}
void SelectFormAction::prepare()
{
	if (do_redraw) {
		update_heading();

		const auto selecttag_format = cfg->get_configvalue("selecttag-format");

		std::uint32_t num_lines = 0;
		std::function<StflRichText(std::uint32_t, std::uint32_t)> render_line;

		switch (type) {
		case SelectionType::TAG:
			num_lines = tags.size();
			render_line = [this, selecttag_format](std::uint32_t line,
			std::uint32_t width) -> StflRichText {
				const auto& tag = tags[line];
				return StflRichText::from_plaintext(
					format_line(selecttag_format,
						tag,
						line + 1,
						width));
			};
			break;
		case SelectionType::FILTER:
			num_lines = filters.size();
			render_line = [this](std::uint32_t line, std::uint32_t width) -> StflRichText {
				(void)width;
				const auto& filter = filters[line];
				return StflRichText::from_plaintext(
					strprintf::fmt(
						"%4u  %s",
						line + 1,
						filter.name));
			};
			break;
		default:
			assert(0);
		}

		tags_list.invalidate_list_content(num_lines, render_line);

		if (is_first_draw && type == SelectionType::TAG && !value.empty()) {
			const auto it = std::find(tags.begin(), tags.end(), value);
			if (it != tags.end()) {
				const auto index = std::distance(tags.begin(), it);
				tags_list.set_position(index);
			}
		}

		is_first_draw = false;
		do_redraw = false;
	}
}

void SelectFormAction::init()
{
	do_redraw = true;

	recalculate_widget_dimensions();

	set_keymap_hints();
}

std::string SelectFormAction::format_line(const std::string& selecttag_format,
	const std::string& tag,
	unsigned int pos,
	unsigned int width)
{
	FmtStrFormatter fmt;

	const auto feedcontainer = v.get_ctrl().get_feedcontainer();

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
	set_title(title);
}

std::vector<KeyMapHintEntry> SelectFormAction::get_keymap_hint() const
{
	static const std::vector<KeyMapHintEntry> hints_tag = {{OP_QUIT, _("Cancel")},
		{OP_OPEN, _("Select Tag")}
	};
	static const std::vector<KeyMapHintEntry> hints_filter = {{OP_QUIT, _("Cancel")},
		{OP_OPEN, _("Select Filter")}
	};
	switch (type) {
	case SelectionType::TAG:
		return hints_tag;
	case SelectionType::FILTER:
		return hints_filter;
	}

	// The above `switch` handles all the cases, but GCC doesn't understand
	// that and wants a `return` here. These lines will never actually be
	// reached, so it's fine to return an empty vector here.
	static const std::vector<KeyMapHintEntry> no_hints;
	return no_hints;
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

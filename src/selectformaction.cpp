#include "selectformaction.h"

#include <cassert>
#include <sstream>

#include "config.h"
#include "formatstring.h"
#include "listformatter.h"
#include "strprintf.h"
#include "utils.h"
#include "view.h"

namespace newsboat {

/*
 * The SelectFormaction is used both for the "select tag" dialog
 * and the "select filter", as they do practically the same. That's
 * why there is the decision between SELECTTAG and SELECTFILTER on
 * a few places.
 */

SelectFormaction::SelectFormaction(View* vv, std::string formstr)
	: Formaction(vv, formstr)
	, quit(false)
	, type(SelectionType::TAG)
{
}

SelectFormaction::~SelectFormaction() {}

void SelectFormaction::handle_cmdline(const std::string& cmd)
{
	unsigned int idx = 0;
	if (1 == sscanf(cmd.c_str(), "%u", &idx)) {
		if (idx > 0 &&
			idx <= ((type == SelectionType::TAG)
					       ? tags.size()
					       : filters.size())) {
			f->set("tagpos", std::to_string(idx - 1));
		}
	} else {
		Formaction::handle_cmdline(cmd);
	}
}

void SelectFormaction::process_operation(operation op,
	bool /* automatic */,
	std::vector<std::string>* /* args */)
{
	bool hardquit = false;
	switch (op) {
	case OP_QUIT:
		value = "";
		quit = true;
		break;
	case OP_HARDQUIT:
		value = "";
		hardquit = true;
		break;
	case OP_OPEN: {
		std::string tagposname = f->get("tagposname");
		unsigned int pos = Utils::to_u(tagposname);
		if (tagposname.length() > 0) {
			switch (type) {
			case SelectionType::TAG: {
				if (pos < tags.size()) {
					value = tags[pos];
					quit = true;
				}
			} break;
			case SelectionType::FILTER: {
				if (pos < filters.size()) {
					value = filters[pos].second;
					quit = true;
				}
			} break;
			default:
				assert(0); // should never happen
			}
		}
	} break;
	default:
		break;
	}

	if (hardquit) {
		while (v->Formaction_stack_size() > 0) {
			v->pop_current_formaction();
		}
	} else if (quit) {
		v->pop_current_formaction();
	}
}

void SelectFormaction::prepare()
{
	if (do_redraw) {
		ListFormatter listfmt;
		unsigned int i = 0;
		switch (type) {
		case SelectionType::TAG:
			for (const auto& tag : tags) {
				std::string tagstr = StrPrintf::fmt(
					"%4u  %s (%u)",
					i + 1,
					tag,
					v->get_ctrl()
						->get_feedcontainer()
						->get_feed_count_per_tag(tag));
				listfmt.add_line(tagstr, i);
				i++;
			}
			break;
		case SelectionType::FILTER:
			for (const auto& filter : filters) {
				std::string tagstr = StrPrintf::fmt(
					"%4u  %s", i + 1, filter.first);
				listfmt.add_line(tagstr, i);
				i++;
			}
			break;
		default:
			assert(0);
		}
		f->modify("taglist", "replace_inner", listfmt.format_list());

		do_redraw = false;
	}
}

void SelectFormaction::init()
{
	std::string title;
	do_redraw = true;
	quit = false;
	value = "";

	std::string Viewwidth = f->get("taglist:w");
	unsigned int width = Utils::to_u(Viewwidth, 80);

	set_keymap_hints();

	FmtStrFormatter fmt;
	fmt.register_fmt('N', PROGRAM_NAME);
	fmt.register_fmt('V', PROGRAM_VERSION);

	switch (type) {
	case SelectionType::TAG:
		title = fmt.do_format(
			v->get_cfg()->get_configvalue("selecttag-title-Format"),
			width);
		break;
	case SelectionType::FILTER:
		title = fmt.do_format(v->get_cfg()->get_configvalue(
					      "selectfilter-title-Format"),
			width);
		break;
	default:
		assert(0); // should never happen
	}
	f->set("head", title);
}

keymap_hint_entry* SelectFormaction::get_keymap_hint()
{
	static keymap_hint_entry hints_tag[] = {{OP_QUIT, _("Cancel")},
		{OP_OPEN, _("Select Tag")},
		{OP_NIL, nullptr}};
	static keymap_hint_entry hints_filter[] = {{OP_QUIT, _("Cancel")},
		{OP_OPEN, _("Select Filter")},
		{OP_NIL, nullptr}};
	switch (type) {
	case SelectionType::TAG:
		return hints_tag;
	case SelectionType::FILTER:
		return hints_filter;
	}
	return nullptr;
}

std::string SelectFormaction::title()
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

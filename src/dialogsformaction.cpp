#include "dialogsformaction.h"

#include <cstdio>
#include <string>

#include "config.h"
#include "fmtstrformatter.h"
#include "listformatter.h"
#include "strprintf.h"
#include "utils.h"
#include "view.h"

namespace newsboat {

DialogsFormAction::DialogsFormAction(View* vv,
	std::string formstr,
	ConfigContainer* cfg)
	: FormAction(vv, formstr, cfg)
	, dialogs_list("dialogs", FormAction::f,
		  cfg->get_configvalue_as_int("scrolloff"))
{
}

DialogsFormAction::~DialogsFormAction() {}

void DialogsFormAction::init()
{
	set_keymap_hints();

	recalculate_widget_dimensions();
}

void DialogsFormAction::prepare()
{
	if (do_redraw) {
		update_heading();

		ListFormatter listfmt;

		unsigned int i = 1;
		for (const auto& fa : v->get_formaction_names()) {
			LOG(Level::DEBUG,
				"DialogsFormAction::prepare: p1 = %p p2 = %p",
				v->get_formaction(fa.first).get(),
				get_parent_formaction().get());
			listfmt.add_line(
				utils::quote_for_stfl(
					strprintf::fmt("%4u %s %s",
						i,
						(v->get_formaction(fa.first).get() ==
							get_parent_formaction().get())
						? "*"
						: " ",
						fa.second)));
			i++;
		}

		dialogs_list.stfl_replace_lines(listfmt);

		do_redraw = false;
	}
}

void DialogsFormAction::update_heading()
{

	const unsigned int width = dialogs_list.get_width();
	const std::string title_format = cfg->get_configvalue("dialogs-title-format");
	FmtStrFormatter fmt;
	fmt.register_fmt('N', PROGRAM_NAME);
	fmt.register_fmt('V', utils::program_version());
	f.set("head", fmt.do_format(title_format, width));
}

KeyMapHintEntry* DialogsFormAction::get_keymap_hint()
{
	static KeyMapHintEntry hints[] = {{OP_QUIT, _("Close")},
		{OP_OPEN, _("Goto Dialog")},
		{OP_CLOSEDIALOG, _("Close Dialog")},
		{OP_NIL, nullptr}
	};
	return hints;
}

bool DialogsFormAction::process_operation(Operation op,
	bool /* automatic */,
	std::vector<std::string>* /* args */)
{
	switch (op) {
	case OP_OPEN: {
		const unsigned int pos = dialogs_list.get_position();
		v->set_current_formaction(pos);
	}
	break;
	case OP_CLOSEDIALOG: {
		const unsigned int pos = dialogs_list.get_position();
		if (pos != 0) {
			v->remove_formaction(pos);
			do_redraw = true;
		} else {
			v->show_error(
				_("Error: you can't remove the feed list!"));
		}
	}
	break;
	case OP_PREV:
	case OP_SK_UP:
		dialogs_list.move_up(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_NEXT:
	case OP_SK_DOWN:
		dialogs_list.move_down(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_SK_HOME:
		dialogs_list.move_to_first();
		break;
	case OP_SK_END:
		dialogs_list.move_to_last();
		break;
	case OP_SK_PGUP:
		dialogs_list.move_page_up(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_SK_PGDOWN:
		dialogs_list.move_page_down(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_QUIT:
		v->pop_current_formaction();
		break;
	default:
		break;
	}
	return true;
}

std::string DialogsFormAction::title()
{
	return ""; // will never be displayed
}

void DialogsFormAction::handle_cmdline(const std::string& cmd)
{
	unsigned int idx = 0;
	if (1 == sscanf(cmd.c_str(), "%u", &idx)) {
		if (idx >= 1 && idx <= v->formaction_stack_size()) {
			dialogs_list.set_position(idx - 1);
		} else {
			v->show_error(_("Invalid position!"));
		}
	} else {
		FormAction::handle_cmdline(cmd);
	}
}

} // namespace newsboat

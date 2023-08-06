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
	: ListFormAction(vv, formstr, "dialogs", cfg)
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

		list.stfl_replace_lines(listfmt);

		do_redraw = false;
	}
}

void DialogsFormAction::update_heading()
{

	const unsigned int width = list.get_width();
	const std::string title_format = cfg->get_configvalue("dialogs-title-format");
	FmtStrFormatter fmt;
	fmt.register_fmt('N', PROGRAM_NAME);
	fmt.register_fmt('V', utils::program_version());
	set_value("head", fmt.do_format(title_format, width));
}

const std::vector<KeyMapHintEntry>& DialogsFormAction::get_keymap_hint() const
{
	static const std::vector<KeyMapHintEntry> hints = {{OP_QUIT, _("Close")},
		{OP_OPEN, _("Goto Dialog")},
		{OP_CLOSEDIALOG, _("Close Dialog")}
	};
	return hints;
}

bool DialogsFormAction::process_operation(Operation op, std::vector<std::string>* args)
{
	switch (op) {
	case OP_OPEN: {
		const unsigned int pos = list.get_position();
		v->set_current_formaction(pos);
	}
	break;
	case OP_CLOSEDIALOG: {
		const unsigned int pos = list.get_position();
		if (pos != 0) {
			v->remove_formaction(pos);
			do_redraw = true;
		} else {
			v->get_statusline().show_error(
				_("Error: you can't remove the feed list!"));
		}
	}
	break;
	case OP_PREV:
		list.move_up(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_NEXT:
		list.move_down(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_QUIT:
		v->pop_current_formaction();
		break;
	default:
		ListFormAction::process_operation(op, args);
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
			list.set_position(idx - 1);
		} else {
			v->get_statusline().show_error(_("Invalid position!"));
		}
	} else {
		FormAction::handle_cmdline(cmd);
	}
}

} // namespace newsboat

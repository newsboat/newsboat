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
	, update_list(true)
	, dialogs_list("dialogs", FormAction::f)
{
}

DialogsFormAction::~DialogsFormAction() {}

void DialogsFormAction::init()
{
	set_keymap_hints();

	f.run(-3); // compute all widget dimensions

	unsigned int width = utils::to_u(f.get("dialogs:w"));
	std::string title_format = cfg->get_configvalue("dialogs-title-format");
	FmtStrFormatter fmt;
	fmt.register_fmt('N', PROGRAM_NAME);
	fmt.register_fmt('V', utils::program_version());
	f.set("head", fmt.do_format(title_format, width));
}

void DialogsFormAction::prepare()
{
	if (update_list) {
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
						fa.second)),
				std::to_string(fa.first));
			i++;
		}

		dialogs_list.stfl_replace_lines(listfmt);

		update_list = false;
	}
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
		std::string dialogposname = f.get("dialogs_pos");
		if (dialogposname.length() > 0) {
			v->set_current_formaction(utils::to_u(dialogposname));
		} else {
			v->show_error(_("No item selected!"));
		}
	}
	break;
	case OP_CLOSEDIALOG: {
		std::string dialogposname = f.get("dialogs_pos");
		if (dialogposname.length() > 0) {
			unsigned int dialogpos = utils::to_u(dialogposname);
			if (dialogpos != 0) {
				v->remove_formaction(dialogpos);
				update_list = true;
			} else {
				v->show_error(
					_("Error: you can't remove the feed "
						"list!"));
			}
		} else {
			v->show_error(_("No item selected!"));
		}
	}
	break;
	case OP_SK_UP:
		dialogs_list.move_up(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
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
		if (idx <= v->formaction_stack_size()) {
			f.set("dialogs_pos", std::to_string(idx - 1));
		} else {
			v->show_error(_("Invalid position!"));
		}
	} else {
		FormAction::handle_cmdline(cmd);
	}
}

} // namespace newsboat

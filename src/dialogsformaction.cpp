#include "dialogsformaction.h"

#include <cstdio>

#include "config.h"
#include "formatstring.h"
#include "listformatter.h"
#include "strprintf.h"
#include "utils.h"
#include "view.h"

namespace newsboat {

DialogsFormaction::DialogsFormaction(View* vv, std::string formstr)
	: Formaction(vv, formstr)
	, update_list(true)
{
}

DialogsFormaction::~DialogsFormaction() {}

void DialogsFormaction::init()
{
	set_keymap_hints();

	unsigned int width = Utils::to_u(f->get("dialogs:w"));
	std::string title_format =
		v->get_cfg()->get_configvalue("dialogs-title-Format");
	FmtStrFormatter fmt;
	fmt.register_fmt('N', PROGRAM_NAME);
	fmt.register_fmt('V', PROGRAM_VERSION);
	f->set("head", fmt.do_format(title_format, width));
}

void DialogsFormaction::prepare()
{
	if (update_list) {
		ListFormatter listfmt;

		unsigned int i = 1;
		for (const auto& fa : v->get_formaction_names()) {
			LOG(Level::DEBUG,
				"DialogsFormaction::prepare: p1 = %p p2 = %p",
				v->get_formaction(fa.first).get(),
				get_parent_formaction().get());
			listfmt.add_line(
				StrPrintf::fmt("%4u %s %s",
					i,
					(v->get_formaction(fa.first).get() ==
						get_parent_formaction().get())
						? "*"
						: " ",
					fa.second),
				fa.first);
			i++;
		}

		f->modify("dialogs", "replace_inner", listfmt.format_list());

		update_list = false;
	}
}

keymap_hint_entry* DialogsFormaction::get_keymap_hint()
{
	static keymap_hint_entry hints[] = {{OP_QUIT, _("Close")},
		{OP_OPEN, _("Goto Dialog")},
		{OP_CLOSEDIALOG, _("Close Dialog")},
		{OP_NIL, nullptr}};
	return hints;
}

void DialogsFormaction::process_operation(operation op,
	bool /* automatic */,
	std::vector<std::string>* /* args */)
{
	switch (op) {
	case OP_OPEN: {
		std::string dialogposname = f->get("dialogpos");
		if (dialogposname.length() > 0) {
			v->set_current_formaction(Utils::to_u(dialogposname));
		} else {
			v->show_error(_("No item selected!"));
		}
	} break;
	case OP_CLOSEDIALOG: {
		std::string dialogposname = f->get("dialogpos");
		if (dialogposname.length() > 0) {
			unsigned int dialogpos = Utils::to_u(dialogposname);
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
	} break;
	case OP_QUIT:
		v->pop_current_formaction();
		break;
	default:
		break;
	}
}

std::string DialogsFormaction::title()
{
	return ""; // will never be displayed
}

void DialogsFormaction::handle_cmdline(const std::string& cmd)
{
	unsigned int idx = 0;
	if (1 == sscanf(cmd.c_str(), "%u", &idx)) {
		if (idx <= v->Formaction_stack_size()) {
			f->set("dialogpos", std::to_string(idx - 1));
		} else {
			v->show_error(_("Invalid position!"));
		}
	} else {
		Formaction::handle_cmdline(cmd);
	}
}

} // namespace newsboat

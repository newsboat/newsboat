#include <config.h>
#include <view.h>
#include <dialogs_formaction.h>
#include <listformatter.h>
#include <utils.h>

namespace newsbeuter {

dialogs_formaction::dialogs_formaction(view * vv, std::string formstr) : formaction(vv, formstr), update_list(true) {
}

dialogs_formaction::~dialogs_formaction() {
}

void dialogs_formaction::init() {
	set_keymap_hints();
}

void dialogs_formaction::prepare() {
	if (update_list) {
		listformatter listfmt;
		std::vector<std::pair<unsigned int, std::string> > formaction_names = v->get_formaction_names();

		for (std::vector<std::pair<unsigned int, std::string> >::iterator it=formaction_names.begin();it!=formaction_names.end();it++) {
			listfmt.add_line(it->second, it->first);
		}

		f->modify("dialogs", "replace_inner", listfmt.format_list());

		update_list = false;
	}
}

keymap_hint_entry * dialogs_formaction::get_keymap_hint() {
	static keymap_hint_entry hints[] = {
		{ OP_QUIT, _("Close") },
		{ OP_OPEN, _("Goto Dialog") },
		{ OP_CLOSEDIALOG, _("Close Dialog") },
		{ OP_NIL, NULL }
	};
	return hints;
}

void dialogs_formaction::process_operation(operation op, bool automatic, std::vector<std::string> * args) {
	switch (op) {
		case OP_OPEN: {
				std::string dialogposname = f->get("dialogpos");
				unsigned int dialogpos = utils::to_u(dialogposname);
				if (dialogposname.length() > 0) {
					v->set_current_formaction(dialogpos);
				} else {
					v->show_error(_("No item selected!"));
				}
			}
			break;
		case OP_CLOSEDIALOG: {
				std::string dialogposname = f->get("dialogpos");
				unsigned int dialogpos = utils::to_u(dialogposname);
				if (dialogposname.length() > 0) {
					v->remove_formaction(dialogpos);
					update_list = true;
				} else {
					v->show_error(_("No item selected!"));
				}
			}
			break;
		case OP_QUIT:
			v->pop_current_formaction();
			break;
		default:
			break;
	}
}

}

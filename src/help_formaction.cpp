#include <sstream>
#include <config.h>
#include <help_formaction.h>
#include <formatstring.h>
#include <view.h>
#include <listformatter.h>
#include <utils.h>

namespace newsbeuter {

help_formaction::help_formaction(view * vv, std::string formstr)
	: formaction(vv, formstr), quit(false) { 
}

help_formaction::~help_formaction() { }

void help_formaction::process_operation(operation op, bool /* automatic */, std::vector<std::string> * /* args */) {
	switch (op) {
		case OP_QUIT:
			quit = true;
			break;
		default:
			break;
	}
	if (quit) {
		v->pop_current_formaction();
	}
}

void help_formaction::prepare() {
	if (do_redraw) {
		std::string listwidth = f->get("helptext:w");
		std::istringstream is(listwidth);
		unsigned int width;
		is >> width;

		fmtstr_formatter fmt;
		fmt.register_fmt('N', PROGRAM_NAME);
		fmt.register_fmt('V', PROGRAM_VERSION);
		f->set("head",fmt.do_format(v->get_cfg()->get_configvalue("help-title-format"), width));
		
		std::vector<keymap_desc> descs;
		v->get_keys()->get_keymap_descriptions(descs, KM_NEWSBEUTER);

		listformatter listfmt;
		
		for (std::vector<keymap_desc>::iterator it=descs.begin();it!=descs.end();++it) {
			unsigned int how_often = 3 - (it->cmd.length() / 8);
			char tabs[] = "\t\t\t";
			tabs[how_often] = '\0';
			listfmt.add_line(utils::strprintf("%s\t%s%s%s", it->key.c_str(), it->cmd.c_str(), tabs, it->desc.c_str()));
		}

		f->modify("helptext","replace_inner", listfmt.format_list());

		do_redraw = false;
	}
	quit = false;
}

void help_formaction::init() {
	set_keymap_hints();
}

keymap_hint_entry * help_formaction::get_keymap_hint() {
	static keymap_hint_entry hints[] = {
		{ OP_QUIT, _("Quit") },
		{ OP_NIL, NULL }
	};
	return hints;
}

}

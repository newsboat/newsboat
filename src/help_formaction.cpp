#include <config.h>
#include <help_formaction.h>
#include <view.h>

namespace newsbeuter {

help_formaction::help_formaction(view * vv, std::string formstr)
	: formaction(vv, formstr), quit(false) { 
}

help_formaction::~help_formaction() { }

void help_formaction::process_operation(operation op, int /*rawchar*/) {
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

		char buf[1024];
		snprintf(buf,sizeof(buf),_("%s %s - Help"), PROGRAM_NAME, PROGRAM_VERSION);
		f->set("head",buf);
		
		std::vector<std::pair<std::string,std::string> > descs;
		v->get_keys()->get_keymap_descriptions(descs, KM_NEWSBEUTER);
		
		std::string code = "{list";
		
		for (std::vector<std::pair<std::string,std::string> >::iterator it=descs.begin();it!=descs.end();++it) {
			std::string line = "{listitem text:";

			std::string descline("\t");
			descline.append(it->first);
			descline.append(1,'\t');
			descline.append(it->second);

			line.append(stfl::quote(descline));
			line.append("}");
			
			code.append(line);
		}
		
		code.append("}");
		
		f->modify("helptext","replace_inner",code);

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

#include <sstream>
#include <config.h>
#include <help_formaction.h>
#include <formatstring.h>
#include <view.h>
#include <listformatter.h>
#include <utils.h>
#include <cstring>

namespace newsbeuter {

help_formaction::help_formaction(view * vv, std::string formstr)
	: formaction(vv, formstr), quit(false), apply_search(false) { 
}

help_formaction::~help_formaction() { }

void help_formaction::process_operation(operation op, bool /* automatic */, std::vector<std::string> * /* args */) {
	switch (op) {
		case OP_QUIT:
			quit = true;
			break;
		case OP_SEARCH: {
				std::vector<qna_pair> qna;
				qna.push_back(qna_pair(_("Search for: "), ""));
				this->start_qna(qna, OP_INT_START_SEARCH, &searchhistory);
			}
			break;
		case OP_CLEARFILTER:
			apply_search = false;
			do_redraw = true;
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
		v->get_keys()->get_keymap_descriptions(descs, v->get_keys()->get_flag_from_context(context));

		listformatter listfmt;
		
		for (std::vector<keymap_desc>::iterator it=descs.begin();it!=descs.end();++it) {
			if (context.length() > 0 && it->ctx != context)
				continue;
			if (!apply_search || strcasestr(it->key.c_str(), searchphrase.c_str())!=NULL || 
					strcasestr(it->cmd.c_str(), searchphrase.c_str())!=NULL ||
					strcasestr(it->desc.c_str(), searchphrase.c_str())!=NULL) {
				unsigned int how_often_1 = 2 - (it->key.length() / 8);
				unsigned int how_often_2 = 3 - (it->cmd.length() / 8);
				char tabs_1[] = "\t\t";
				char tabs_2[] = "\t\t\t";
				tabs_1[how_often_1] = '\0';
				tabs_2[how_often_2] = '\0';
				listfmt.add_line(utils::strprintf("%s%s%s%s%s", it->key.c_str(), tabs_1, it->cmd.c_str(), tabs_2, it->desc.c_str()));
			}
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
		{ OP_SEARCH, _("Search") },
		{ OP_CLEARFILTER, _("Clear") },
		{ OP_NIL, NULL }
	};
	return hints;
}

void help_formaction::finished_qna(operation ) {
	searchphrase = qna_responses[0];
	apply_search = true;
	do_redraw = true;
}

void help_formaction::set_context(const std::string& ctx) {
	if (context != ctx) {
		do_redraw = true;
		context = ctx;
	}
}

std::string help_formaction::title() {
	return _("Help");
}

}

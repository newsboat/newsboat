#include <sstream>
#include <config.h>
#include <help_formaction.h>
#include <formatstring.h>
#include <view.h>
#include <listformatter.h>
#include <utils.h>
#include <keymap.h>
#include <cstring>

namespace newsbeuter {

help_formaction::help_formaction(view * vv, std::string formstr)
	: formaction(vv, formstr), quit(false), apply_search(false) { 
}

help_formaction::~help_formaction() { }

void help_formaction::process_operation(operation op, bool /* automatic */, std::vector<std::string> * /* args */) {
	bool hardquit = false;
	switch (op) {
		case OP_QUIT:
			quit = true;
			break;
		case OP_HARDQUIT:
			hardquit = true;
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
	if (hardquit) {
		while (v->formaction_stack_size() > 0) {
			v->pop_current_formaction();
		}
	} else if (quit) {
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

		std::string highlighted_searchphrase = utils::strprintf("<hl>%s</>", searchphrase.c_str());
		std::vector<std::string> colors = utils::tokenize(v->get_cfg()->get_configvalue("search-highlight-colors"), " ");
		f->set("highlight", make_colorstring(colors));
		listformatter listfmt;

		unsigned int unbound_count = 0;
		unsigned int syskey_count = 0;

		for (unsigned int i=0;i<3;i++) {
			for (auto desc : descs) {
				bool condition;
				switch (i) {
					case 0: 
						condition = (desc.key.length() == 0 || desc.flags & KM_SYSKEYS); 
						if (desc.key.length() == 0)
							unbound_count++;
						if (desc.flags & KM_SYSKEYS)
							syskey_count++;
						break;
					case 1: 
						condition = !(desc.flags & KM_SYSKEYS); 
						break;
					case 2: 
						condition = (desc.key.length() > 0 || desc.flags & KM_SYSKEYS); 
						break;
					default: condition = true; break;
				}
				if (context.length() > 0 && (desc.ctx != context || condition))
					continue;
				if (!apply_search || strcasestr(desc.key.c_str(), searchphrase.c_str())!=NULL || 
						strcasestr(desc.cmd.c_str(), searchphrase.c_str())!=NULL ||
						strcasestr(desc.desc.c_str(), searchphrase.c_str())!=NULL) {
					char tabs_1[] = "                ";
					char tabs_2[] = "                        ";
					int how_often_1 = strlen(tabs_1) - desc.key.length();
					int how_often_2 = strlen(tabs_2) - desc.cmd.length();
					if (how_often_1 <= 0)
						how_often_1 = 1;
					if (how_often_2 <= 0)
						how_often_2 = 1;
					tabs_1[how_often_1] = '\0';
					tabs_2[how_often_2] = '\0';
					std::string line;
					switch (i) {
						case 0:
						case 1: line = utils::strprintf("%s%s%s%s%s", desc.key.c_str(), tabs_1, desc.cmd.c_str(), tabs_2, desc.desc.c_str()); break;
						case 2: line = utils::strprintf("%s%s%s%s", desc.cmd.c_str(), tabs_1, tabs_2, desc.desc.c_str()); break;
					}
					LOG(LOG_DEBUG, "help_formaction::prepare: step 1 - line = %s", line.c_str());
					line = utils::quote_for_stfl(line);
					LOG(LOG_DEBUG, "help_formaction::prepare: step 2 - line = %s", line.c_str());
					if (apply_search && searchphrase.length() > 0) {
						line = utils::replace_all(line, searchphrase, highlighted_searchphrase);
						LOG(LOG_DEBUG, "help_formaction::prepare: step 3 - line = %s", line.c_str());
					}
					listfmt.add_line(line);
				}
			}
			switch (i) {
				case 0:
					if (syskey_count > 0) {
						listfmt.add_line("");
						listfmt.add_line(_("Generic bindings:"));
						listfmt.add_line("");
					}
					break;
				case 1:
					if (unbound_count > 0) {
						listfmt.add_line("");
						listfmt.add_line(_("Unbound functions:"));
						listfmt.add_line("");
					}
				break;
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

void help_formaction::finished_qna(operation op) {
	switch (op) {
	case OP_INT_START_SEARCH:
		searchphrase = qna_responses[0];
		apply_search = true;
		do_redraw = true;
		break;
	default:
		break;
	}
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

std::string help_formaction::make_colorstring(const std::vector<std::string>& colors) {
	std::string result;
	if (colors.size() > 0) {
		if (colors[0] != "default") {
			result.append("fg=");
			result.append(colors[0]);
		}
		if (colors.size() > 1) {
			if (colors[1] != "default") {
				if (result.length() > 0)
					result.append(",");
				result.append("bg=");
				result.append(colors[1]);
			}
		}
		for (unsigned int i=2;i<colors.size();i++) {
			if (result.length() > 0)
				result.append(",");
			result.append("attr=");
			result.append(colors[i]);
		}
	}
	return result;
}

}

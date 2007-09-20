#include <select_formaction.h>
#include <view.h>
#include <config.h>

#include <sstream>
#include <cassert>

namespace newsbeuter {

/*
 * The select_formaction is used both for the "select tag" dialog
 * and the "select filter", as they do practically the same. That's
 * why there is the decision between SELECTTAG and SELECTFILTER on
 * a few places.
 */

select_formaction::select_formaction(view * vv, std::string formstr)
	: formaction(vv, formstr) { }

select_formaction::~select_formaction() { }

void select_formaction::handle_cmdline(const std::string& cmd) {
	unsigned int idx = 0;
	if (1==sscanf(cmd.c_str(),"%u",&idx)) {
		if (idx > 0 && idx <= ((type == SELECTTAG) ? tags.size() : filters.size())) {
			std::ostringstream idxstr;
			idxstr << idx - 1;
			f->set("tagpos", idxstr.str());
		}
	} else {
		formaction::handle_cmdline(cmd);
	}
}

void select_formaction::process_operation(operation op) {
	switch (op) {
		case OP_QUIT:
			value = "";
			quit = true;
			break;
		case OP_OPEN: {
				std::string tagposname = f->get("tagposname");
				std::istringstream posname(tagposname);
				unsigned int pos = 0;
				posname >> pos;
				if (tagposname.length() > 0) {
					switch (type) {
					case SELECTTAG: {
							if (pos < tags.size()) {
								value = tags[pos];
								quit = true;
							}
						}
						break;
					case SELECTFILTER: {
							if (pos < filters.size()) {
								value = filters[pos].second;
								quit = true;
							}
						}
						break;
					default:
						assert(0); // should never happen
					}
				}
			}
			break;
		default:
			break;
	}

	if (quit) {
		v->pop_current_formaction();
	}
}

void select_formaction::prepare() {
	if (do_redraw) {
		std::string code = "{list";
		unsigned int i=0;
		switch (type) {
		case SELECTTAG:
			for (std::vector<std::string>::const_iterator it=tags.begin();it!=tags.end();++it,++i) {
				std::ostringstream line;
				char num[32];
				snprintf(num,sizeof(num),"%4d  ", i+1);
				std::string tagstr = num;
				tagstr.append(it->c_str());
				line << "{listitem[" << i << "] text:" << stfl::quote(tagstr.c_str()) << "}";
				code.append(line.str());
			}
			break;
		case SELECTFILTER:
			for (std::vector<filter_name_expr_pair>::const_iterator it=filters.begin();it!=filters.end();++it,++i) {
				std::ostringstream line;
				char num[32];
				snprintf(num,sizeof(num),"%4d  ", i+1);
				std::string tagstr = num;
				tagstr.append(it->first.c_str());
				line << "{listitem[" << i << "] text:" << stfl::quote(tagstr.c_str()) << "}";
				code.append(line.str());
			}
			break;
		default:
			assert(0);
		}
		code.append("}");
		f->modify("taglist", "replace_inner", code);
		
		do_redraw = false;
	}
}

void select_formaction::init() {
	do_redraw = true;
	quit = false;
	value = "";
	char buf[1024];

	set_keymap_hints();

	switch (type) {
	case SELECTTAG:
		snprintf(buf, sizeof(buf), _("%s %s - Select Tag"), PROGRAM_NAME, PROGRAM_VERSION);
		break;
	case SELECTFILTER:
		snprintf(buf, sizeof(buf), _("%s %s - Select Filter"), PROGRAM_NAME, PROGRAM_VERSION);
		break;
	default:
		assert(0); // should never happen
	}
	f->set("head", buf);
}

keymap_hint_entry * select_formaction::get_keymap_hint() {
	static keymap_hint_entry hints_tag[] = {
		{ OP_QUIT, _("Cancel") },
		{ OP_OPEN, _("Select Tag") },
		{ OP_NIL, NULL }
	};
	static keymap_hint_entry hints_filter[] = {
		{ OP_QUIT, _("Cancel") },
		{ OP_OPEN, _("Select Filter") },
		{ OP_NIL, NULL }
	};
	switch (type) {
	case SELECTTAG:
		return hints_tag;
	case SELECTFILTER:
		return hints_filter;
	}
	return NULL;
}




}

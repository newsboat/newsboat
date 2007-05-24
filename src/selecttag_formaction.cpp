#include <selecttag_formaction.h>
#include <view.h>
#include <config.h>

#include <sstream>

namespace newsbeuter {

selecttag_formaction::selecttag_formaction(view * vv, std::string formstr)
	: formaction(vv, formstr) { }

selecttag_formaction::~selecttag_formaction() { }

void selecttag_formaction::process_operation(operation op, int /*raw_char*/) {
	switch (op) {
		case OP_QUIT:
			tag = "";
			quit = true;
			break;
		case OP_OPEN: {
				std::string tagposname = f->get("tagposname");
				if (tagposname.length() > 0) {
					std::istringstream posname(tagposname);
					unsigned int pos = 0;
					posname >> pos;
					if (pos < tags.size()) {
						tag = tags[pos];
						quit = true;
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

void selecttag_formaction::prepare() {
	if (do_redraw) {
		std::string code = "{list";
		unsigned int i=0;
		for (std::vector<std::string>::const_iterator it=tags.begin();it!=tags.end();++it,++i) {
			std::ostringstream line;
			char num[32];
			snprintf(num,sizeof(num)," %4d. ", i+1);
			std::string tagstr = num;
			tagstr.append(it->c_str());
			line << "{listitem[" << i << "] text:" << stfl::quote(tagstr.c_str()) << "}";
			code.append(line.str());
		}
		code.append("}");
		f->modify("taglist", "replace_inner", code);
		
		do_redraw = false;
	}
}

void selecttag_formaction::init() {
	do_redraw = true;
	quit = false;
	tag = "";
	char buf[1024];
	snprintf(buf, sizeof(buf), _("%s %s - Select Tag"), PROGRAM_NAME, PROGRAM_VERSION);
	f->set("head", buf);
}

keymap_hint_entry * selecttag_formaction::get_keymap_hint() {
	static keymap_hint_entry hints[] = {
		{ OP_QUIT, _("Cancel") },
		{ OP_OPEN, _("Select Tag") },
		{ OP_NIL, NULL }
	};
	return hints;
}




}

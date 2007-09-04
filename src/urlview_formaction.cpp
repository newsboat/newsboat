#include <urlview_formaction.h>
#include <view.h>
#include <config.h>

#include <sstream>

namespace newsbeuter {

/*
 * The urlview_formaction is probably the simplest dialog of all. It
 * displays a list of URLs, and makes it possible to open the URLs
 * in a browser or to bookmark them.
 */

urlview_formaction::urlview_formaction(view * vv, std::string formstr)
	: formaction(vv, formstr), quit(false) { }

urlview_formaction::~urlview_formaction() {
}

void urlview_formaction::process_operation(operation op) {
	switch (op) {
		case OP_OPEN: 
			{
				std::string posstr = f->get("feedpos");
				if (posstr.length() > 0) {
					std::istringstream is(posstr);
					unsigned int idx;
					is >> idx;
					v->set_status(_("Starting browser..."));
					v->open_in_browser(links[idx].first);
					v->set_status("");
				} else {
					v->show_error(_("No link selected!"));
				}
			}
			break;
		case OP_BOOKMARK: {
				std::string posstr = f->get("feedpos");
				if (posstr.length() > 0) {
					std::istringstream is(posstr);
					unsigned int idx;
					is >> idx;

					this->start_bookmark_qna("", links[idx].first, "");

				} else {
					v->show_error(_("No link selected!"));
				}
			}
			break;
		case OP_QUIT:
			quit = true;
			break;
		default: // nothing
			break;
	}
	if (quit) {
		v->pop_current_formaction();
	}
}

void urlview_formaction::prepare() {
	if (do_redraw) {
		std::string code = "{list";
		unsigned int i=0;
		for (std::vector<linkpair>::iterator it = links.begin(); it != links.end(); ++it, ++i) {
			std::ostringstream os;
			char line[1024];
			snprintf(line,sizeof(line),"%2u  %s",i+1,it->first.c_str());
			os << "{listitem[" << i << "] text:" << stfl::quote(line) << "}";
			code.append(os.str());
		}
		code.append("}");

		f->modify("urls","replace_inner",code);
	}
}

void urlview_formaction::init() {
	v->set_status("");
	char buf[1024];
	snprintf(buf,sizeof(buf),_("%s %s - URLs"), PROGRAM_NAME, PROGRAM_VERSION);
	f->set("head", buf);
	do_redraw = true;
	quit = false;
	set_keymap_hints();
}

keymap_hint_entry * urlview_formaction::get_keymap_hint() {
	static keymap_hint_entry hints[] = {
		{ OP_QUIT, _("Quit") },
		{ OP_OPEN, _("Open in Browser") },
		{ OP_BOOKMARK, _("Save Bookmark") },
		{ OP_NIL, NULL }
	};
	return hints;
}

}

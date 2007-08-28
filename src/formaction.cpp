#include <formaction.h>
#include <view.h>
#include <utils.h>
#include <config.h>
#include <logger.h>
#include <cassert>

namespace newsbeuter {

formaction::formaction(view * vv, std::string formstr) : v(vv), f(0), do_redraw(true) { 
	f = new stfl::form(formstr);
}

void formaction::set_keymap_hints() {
	f->set("help", prepare_keymap_hint(this->get_keymap_hint()));
}

formaction::~formaction() { 
	delete f;
}

stfl::form * formaction::get_form() {
	return f;
}

std::string formaction::prepare_keymap_hint(keymap_hint_entry * hints) {
	std::string keymap_hint;
	for (int i=0;hints[i].op != OP_NIL; ++i) {
		keymap_hint.append(v->get_keys()->getkey(hints[i].op));
		keymap_hint.append(":");
		keymap_hint.append(hints[i].text);
		keymap_hint.append(" ");
	}
	return keymap_hint;	
}

std::string formaction::get_value(const std::string& value) {
	return f->get(value);
}

void formaction::process_op(operation op) {
	switch (op) {
		case OP_INT_PREV_CMDLINEHISTORY:
			f->set("cmdtext", cmdlinehistory.prev());
			break;
		case OP_INT_NEXT_CMDLINEHISTORY:
			f->set("cmdtext", cmdlinehistory.next());
			break;
		case OP_INT_CANCEL_CMDLINE:
			f->modify("lastline","replace","{hbox[lastline] .expand:0 {label[msglabel] .expand:h text[msg]:\"\"}}");
			break;
		case OP_INT_END_CMDLINE: {
				f->set_focus("feeds");
				std::string cmdline = f->get("cmdtext");
				cmdlinehistory.add_line(cmdline);
				GetLogger().log(LOG_DEBUG,"formaction: commandline = `%s'", cmdline.c_str());
				f->modify("lastline","replace","{hbox[lastline] .expand:0 {label[msglabel] .expand:h text[msg]:\"\"}}");
				this->handle_cmdline(cmdline);
			}
			break;
		case OP_CMDLINE:
			f->modify("lastline","replace", "{hbox[lastline] .expand:0 {label .expand:0 text:\":\"}{input[cmdline] on_ESC:cancel-cmdline on_ENTER:end-cmdline on_UP:prev-cmdline-history on_DOWN:next-cmdline-history modal:1 .expand:h text[cmdtext]:\"\"}}");
			f->set_focus("cmdline");
			break;
		case OP_INT_BM_END_URL: {
				bookmark_url = f->get("bmurl");
				std::string replacestr("{hbox[lastline] .expand:0 {label .expand:0 text:\"");
				replacestr.append(_("Title: "));
				replacestr.append("\"}{input[bminput] on_ESC:bm-cancel on_ENTER:bm-end-title modal:1 .expand:h text[bmtext]:");
				replacestr.append(stfl::quote(bookmark_title));
				replacestr.append("}}");
				f->modify("lastline", "replace", replacestr);
				f->set_focus("bminput");
			}
			break;
		case OP_INT_BM_END_TITLE: {
				bookmark_title = f->get("bmtext");
				std::string replacestr("{hbox[lastline] .expand:0 {label .expand:0 text:\"");
				replacestr.append(_("Description: "));
				replacestr.append("\"}{input[bminput] on_ESC:bm-cancel on_ENTER:bm-end-desc modal:1 .expand:h text[bmdesc]:");
				replacestr.append(stfl::quote(bookmark_desc));
				replacestr.append("}}");
				f->modify("lastline", "replace", replacestr);
				f->set_focus("bminput");
			}
			break;
		case OP_INT_BM_END_DESC: {
				bookmark_desc = f->get("bmdesc");
				f->modify("lastline","replace","{hbox[lastline] .expand:0 {label[msglabel] .expand:h text[msg]:\"\"}}");
				v->set_status(_("Saving bookmark..."));
				GetLogger().log(LOG_DEBUG, "formaction::process_op: bookmarking url = `%s' title = `%s' description = `%s'", bookmark_url.c_str(), bookmark_title.c_str(), bookmark_desc.c_str());
				std::string retval = v->get_ctrl()->bookmark(bookmark_url, bookmark_title, bookmark_desc);
				if (retval.length() == 0) {
					v->set_status(_("Saved bookmark."));
				} else {
					v->set_status((std::string(_("Error while saving bookmark: ")) + retval).c_str());
				}
			}
			break;
		case OP_INT_BM_CANCEL: {
				f->modify("lastline","replace","{hbox[lastline] .expand:0 {label[msglabel] .expand:h text[msg]:\"\"}}");
			}
			break;
		default:
			this->process_operation(op);
	}
}

void formaction::handle_cmdline(const std::string& cmdline) {
	std::vector<std::string> tokens = utils::tokenize_quoted(cmdline, " \t=");
	char buf[1024];
	configcontainer * cfg = v->get_cfg();
	assert(cfg != NULL);
	if (tokens.size() > 0) {
		std::string cmd = tokens[0];
		tokens.erase(tokens.begin());
		if (cmd == "set") {
			if (tokens.size()==0) {
				v->show_error(_("usage: set <variable>[=<value>]"));
			} else if (tokens.size()==1) {
				snprintf(buf,sizeof(buf), "  %s=%s", tokens[0].c_str(), cfg->get_configvalue(tokens[0]).c_str());
				v->set_status(buf);
			} else if (tokens.size()==2) {
				cfg->set_configvalue(tokens[0], tokens[1]);
				set_redraw(true); // because some configuration value might have changed something UI-related
			} else {
				v->show_error(_("usage: set <variable>[=<value>]"));
			}
		} else if (cmd == "quit") {
			while (v->formaction_stack_size() > 0) {
				v->pop_current_formaction();
			}
		}
	}
}

}

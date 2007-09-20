#include <formaction.h>
#include <view.h>
#include <utils.h>
#include <config.h>
#include <logger.h>
#include <cassert>

namespace newsbeuter {

history formaction::searchhistory;
history formaction::cmdlinehistory;

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
	/*
	 * This function generates the "keymap hint" line by putting
	 * together the elements of a structure, and looking up the
	 * currently set keybinding so that the "keymap hint" line always
	 * reflects the current configuration.
	 */
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


void formaction::start_cmdline() {
	std::vector<qna_pair> qna;
	qna.push_back(qna_pair(":", ""));
	this->start_qna(qna, OP_INT_END_CMDLINE, &formaction::cmdlinehistory);
}


void formaction::process_op(operation op) {
	switch (op) {
		case OP_CMDLINE: 
			start_cmdline();
			break;
		case OP_INT_CANCEL_QNA:
			f->modify("lastline","replace","{hbox[lastline] .expand:0 {label[msglabel] .expand:h text[msg]:\"\"}}");
			break;
		case OP_INT_QNA_NEXTHIST:
			if (qna_history) {
				f->set("qna_value", qna_history->next());
			}
			break;
		case OP_INT_QNA_PREVHIST:
			if (qna_history) {
				f->set("qna_value", qna_history->prev());
			}
			break;
		case OP_INT_END_QUESTION:
			/*
			 * An answer has been entered, we save the value, and ask the next question.
			 */
			qna_responses.push_back(f->get("qna_value"));
			start_next_question();
			break;
		default:
			this->process_operation(op);
	}
}

void formaction::handle_cmdline(const std::string& cmdline) {
	/*
	 * this is the command line handling that is available on all dialogs.
	 * It is only called when the handle_cmdline() methods of the derived classes
	 * are unable to handle to command line or when the derived class doesn't
	 * implement the handle_cmdline() method by itself.
	 *
	 * It works the same way basically everywhere: first the command line
	 * is tokenized, and then the tokens are looked at.
	 */
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

void formaction::start_qna(const std::vector<qna_pair>& prompts, operation finish_op, history * h) {
	/*
	 * the formaction base class contains a "Q&A" mechanism that makes it possible for all formaction-derived classes to
	 * query the user for 1 or more values, optionally with a history.
	 *
	 * Every question is a prompt (such as "Search for: "), with an default value. These need to be provided as a vector
	 * of (string, string) tuples. What also needs to be provided is the operation that will to be signaled to the 
	 * finished_qna() method when reading all answers is finished, and optionally, a pointer to a history object to support
	 * browsing of the input history. When reading is done, the responses can be found in the qna_responses vector. In this
	 * vector, the first fields corresponds with the first prompt, the second field with the second prompt, etc.
	 */
	qna_prompts = prompts;
	if (qna_responses.size() > 0) {
		qna_responses.erase(qna_responses.begin(), qna_responses.end());
	}
	finish_operation = finish_op;
	qna_history = h;
	start_next_question();
}

void formaction::finished_qna(operation op) {
	switch (op) {
		/*
		 * since bookmarking is available in several formactions, I decided to put this into
		 * the base class so that all derived classes can take advantage of it. We also see
		 * here how the signaling of a finished "Q&A" is handled:
		 * 	- check for the right operation
		 * 	- take the responses
		 * 	- run operation (in this case, save the bookmark)
		 * 	- signal success (or failure) to the user
		 */
		case OP_INT_BM_END: {
				assert(qna_responses.size() == 3 && qna_prompts.size() == 0); // everything must be answered
				v->set_status(_("Saving bookmark..."));
				std::string retval = v->get_ctrl()->bookmark(qna_responses[0], qna_responses[1], qna_responses[2]);
				if (retval.length() == 0) {
					v->set_status(_("Saved bookmark."));
				} else {
					v->set_status((std::string(_("Error while saving bookmark: ")) + retval).c_str());
				}
			}
			break;
		case OP_INT_END_CMDLINE: {
				f->set_focus("feeds");
				std::string cmdline = qna_responses[0];
				formaction::cmdlinehistory.add_line(cmdline);
				GetLogger().log(LOG_DEBUG,"formaction: commandline = `%s'", cmdline.c_str());
				this->handle_cmdline(cmdline);
			}
			break;
		default:
			break;
	}
}


void formaction::start_bookmark_qna(const std::string& default_title, const std::string& default_url, const std::string& default_desc) {
	GetLogger().log(LOG_DEBUG, "formaction::start_bookmark_qna: OK, starting bookmark Q&A...");
	std::vector<qna_pair> prompts;

	prompts.push_back(qna_pair(_("URL: "), default_url));
	prompts.push_back(qna_pair(_("Title: "), default_title));
	prompts.push_back(qna_pair(_("Description: "), default_desc));

	start_qna(prompts, OP_INT_BM_END);
}

void formaction::start_next_question() {
	/*
	 * If there is one more prompt to be presented to the user, set it up.
	 */
	if (qna_prompts.size() > 0) {
		std::string replacestr("{hbox[lastline] .expand:0 {label .expand:0 text:");
		replacestr.append(stfl::quote(qna_prompts[0].first));
		replacestr.append("}{input[qnainput] on_ESC:cancel-qna on_UP:qna-prev-history on_DOWN:qna-next-history on_ENTER:end-question modal:1 .expand:h text[qna_value]:");
		replacestr.append(stfl::quote(qna_prompts[0].second));
		replacestr.append("}}");
		qna_prompts.erase(qna_prompts.begin());
		f->modify("lastline", "replace", replacestr);
		f->set_focus("qnainput");
	} else {
	/* 
	 * If there are no more prompts, restore the last line with the usual label, and signal the end of the "Q&A" to the finished_qna() method.
	 */
		f->modify("lastline","replace","{hbox[lastline] .expand:0 {label[msglabel] .expand:h text[msg]:\"\"}}");
		this->finished_qna(finish_operation);
	}
}


}

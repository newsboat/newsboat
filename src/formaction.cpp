#include <formaction.h>
#include <view.h>
#include <utils.h>
#include <config.h>
#include <logger.h>
#include <cassert>
#include <exceptions.h>
#include <algorithm>

namespace newsbeuter {

history formaction::searchhistory;
history formaction::cmdlinehistory;

formaction::formaction(view * vv, std::string formstr) : v(vv), f(new stfl::form(formstr)), do_redraw(true) { 
	if (v) {
		if (v->get_cfg()->get_configvalue_as_bool("show-keymap-hint") == false) {
			f->set("showhint", "0");
		}
	}
	valid_cmds.push_back("set");
	valid_cmds.push_back("quit");
	valid_cmds.push_back("source");
	valid_cmds.push_back("dumpconfig");
	valid_cmds.push_back("dumpform");
}

void formaction::set_keymap_hints() {
	f->set("help", prepare_keymap_hint(this->get_keymap_hint()));
}

void formaction::recalculate_form() {
	f->run(-3);
}

formaction::~formaction() { }

std::tr1::shared_ptr<stfl::form> formaction::get_form() {
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
		keymap_hint.append(v->get_keys()->getkey(hints[i].op, this->id()));
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
	v->inside_cmdline(true);
	this->start_qna(qna, OP_INT_END_CMDLINE, &formaction::cmdlinehistory);
}


void formaction::process_op(operation op, bool automatic, std::vector<std::string> * args) {
	switch (op) {
		case OP_REDRAW:
			LOG(LOG_DEBUG, "formaction::process_op: redrawing screen");
			stfl::reset();
			break;
		case OP_CMDLINE: 
			start_cmdline();
			break;
		case OP_INT_SET:
			if (automatic) {
				std::string cmdline = "set ";
				if (args) {
					for (std::vector<std::string>::iterator it=args->begin();it!=args->end();++it) {
						cmdline.append(utils::strprintf("%s ", stfl::quote(*it).c_str()));
					}
				}
				LOG(LOG_DEBUG, "formaction::process_op: running commandline `%s'", cmdline.c_str());
				this->handle_cmdline(cmdline);
			} else {
				LOG(LOG_WARN, "formaction::process_op: got OP_INT_SET, but not automatic");
			}
			break;
		case OP_INT_CANCEL_QNA:
			f->modify("lastline","replace","{hbox[lastline] .expand:0 {label[msglabel] .expand:h text[msg]:\"\"}}");
			v->inside_qna(false);
			v->inside_cmdline(false);
			break;
		case OP_INT_QNA_NEXTHIST:
			if (qna_history) {
				std::string entry = qna_history->next();
				f->set("qna_value", entry);
				f->set("qna_value_pos", utils::to_s(entry.length()));
			}
			break;
		case OP_INT_QNA_PREVHIST:
			if (qna_history) {
				std::string entry = qna_history->prev();
				f->set("qna_value", entry);
				f->set("qna_value_pos", utils::to_s(entry.length()));
			}
			break;
		case OP_INT_END_QUESTION:
			/*
			 * An answer has been entered, we save the value, and ask the next question.
			 */
			qna_responses.push_back(f->get("qna_value"));
			start_next_question();
			break;
		case OP_VIEWDIALOGS:
			v->view_dialogs();
			break;
		case OP_NEXTDIALOG:
			v->goto_next_dialog();
			break;
		case OP_PREVDIALOG:
			v->goto_prev_dialog();
			break;
		default:
			this->process_operation(op, automatic, args);
	}
}

std::vector<std::string> formaction::get_suggestions(const std::string& fragment) {
	LOG(LOG_DEBUG, "formaction::get_suggestions: fragment = %s", fragment.c_str());
	std::vector<std::string> result;
	// first check all formaction command suggestions
	for (std::vector<std::string>::iterator it=valid_cmds.begin();it!=valid_cmds.end();++it) {
		LOG(LOG_DEBUG, "formaction::get_suggestions: extracted part: %s", it->substr(0, fragment.length()).c_str());
		if (it->substr(0, fragment.length()) == fragment) {
			LOG(LOG_DEBUG, "...and it matches.");
			result.push_back(*it);
		}
	}
	if (result.empty()) {
		std::vector<std::string> tokens = utils::tokenize_quoted(fragment, " \t=");
		if (tokens.size() >= 1) {
			if (tokens[0] == "set") {
				if (tokens.size() < 3) {
					std::vector<std::string> variable_suggestions;
					std::string variable_fragment;
					if (tokens.size() > 1)
						variable_fragment = tokens[1];
					variable_suggestions = v->get_cfg()->get_suggestions(variable_fragment);
					for (std::vector<std::string>::iterator it=variable_suggestions.begin();it!=variable_suggestions.end();++it) {
						std::string line = fragment + it->substr(variable_fragment.length(), it->length()-variable_fragment.length());
						result.push_back(line);
						LOG(LOG_DEBUG, "formaction::get_suggestions: suggested %s", line.c_str());
					}
				}
			}
		}
	}
	LOG(LOG_DEBUG, "formaction::get_suggestions: %u suggestions", result.size());
	return result;
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
	configcontainer * cfg = v->get_cfg();
	assert(cfg != NULL);
	if (!tokens.empty()) {
		std::string cmd = tokens[0];
		tokens.erase(tokens.begin());
		if (cmd == "set") {
			if (tokens.empty()) {
				v->show_error(_("usage: set <variable>[=<value>]"));
			} else if (tokens.size()==1) {
				std::string var = tokens[0];
				if (var.length() > 0) {
					if (var[var.length()-1] == '!') {
						var.erase(var.length()-1);
						cfg->toggle(var);
						set_redraw(true);
					} else if (var[var.length()-1] == '&') {
						var.erase(var.length()-1);
						cfg->reset_to_default(var);
						set_redraw(true);
					}
					v->set_status(utils::strprintf("  %s=%s", var.c_str(), utils::quote_if_necessary(cfg->get_configvalue(var)).c_str()));
				}
			} else if (tokens.size()==2) {
				std::string result = configparser::evaluate_backticks(tokens[1]);
				utils::trim_end(result);
				cfg->set_configvalue(tokens[0], result);
				set_redraw(true); // because some configuration value might have changed something UI-related
			} else {
				v->show_error(_("usage: set <variable>[=<value>]"));
			}
		} else if (cmd == "quit") {
			while (v->formaction_stack_size() > 0) {
				v->pop_current_formaction();
			}
		} else if (cmd == "source") {
			if (tokens.empty()) {
				v->show_error(_("usage: source <file> [...]"));
			} else {
				for (std::vector<std::string>::iterator it=tokens.begin();it!=tokens.end();++it) {
					try {
						v->get_ctrl()->load_configfile(utils::resolve_tilde(*it));
					} catch (const configexception& ex) {
						v->show_error(ex.what());
						break;
					}
				}
			}
		} else if (cmd == "dumpconfig") {
			if (tokens.size()!=1) {
				v->show_error(_("usage: dumpconfig <file>"));
			} else {
				v->get_ctrl()->dump_config(utils::resolve_tilde(tokens[1]));
				v->show_error(utils::strprintf(_("Saved configuration to %s"), tokens[1].c_str()));
			}
		} else if (cmd == "dumpform") {
			v->dump_current_form();
		} else {
			v->show_error(utils::strprintf(_("Not a command: %s"), cmdline.c_str()));
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
	qna_responses.clear();
	finish_operation = finish_op;
	qna_history = h;
	v->inside_qna(true);
	start_next_question();
}

void formaction::finished_qna(operation op) {
	v->inside_qna(false);
	v->inside_cmdline(false);
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
					v->set_status(std::string(_("Error while saving bookmark: ")) + retval);
					LOG(LOG_DEBUG, "formaction::finished_qna: error while saving bookmark, retval = `%s'", retval.c_str());
				}
			}
			break;
		case OP_INT_END_CMDLINE: {
				f->set_focus("feeds");
				std::string cmdline = qna_responses[0];
				formaction::cmdlinehistory.add_line(cmdline);
				LOG(LOG_DEBUG,"formaction: commandline = `%s'", cmdline.c_str());
				this->handle_cmdline(cmdline);
			}
			break;
		default:
			break;
	}
}


void formaction::start_bookmark_qna(const std::string& default_title, const std::string& default_url, const std::string& default_desc) {
	LOG(LOG_DEBUG, "formaction::start_bookmark_qna: starting bookmark Q&A... default_title = %s default_url = %s default_desc = %s", default_title.c_str(), default_url.c_str(), default_desc.c_str());
	std::vector<qna_pair> prompts;

	std::string new_title = "";
	prompts.push_back(qna_pair(_("URL: "), default_url));
	if(default_title.empty()) { // call the function to figure out title from url only if the default_title is no good
		new_title = make_title(default_url);
		prompts.push_back(qna_pair(_("Title: "), new_title));
	}
	else {
		prompts.push_back(qna_pair(_("Title: "), default_title));
	}
	prompts.push_back(qna_pair(_("Description: "), default_desc));

	start_qna(prompts, OP_INT_BM_END);
}

std::string formaction::make_title(const std::string& const_url) {
	/* Sometimes it is possible to construct the title from the URL
	 * This attempts to do just that. eg: http://domain.com/story/yy/mm/dd/title-with-dashes?a=b
	*/
	std::string url = (std::string&) const_url;
	std::string::size_type pos_of_slash = url.find_last_of('/', (int)url.length()-2); 		// get to the final part of the URI's path, catering for situation where last char is '/'
	if (url[url.length()-1] == '/')
		url.erase((int)url.length()-1);
	std::string path=url.substr(pos_of_slash+1); 				// extract just the juicy part 'title-with-dashes?a=b'
	std::string::size_type pos_of_qmrk = path.find_first_of('?'); 		// find where query part of URI starts
        std::string title = path.substr(0,pos_of_qmrk); 			//throw away the query part 'title-with-dashes'
	std::replace(title.begin(), title.end(), '-', ' ');			// 'title with dashes'
        if (title.at(0)>= 'a' && title.at(0)<= 'z') title[0] -= 'a' - 'A';	//'Title with dashes'
	return title;
}

void formaction::start_next_question() {
	/*
	 * If there is one more prompt to be presented to the user, set it up.
	 */
	if (qna_prompts.size() > 0) {
		std::string replacestr("{hbox[lastline] .expand:0 {label .expand:0 text:");
		replacestr.append(stfl::quote(qna_prompts[0].first));
		replacestr.append("}{input[qnainput] on_ESC:cancel-qna on_UP:qna-prev-history on_DOWN:qna-next-history on_ENTER:end-question modal:1 .expand:h @bind_home:** @bind_end:** text[qna_value]:");
		replacestr.append(stfl::quote(qna_prompts[0].second));
		replacestr.append(" pos[qna_value_pos]:");
		replacestr.append(utils::to_s(qna_prompts[0].second.length()));
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

void formaction::load_histories(const std::string& searchfile, const std::string& cmdlinefile) {
	searchhistory.load_from_file(searchfile);
	cmdlinehistory.load_from_file(cmdlinefile);
}

void formaction::save_histories(const std::string& searchfile, const std::string& cmdlinefile, unsigned int limit) {
	searchhistory.save_to_file(searchfile, limit);
	cmdlinehistory.save_to_file(cmdlinefile, limit);
}


}

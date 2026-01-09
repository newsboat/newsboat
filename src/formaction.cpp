#include "formaction.h"

#include <cassert>
#include <cinttypes>
#include <ncurses.h>

#include "config.h"
#include "configexception.h"
#include "controller.h"
#include "logger.h"
#include "strprintf.h"
#include "textviewwidget.h"
#include "utils.h"
#include "view.h"

namespace newsboat {

History FormAction::searchhistory;
History FormAction::cmdlinehistory;

FormAction::FormAction(View& vv, std::string formstr, ConfigContainer* cfg)
	: v(vv)
	, cfg(cfg)
	, f(formstr)
	, do_redraw(true)
	, head_line(f, "head")
	, msg_line(f, "msg")
	, qna_prompt_line(f, "qna_prompt")
	, qna_input(f, "qna")
	, qna_finish_operation(QnaFinishAction::None)
	, qna_history(nullptr)
	, tab_count(0)
{
	if (cfg->get_configvalue_as_bool("show-keymap-hint") == false) {
		set_value("showhint", "0");
	}
	if (cfg->get_configvalue_as_bool("show-title-bar") == false) {
		set_value("showtitle", "0");
	}
	if (cfg->get_configvalue_as_bool("swap-title-and-hints") ==
		true) {
		std::string hints = f.dump("hints", "", 0);
		std::string title = f.dump("title", "", 0);
		f.modify("title", "replace", "label[swap-title]");
		f.modify("hints", "replace", "label[swap-hints]");
		f.modify("swap-title", "replace", hints);
		f.modify("swap-hints", "replace", title);
	}
	valid_cmds.push_back("set");
	valid_cmds.push_back("quit");
	valid_cmds.push_back("source");
	valid_cmds.push_back("dumpconfig");
	valid_cmds.push_back("exec");
}

void FormAction::report_unhandled_operation(Operation op)
{
	set_status(strprintf::fmt(_("Operation %s not handled in %s"), KeyMap::get_op_name(op),
			dialog_name(id())));
}

void FormAction::set_keymap_hints()
{
	set_value("help", v.get_keymap()->prepare_keymap_hint(this->get_keymap_hint(),
			this->id()).stfl_quoted());
}

std::string FormAction::get_value(const std::string& name)
{
	return f.get(name);
}

void FormAction::set_value(const std::string& name, const std::string& value)
{
	f.set(name, value);
}

void FormAction::set_status(const std::string& text)
{
	msg_line.set_text(text);
}

void FormAction::draw_form()
{
	f.run(-1);
}

std::string FormAction::draw_form_wait_for_event(unsigned int timeout)
{
	return f.run(timeout);
}

void FormAction::recalculate_widget_dimensions()
{
	f.run(-3);
}

void FormAction::start_cmdline(std::string default_value)
{
	std::vector<QnaPair> qna;
	qna.push_back(QnaPair(":", default_value));
	v.inside_cmdline(true);
	this->start_qna(qna, QnaFinishAction::RunCmdLine, &FormAction::cmdlinehistory);
}

bool FormAction::process_op(Operation op,
	const std::vector<std::string>& args,
	BindingType bindingType)
{
	switch (op) {
	case OP_REDRAW:
		LOG(Level::DEBUG, "FormAction::process_op: redrawing screen");
		Stfl::reset();
		break;
	case OP_CMDLINE:
		start_cmdline();
		break;
	case OP_SET:
		switch (bindingType) {
		case BindingType::Bind:
		case BindingType::Macro:
			if (args.size() == 2) {
				const std::string key = args.at(0);
				const std::string value = args.at(1);
				cfg->set_configvalue(key, value);
				set_redraw(true);
				return true;
			}
			if (args.size() == 1) {
				if (handle_single_argument_set(args.at(0))) {
					return true;
				}
			}
			v.get_statusline().show_error(_("usage: set <config-option> <value>"));
			return false;
		case BindingType::BindKey:
			LOG(Level::WARN,
				"FormAction::process_op: got OP_SET, but from a bind-key which does not support arguments");
			break;
		}
		break;
	case OP_VIEWDIALOGS:
		v.view_dialogs();
		break;
	case OP_NEXTDIALOG:
		v.goto_next_dialog();
		break;
	case OP_PREVDIALOG:
		v.goto_prev_dialog();
		break;
	default:
		return this->process_operation(op, args, bindingType);
	}
	return true;
}

std::vector<std::string> FormAction::get_suggestions(
	const std::string& fragment)
{
	LOG(Level::DEBUG,
		"FormAction::get_suggestions: fragment = %s",
		fragment);
	std::vector<std::string> result;
	// first check all formaction command suggestions
	for (const auto& cmd : valid_cmds) {
		LOG(Level::DEBUG,
			"FormAction::get_suggestions: extracted part: %s",
			cmd.substr(0, fragment.length()));
		if (cmd.substr(0, fragment.length()) == fragment) {
			LOG(Level::DEBUG, "...and it matches.");
			result.push_back(cmd);
		}
	}
	if (result.empty()) {
		std::vector<std::string> tokens =
			utils::tokenize_quoted(fragment, " \t=");
		if (tokens.size() >= 1) {
			if (tokens[0] == "set") {
				if (tokens.size() < 3) {
					std::vector<std::string>
					variable_suggestions;
					std::string variable_fragment;
					if (tokens.size() > 1) {
						variable_fragment = tokens[1];
					}
					variable_suggestions =
						cfg->get_suggestions(
							variable_fragment);
					for (const auto& suggestion :
						variable_suggestions) {
						std::string line = fragment +
							suggestion.substr(
								variable_fragment
								.length(),
								suggestion.length() -
								variable_fragment
								.length());
						result.push_back(line);
						LOG(Level::DEBUG,
							"FormAction::get_"
							"suggestions: "
							"suggested %s",
							line);
					}
				}
			} else if (tokens[0] == "exec") {
				if (tokens.size() <= 2) {
					const std::string start = (tokens.size() == 2) ? tokens[1] : "";
					const std::vector<KeyMapDesc> descs = v.get_keymap()->get_keymap_descriptions(
							this->id()
						);
					for (const KeyMapDesc& desc: descs) {
						const std::string cmd = desc.cmd;
						if (cmd.rfind(start, 0) == 0) {
							result.push_back(std::string("exec ") + cmd);
						}
					}
				}
			}
		}
	}
	LOG(Level::DEBUG,
		"FormAction::get_suggestions: %" PRIu64 " suggestions",
		static_cast<uint64_t>(result.size()));
	return result;
}

void FormAction::handle_cmdline(const std::string& cmdline)
{
	/*
	 * this is the command line handling that is available on all dialogs.
	 * It is only called when the handle_cmdline() methods of the derived
	 * classes are unable to handle to command line or when the derived
	 * class doesn't implement the handle_cmdline() method by itself.
	 *
	 * It works the same way basically everywhere: first the command line
	 * is tokenized, and then the tokens are looked at.
	 */
	constexpr auto delimiters = " \t=";
	const auto command = FormAction::parse_command(cmdline, delimiters);
	assert(cfg != nullptr);
	handle_parsed_command(command);
}

void FormAction::handle_set(const std::vector<std::string>& args)
{
	if (args.size() == 1) {
		if (handle_single_argument_set(args[0])) {
			return;
		}
		v.get_statusline().show_message(strprintf::fmt("  %s=%s",
				args[0],
				utils::quote_if_necessary(cfg->get_configvalue(args[0]))));
	} else if (args.size() == 2) {
		std::string value = ConfigParser::evaluate_backticks(args[1]);
		utils::trim_end(value);
		const auto& key = args[0];
		const auto result = cfg->set_configvalue(key, value);
		if (!result) {
			v.get_statusline().show_error(strprintf::fmt(_("error setting '%s' to '%s': %s"),
					key,
					value,
					result.error()));
		}
		// because some configuration value might have changed something UI-related
		set_redraw(true);
	} else {
		v.get_statusline().show_error(
			_("usage: set <variable>[=<value>]"));
	}
}

void FormAction::handle_quit()
{
	while (v.formaction_stack_size() > 0) {
		v.pop_current_formaction();
	}
}

void FormAction::handle_source(const std::vector<std::string>& args)
{
	if (args.empty()) {
		v.get_statusline().show_error(_("usage: source <file> [...]"));
	} else {
		for (const auto& param : args) {
			try {
				const auto path = Filepath::from_locale_string(param);
				v.get_ctrl().load_configfile(utils::resolve_tilde(path));
			} catch (const ConfigException& ex) {
				v.get_statusline().show_error(ex.what());
				break;
			}
		}
	}
}

void FormAction::handle_dumpconfig(const std::vector<std::string>& args)
{
	if (args.size() != 1) {
		v.get_statusline().show_error(_("usage: dumpconfig <file>"));
	} else {
		const auto path = Filepath::from_locale_string(args[0]);
		v.get_ctrl().dump_config(utils::resolve_tilde(path));
		v.get_statusline().show_message(strprintf::fmt(
				_("Saved configuration to %s"),
				args[0]));
	}
}

void FormAction::handle_exec(const std::vector<std::string>& args)
{
	if (args.size() != 1) {
		v.get_statusline().show_error(_("usage: exec <operation>"));
	} else {
		const auto op = v.get_keymap()->get_opcode(args[0]);
		if (op != OP_NIL) {
			std::vector<std::string> args;
			process_op(op, args);
		} else {
			v.get_statusline().show_error(_("Operation not found"));
		}
	}
}

void FormAction::handle_parsed_command(const Command& command)
{
	switch (command.type) {
	case CommandType::SET:
		handle_set(command.args);
		break;
	case CommandType::QUIT:
		handle_quit();
		break;
	case CommandType::SOURCE:
		handle_source(command.args);
		break;
	case CommandType::DUMPCONFIG:
		handle_dumpconfig(command.args);
		break;
	case CommandType::EXEC:
		handle_exec(command.args);
		break;
	case CommandType::UNKNOWN:
		v.get_statusline().show_error(strprintf::fmt(_("Not a command: %s"), command.args[0]));
		break;
	case CommandType::INVALID:
		break;
	default:
		break;
	}
}

bool FormAction::handle_list_operations(ListWidget& list, Operation op)
{
	switch (op) {
	case OP_SK_UP:
		list.move_up(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_SK_DOWN:
		list.move_down(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_SK_HOME:
		list.move_to_first();
		break;
	case OP_SK_END:
		list.move_to_last();
		break;
	case OP_SK_PGUP:
		list.move_page_up(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_SK_PGDOWN:
		list.move_page_down(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_SK_HALF_PAGE_UP:
		list.scroll_halfpage_up(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_SK_HALF_PAGE_DOWN:
		list.scroll_halfpage_down(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	default:
		report_unhandled_operation(op);
		return false;
	}
	return true;
}

bool FormAction::handle_textview_operations(TextviewWidget& textview, Operation op)
{
	switch (op) {
	case OP_SK_UP:
		textview.scroll_up();
		break;
	case OP_SK_DOWN:
		textview.scroll_down();
		break;
	case OP_SK_HOME:
		textview.scroll_to_top();
		break;
	case OP_SK_END:
		textview.scroll_to_bottom();
		break;
	case OP_SK_PGUP:
		textview.scroll_page_up();
		break;
	case OP_SK_PGDOWN:
		textview.scroll_page_down();
		break;
	case OP_SK_HALF_PAGE_UP:
		textview.scroll_halfpage_up();
		break;
	case OP_SK_HALF_PAGE_DOWN:
		textview.scroll_halfpage_down();
		break;
	default:
		report_unhandled_operation(op);
		return false;
	}
	return true;
}

bool FormAction::handle_single_argument_set(std::string argument)
{
	if (argument.size() >= 1 && argument.back() == '!') {
		argument.pop_back();
		cfg->toggle(argument);
		set_redraw(true);
		return true;
	}
	if (argument.size() >= 1 && argument.back() == '&') {
		argument.pop_back();
		cfg->reset_to_default(argument);
		set_redraw(true);
		return true;
	}
	return false;
}

void FormAction::start_qna(const std::vector<QnaPair>& prompts,
	QnaFinishAction finish_op,
	History* h)
{
	/*
	 * the formaction base class contains a "Q&A" mechanism that makes it
	 * possible for all formaction-derived classes to query the user for 1
	 * or more values, optionally with a history.
	 *
	 * Every question is a prompt (such as "Search for: "), with an default
	 * value. These need to be provided as a vector of (string, string)
	 * tuples. What also needs to be provided is the operation that will to
	 * be signaled to the finished_qna() method when reading all answers is
	 * finished, and optionally, a pointer to a history object to support
	 * browsing of the input history. When reading is done, the responses
	 * can be found in the qna_responses vector. In this vector, the first
	 * fields corresponds with the first prompt, the second field with the
	 * second prompt, etc.
	 */
	qna_prompts = prompts;
	qna_responses.clear();
	qna_finish_operation = finish_op;
	qna_history = h;
	v.inside_qna(true);
	start_next_question();
}

void FormAction::finish_qna_question()
{
	qna_responses.push_back(qna_input.get_value());
	start_next_question();
}

void FormAction::cancel_qna()
{
	LOG(Level::DEBUG, "FormAction::cancel_qna");

	qna_prompt_line.hide();
	qna_input.hide();
	msg_line.show();
	msg_line.set_text("");

	f.set_focus(main_widget());

	v.inside_qna(false);
	v.inside_cmdline(false);
}

void FormAction::qna_next_history()
{
	if (qna_history) {
		std::string entry = qna_history->next_line();
		qna_input.set_value(entry);
		qna_input.set_position(entry.length());
	}
}

void FormAction::qna_previous_history()
{
	if (qna_history) {
		std::string entry = qna_history->previous_line();
		qna_input.set_value(entry);
		qna_input.set_position(entry.length());
	}
}

void FormAction::clear_line()
{
	qna_input.set_value("");
	qna_input.set_position(0);
}

void FormAction::clear_eol()
{
	unsigned int pos = qna_input.get_position();
	std::string val = qna_input.get_value();
	val.erase(pos, val.length());
	qna_input.set_value(val);
	qna_input.set_position(val.length());
	LOG(Level::DEBUG, "View::clear_eol: cleared to end of line");
}

void FormAction::delete_word()
{
	std::string::size_type curpos = qna_input.get_position();
	std::string val = qna_input.get_value();
	std::string::size_type firstpos = curpos;
	LOG(Level::DEBUG, "View::delete_word: before val = %s", val);
	if (firstpos >= val.length() || ::isspace(val[firstpos])) {
		if (firstpos != 0 && firstpos >= val.length()) {
			firstpos = val.length() - 1;
		}
		while (firstpos > 0 && ::isspace(val[firstpos])) {
			--firstpos;
		}
	}
	while (firstpos > 0 && !::isspace(val[firstpos])) {
		--firstpos;
	}
	if (firstpos != 0) {
		firstpos++;
	}
	val.erase(firstpos, curpos - firstpos);
	LOG(Level::DEBUG, "View::delete_word: after val = %s", val);
	qna_input.set_value(val);
	qna_input.set_position(firstpos);
}

void FormAction::handle_cmdline_completion()
{
	std::string fragment = qna_input.get_value();
	if (fragment != last_fragment || fragment == "") {
		last_fragment = fragment;
		suggestions = get_suggestions(fragment);
		tab_count = 0;
	}
	tab_count++;
	std::string suggestion;
	switch (suggestions.size()) {
	case 0:
		LOG(Level::DEBUG, "FormAction::handle_cmdline_completion: found no suggestion for `%s'",
			fragment);
		// direct call to ncurses - we beep to signal that there is no suggestion available, just like vim
		::beep();
		return;
	case 1:
		suggestion = suggestions[0];
		break;
	default:
		suggestion = suggestions[(tab_count - 1) % suggestions.size()];
		break;
	}
	qna_input.set_value(suggestion);
	qna_input.set_position(suggestion.length());
	last_fragment = suggestion;
}

void FormAction::handle_qna_event(std::string event, bool inside_cmd)
{
	if (event == "ESC") {
		cancel_qna();
	} else if (inside_cmd && event == "TAB") {
		handle_cmdline_completion();
	} else if (event == "UP") {
		qna_previous_history();
	} else if (event == "DOWN") {
		qna_next_history();
	} else if (event == "ENTER") {
		finish_qna_question();
	} else if (event == "^U") {
		clear_line();
	} else if (event == "^K") {
		clear_eol();
	} else if (event == "^G") {
		cancel_qna();
	} else if (event == "^W") {
		delete_word();
	}
}

void FormAction::finished_qna(QnaFinishAction op)
{
	v.inside_qna(false);
	v.inside_cmdline(false);
	switch (op) {
	/*
	 * since bookmarking is available in several formactions, I decided to
	 * put this into the base class so that all derived classes can take
	 * advantage of it. We also see here how the signaling of a finished
	 * "Q&A" is handled:
	 * 	- check for the right operation
	 * 	- take the responses
	 * 	- run operation (in this case, save the bookmark)
	 * 	- signal success (or failure) to the user
	 */
	case QnaFinishAction::Bookmark: {
		assert(qna_responses.size() == 4 &&
			qna_prompts.size() == 0); // everything must be answered
		v.get_statusline().show_message(_("Saving bookmark..."));
		std::string retval = bookmark(qna_responses[0],
				qna_responses[1],
				qna_responses[2],
				qna_responses[3]);
		if (retval.length() == 0) {
			v.get_statusline().show_message(_("Saved bookmark."));
		} else {
			v.get_statusline().show_message(
				_s("Error while saving bookmark: ") + retval);
			LOG(Level::DEBUG,
				"FormAction::finished_qna: error while saving "
				"bookmark, retval = `%s'",
				retval);
		}
	}
	break;
	case QnaFinishAction::RunCmdLine: {
		f.set_focus(main_widget());
		std::string cmdline = qna_responses[0];
		FormAction::cmdlinehistory.add_line(cmdline);
		LOG(Level::DEBUG, "FormAction: commandline = `%s'", cmdline);
		this->handle_cmdline(cmdline);
	}
	break;
	default:
		break;
	}
}

void FormAction::set_title(const std::string& title)
{
	head_line.set_text(title);
}

void FormAction::start_bookmark_qna(const std::string& default_title,
	const std::string& default_url,
	const std::string& default_feed_title)
{
	LOG(Level::DEBUG,
		"FormAction::start_bookmark_qna: starting bookmark Q&A... "
		"default_title = %s default_url = %s "
		"default_feed_title = %s",
		default_title,
		default_url,
		default_feed_title);
	std::vector<QnaPair> prompts;

	bool is_bm_autopilot = cfg->get_configvalue_as_bool("bookmark-autopilot");
	prompts.push_back(QnaPair(_("URL: "), default_url));
	// call the function to figure out title from url only if the default_title is no good
	if (default_title.empty()) {
		prompts.push_back(QnaPair(_("Title: "), utils::make_title(default_url)));
	} else {
		prompts.push_back(QnaPair(_("Title: "), utils::utf8_to_locale(default_title)));
	}
	prompts.push_back(QnaPair(_("Description: "), ""));
	prompts.push_back(QnaPair(_("Feed title: "), default_feed_title));

	if (is_bm_autopilot) { // If bookmarking is set to autopilot don't prompt for url, title, desc
		std::string title;
		if (default_title.empty()) {
			title = utils::make_title(default_url); // try to make the title from url
		} else {
			// assignment just to make the call to bookmark() below easier
			title = utils::utf8_to_locale(default_title);
		}

		// if url or title is missing, abort autopilot and ask user
		if (default_url.empty() || title.empty()) {
			start_qna(prompts, QnaFinishAction::Bookmark);
		} else {
			v.get_statusline().show_message(_("Saving bookmark on autopilot..."));
			std::string retval = bookmark(default_url,
					title,
					"",
					default_feed_title);
			if (retval.length() == 0) {
				v.get_statusline().show_message(_("Saved bookmark."));
			} else {
				v.get_statusline().show_message(
					_s("Error while saving bookmark: ") +
					retval);
				LOG(Level::DEBUG,
					"FormAction::finished_qna: error while "
					"saving bookmark, retval = `%s'",
					retval);
			}
		}
	} else {
		start_qna(prompts, QnaFinishAction::Bookmark);
	}
}

Command FormAction::parse_command(const std::string& input,
	std::string delimiters)
{
	auto tokens = utils::tokenize_quoted(input, delimiters);
	if (tokens.empty()) {
		return Command(CommandType::INVALID);
	} else {
		auto cmd_name = tokens.front();
		tokens.erase(tokens.begin());
		if (cmd_name == "set") {
			return Command(CommandType::SET, std::move(tokens));
		} else if (cmd_name == "q" || cmd_name == "quit") {
			return Command(CommandType::QUIT, std::move(tokens));
		} else if (cmd_name == "source") {
			return Command(CommandType::SOURCE, std::move(tokens));
		} else if (cmd_name == "dumpconfig") {
			return Command(CommandType::DUMPCONFIG, std::move(tokens));
		} else if (cmd_name == "exec") {
			return Command(CommandType::EXEC, std::move(tokens));
		} else if (cmd_name == "tag") {
			return Command(CommandType::TAG, std::move(tokens));
		} else if (cmd_name == "goto") {
			return Command(CommandType::GOTO, std::move(tokens));
		} else if (cmd_name == "save") {
			return Command(CommandType::SAVE, std::move(tokens));
		} else {
			tokens.insert(tokens.begin(), std::move(cmd_name));
			return Command(CommandType::UNKNOWN, std::move(tokens));
		}
	}
}

void FormAction::start_next_question()
{
	/*
	 * If there is one more prompt to be presented to the user, set it up.
	 */
	if (qna_prompts.size() > 0) {
		qna_prompt_line.set_text(qna_prompts[0].first);
		qna_input.set_value(qna_prompts[0].second);

		qna_prompt_line.show();
		qna_input.show();
		msg_line.hide();

		f.set_focus("qnainput");

		// Set position to 0 and back to ensure that the text is visible
		draw_form();
		qna_input.set_position(qna_prompts[0].second.length());

		qna_prompts.erase(qna_prompts.begin());
	} else {
		/*
		 * If there are no more prompts, restore the last line with the
		 * usual label, and signal the end of the "Q&A" to the
		 * finished_qna() method.
		 */
		qna_prompt_line.hide();
		qna_input.hide();
		msg_line.show();
		msg_line.set_text("");

		f.set_focus(main_widget());

		this->finished_qna(qna_finish_operation);
	}
}

void FormAction::load_histories(const Filepath& searchfile,
	const Filepath& cmdlinefile)
{
	searchhistory.load_from_file(searchfile);
	cmdlinehistory.load_from_file(cmdlinefile);
}

void FormAction::save_histories(const Filepath& searchfile,
	const Filepath& cmdlinefile,
	unsigned int limit)
{
	searchhistory.save_to_file(searchfile, limit);
	cmdlinehistory.save_to_file(cmdlinefile, limit);
}

std::string FormAction::bookmark(const std::string& url,
	const std::string& title,
	const std::string& description,
	const std::string& feed_title)
{
	std::string bookmark_cmd = cfg->get_configvalue("bookmark-cmd");
	bool is_interactive =
		cfg->get_configvalue_as_bool("bookmark-interactive");
	if (bookmark_cmd.length() > 0) {
		std::string cmdline = strprintf::fmt("%s '%s' '%s' '%s' '%s'",
				bookmark_cmd,
				utils::replace_all(url, "'", "%27"),
				utils::replace_all(title, "'", "%27"),
				utils::replace_all(description, "'", "%27"),
				utils::replace_all(feed_title, "'", "%27"));

		LOG(Level::DEBUG, "FormAction::bookmark: cmd = %s", cmdline);

		if (is_interactive) {
			v.push_empty_formaction();
			Stfl::reset();
			utils::run_interactively(cmdline, "FormAction::bookmark");
			v.drop_queued_input();
			v.pop_current_formaction();
			return "";
		} else {
			const char* my_argv[4];
			my_argv[0] = "/bin/sh";
			my_argv[1] = "-c";
			my_argv[2] = cmdline.c_str();
			my_argv[3] = nullptr;
			return utils::run_program(my_argv, "");
		}
	} else {
		return _(
				"bookmarking support is not configured. Please set the "
				"configuration variable `bookmark-cmd' accordingly.");
	}
}

} // namespace newsboat

#include <itemlist.h>
#include <itemview.h>
#include <help.h>
#include <filebrowser.h>
#include <urlview.h>
#include <selecttag.h>
#include <feedlist.h>
#include <dialogs.h>
#include <formatstring.h>

#include <formaction.h>
#include <feedlist_formaction.h>
#include <itemlist_formaction.h>
#include <itemview_formaction.h>
#include <help_formaction.h>
#include <urlview_formaction.h>
#include <select_formaction.h>
#include <dialogs_formaction.h>

#include <logger.h>
#include <reloadthread.h>
#include <exception.h>
#include <exceptions.h>
#include <keymap.h>
#include <utils.h>
#include <regexmanager.h>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cerrno>

#include <assert.h>
#include <libgen.h>
#include <sys/types.h>
#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <unistd.h>
#include <config.h>
#include <sys/param.h>
#include <string.h>
#include <ncurses.h>


extern "C" {
#include <stfl.h>
}

#include <view.h>
#include <rss.h>
#include <htmlrenderer.h>
#include <cstring>
#include <cstdio>

namespace newsbeuter {

view::view(controller * c) : ctrl(c), cfg(0), keys(0), mtx(0), current_formaction(0), is_inside_qna(false), is_inside_cmdline(false), tab_count(0) {
	mtx = new mutex();
}

view::~view() {
	stfl::reset();
	delete mtx;
}

void view::set_config_container(configcontainer * cfgcontainer) {
	cfg = cfgcontainer;	
}

void view::set_keymap(keymap * k) {
	keys = k;
}


void view::update_bindings() {
	for (std::vector<std::tr1::shared_ptr<formaction> >::iterator it=formaction_stack.begin();it!=formaction_stack.end();++it) {
		if (*it) {
			set_bindings(*it);
		}
	}
}

void view::set_bindings(std::tr1::shared_ptr<formaction> fa) {
	std::string upkey("** "); upkey.append(keys->getkey(OP_SK_UP, fa->id()));
	std::string downkey("** "); downkey.append(keys->getkey(OP_SK_DOWN, fa->id()));
	fa->get_form()->set("bind_up", upkey);
	fa->get_form()->set("bind_down", downkey);

	std::string pgupkey;
	std::string pgdownkey;
	if (fa->id() == "article" || fa->id() == "help") {
		pgupkey.append("** b ");
		pgdownkey.append("** SPACE ");
	} else {
		pgupkey.append("** "); 
		pgdownkey.append("** "); 
	}

	pgupkey.append(keys->getkey(OP_SK_PGUP, fa->id()));
	pgdownkey.append(keys->getkey(OP_SK_PGDOWN, fa->id()));

	fa->get_form()->set("bind_page_up", pgupkey);
	fa->get_form()->set("bind_page_down", pgdownkey);

	std::string homekey("** "); homekey.append(keys->getkey(OP_SK_HOME, fa->id()));
	std::string endkey("** "); endkey.append(keys->getkey(OP_SK_END, fa->id()));
	fa->get_form()->set("bind_home", homekey);
	fa->get_form()->set("bind_end", endkey);
}

std::tr1::shared_ptr<formaction> view::get_current_formaction() {
	if (formaction_stack.size() > 0 && current_formaction < formaction_stack.size()) {
		return formaction_stack[current_formaction];
	}
	return std::tr1::shared_ptr<formaction>();
}

void view::set_status_unlocked(const std::string& msg) {
	if (formaction_stack.size() > 0 && get_current_formaction() != NULL) {
		std::tr1::shared_ptr<stfl::form> form = get_current_formaction()->get_form();
		if (form != NULL) {
			form->set("msg",msg);
			form->run(-1);
		} else {
			LOG(LOG_ERROR, "view::set_status_unlocked: form for formaction of type %s is NULL!", get_current_formaction()->id().c_str());
		}
	}
}

void view::set_status(const std::string& msg) {
	scope_mutex lock(mtx);
	set_status_unlocked(msg);
}

void view::show_error(const std::string& msg) {
	set_status(msg);
}

void view::run() {
	bool have_macroprefix = false;
	std::vector<macrocmd> macrocmds;

	// create feedlist
	std::tr1::shared_ptr<feedlist_formaction> feedlist(new feedlist_formaction(this, feedlist_str));
	set_bindings(feedlist);
	feedlist->set_regexmanager(rxman);
	feedlist->set_tags(tags);
	apply_colors(feedlist);
	formaction_stack.push_back(feedlist);
	current_formaction = formaction_stack_size() - 1;

	get_current_formaction()->init();

	stfl::reset();

	/*
	 * This is the main "event" loop of newsbeuter.
	 */

	while (formaction_stack.size() > 0) {
		// first, we take the current formaction.
		std::tr1::shared_ptr<formaction> fa = get_current_formaction();

		// we signal "oh, you will receive an operation soon"
		fa->prepare();

		if (!macrocmds.empty()) {
			// if there is any macro command left to process, we do so

			fa->get_form()->run(-1);
			fa->process_op(macrocmds[0].op, true, &macrocmds[0].args);

			macrocmds.erase(macrocmds.begin()); // remove first macro command, since it has already been processed

		} else {

			// we then receive the event and ignore timeouts.
			const char * event = fa->get_form()->run(60000);

			if (ctrl_c_hit) {
				ctrl_c_hit = 0;
				cancel_input(fa);
				if (!get_cfg()->get_configvalue_as_bool("confirm-exit") || confirm(_("Do you really want to quit (y:Yes n:No)? "), _("yn")) == *_("y")) {
					stfl::reset();
					utils::remove_fs_lock(lock_file);
					::exit(EXIT_FAILURE);
				}
			}

			if (!event || strcmp(event,"TIMEOUT")==0) {
				if (fa->id() == "article")
					std::tr1::dynamic_pointer_cast<itemview_formaction, formaction>(fa)->update_percent();
				continue;
			}

			if (is_inside_qna) {
				LOG(LOG_DEBUG, "view::run: we're inside QNA input");
				if (is_inside_cmdline && strcmp(event, "TAB")==0) {
					handle_cmdline_completion(fa);
					continue;
				}
				if (strcmp(event, "^U")==0) {
					clear_line(fa);
					continue;
				} else if (strcmp(event, "^K")==0) {
					clear_eol(fa);
					continue;
				} else if (strcmp(event, "^G")==0) {
					cancel_input(fa);
					continue;
				} else if (strcmp(event, "^W")==0) {
					delete_word(fa);
					continue;
				}
			}

			LOG(LOG_DEBUG, "view::run: event = %s", event);

			// retrieve operation code through the keymap
			operation op;
			
			if (have_macroprefix) {
				have_macroprefix = false;
				LOG(LOG_DEBUG, "view::run: running macro `%s'", event);
				macrocmds = keys->get_macro(event);
				set_status("");
			} else {
				op = keys->get_operation(event, fa->id());

				LOG(LOG_DEBUG, "view::run: event = %s op = %u", event, op);

				if (OP_MACROPREFIX == op) {
					have_macroprefix = true;
					set_status("macro-");
				}

				// now we handle the operation to the formaction.
				fa->process_op(op);
			}
		}
	}

	stfl::reset();
}

std::string view::run_modal(std::tr1::shared_ptr<formaction> f, const std::string& value) {
	f->init();
	unsigned int stacksize = formaction_stack.size();

	formaction_stack.push_back(f);
	current_formaction = formaction_stack_size() - 1;

	while (formaction_stack.size() > stacksize) {
		std::tr1::shared_ptr<formaction> fa = get_current_formaction();

		fa->prepare();

		const char * event = fa->get_form()->run(1000);
		LOG(LOG_DEBUG, "view::run: event = %s", event);
		if (!event || strcmp(event,"TIMEOUT")==0) continue;

		operation op = keys->get_operation(event, fa->id());

		if (OP_REDRAW == op) {
			stfl::reset();
			continue;
		}

		fa->process_op(op);
	}

	if (value == "")
		return "";
	else
		return f->get_value(value);
}

std::string view::get_filename_suggestion(const std::string& s) {
	/*
	 * With this function, we generate normalized filenames for saving
	 * articles to files.
	 */
	std::string retval;
	for (unsigned int i=0;i<s.length();++i) {
		if (isalnum(s[i]))
			retval.append(1,s[i]);
		else if (s[i] == '/' || s[i] == ' ' || s[i] == '\r' || s[i] == '\n') 
			retval.append(1,'_');
	}
	if (retval.length() == 0)
		retval = "article.txt";
	else
		retval.append(".txt");
	LOG(LOG_DEBUG,"view::get_filename_suggestion: %s -> %s", s.c_str(), retval.c_str());
	return retval;	
}

void view::push_empty_formaction() {
	formaction_stack.push_back(std::tr1::shared_ptr<formaction>());
	current_formaction = formaction_stack_size() - 1;
}

void view::open_in_pager(const std::string& filename) {
	formaction_stack.push_back(std::tr1::shared_ptr<formaction>());
	current_formaction = formaction_stack_size() - 1;
	std::string cmdline;
	std::string pager = cfg->get_configvalue("pager");
	if (pager.find("%f") != std::string::npos) {
		fmtstr_formatter fmt;
		fmt.register_fmt('f', filename);
		cmdline = fmt.do_format(pager, 0);
	} else {
		const char * env_pager = NULL;
		if (pager != "")
			cmdline.append(pager);
		else if ((env_pager = getenv("PAGER")) != NULL)
			cmdline.append(env_pager);
		else
			cmdline.append("more");
		cmdline.append(" ");
		cmdline.append(filename);
	}
	stfl::reset();
	LOG(LOG_DEBUG, "view::open_in_browser: running `%s'", cmdline.c_str());
	::system(cmdline.c_str());
	pop_current_formaction();
}

void view::open_in_browser(const std::string& url) {
	formaction_stack.push_back(std::tr1::shared_ptr<formaction>());
	current_formaction = formaction_stack_size() - 1;
	std::string cmdline;
	std::string browser = cfg->get_configvalue("browser");
	if (browser.find("%u") != std::string::npos) {
		fmtstr_formatter fmt;
		std::string newurl;
		newurl = utils::replace_all(url, "'", "%27");
		newurl.insert(0, "'");
		newurl.append("'");
		fmt.register_fmt('u', newurl);
		cmdline = fmt.do_format(browser, 0);
	} else {
		if (browser != "")
			cmdline.append(browser);
		else
			cmdline.append("lynx");
		cmdline.append(" '");
		cmdline.append(utils::replace_all(url,"'", "%27"));
		cmdline.append("'");
	}
	stfl::reset();
	LOG(LOG_DEBUG, "view::open_in_browser: running `%s'", cmdline.c_str());
	::system(cmdline.c_str());
	pop_current_formaction();
}

void view::update_visible_feeds(std::vector<std::tr1::shared_ptr<rss_feed> > feeds) {
	try {
		if (formaction_stack_size() > 0) {
			scope_mutex lock(mtx);
			std::tr1::shared_ptr<feedlist_formaction> feedlist = std::tr1::dynamic_pointer_cast<feedlist_formaction, formaction>(formaction_stack[0]);
			feedlist->update_visible_feeds(feeds);
		}
	} catch (const matcherexception& e) {
		set_status(utils::strprintf(_("Error: applying the filter failed: %s"), e.what()));
		LOG(LOG_DEBUG, "view::update_visible_feeds: inside catch: %s", e.what());
	}
}

void view::set_feedlist(std::vector<std::tr1::shared_ptr<rss_feed> > feeds) {
	try {
		scope_mutex lock(mtx);

		for (std::vector<std::tr1::shared_ptr<rss_feed> >::iterator it=feeds.begin();it!=feeds.end();++it) {
			if ((*it)->rssurl().substr(0,6) != "query:") {
				(*it)->set_feedptrs(*it);
			}
		}

		if (formaction_stack_size() > 0) {
			std::tr1::shared_ptr<feedlist_formaction> feedlist = std::tr1::dynamic_pointer_cast<feedlist_formaction, formaction>(formaction_stack[0]);
			feedlist->set_feedlist(feeds);
		}
	} catch (const matcherexception& e) {
		set_status(utils::strprintf(_("Error: applying the filter failed: %s"), e.what()));
	}
}


void view::set_tags(const std::vector<std::string>& t) {
	tags = t;
}

void view::push_searchresult(std::tr1::shared_ptr<rss_feed> feed, const std::string& phrase) {
	assert(feed != NULL);
	LOG(LOG_DEBUG, "view::push_searchresult: pushing search result");

	if (feed->items().size() > 0) {
		std::tr1::shared_ptr<itemlist_formaction> searchresult(new itemlist_formaction(this, itemlist_str));
		set_bindings(searchresult);
		searchresult->set_regexmanager(rxman);
		searchresult->set_feed(feed);
		searchresult->set_show_searchresult(true);
		searchresult->set_searchphrase(phrase);
		apply_colors(searchresult);
		searchresult->set_parent_formaction(get_current_formaction());
		searchresult->init();
		formaction_stack.push_back(searchresult);
		current_formaction = formaction_stack_size() - 1;
	} else {
		show_error(_("Error: feed contains no items!"));
	}

}

void view::push_itemlist(std::tr1::shared_ptr<rss_feed> feed) {
	assert(feed != NULL);

	feed->purge_deleted_items();

	prepare_query_feed(feed);

	if (feed->items().size() > 0) {
		std::tr1::shared_ptr<itemlist_formaction> itemlist(new itemlist_formaction(this, itemlist_str));
		set_bindings(itemlist);
		itemlist->set_regexmanager(rxman);
		itemlist->set_feed(feed);
		itemlist->set_show_searchresult(false);
		apply_colors(itemlist);
		itemlist->set_parent_formaction(get_current_formaction());
		itemlist->init();
		formaction_stack.push_back(itemlist);
		current_formaction = formaction_stack_size() - 1;
	} else {
		show_error(_("Error: feed contains no items!"));
	}
}

void view::push_itemlist(unsigned int pos) {
	std::tr1::shared_ptr<rss_feed> feed = ctrl->get_feed(pos);
	LOG(LOG_DEBUG, "view::push_itemlist: retrieved feed at position %d", pos);
	push_itemlist(feed);
	if (feed->items().size() > 0) {
		std::tr1::shared_ptr<itemlist_formaction> itemlist = std::tr1::dynamic_pointer_cast<itemlist_formaction, formaction>(get_current_formaction());
		itemlist->set_pos(pos);
	}
}

void view::push_itemview(std::tr1::shared_ptr<rss_feed> f, const std::string& guid, const std::string& searchphrase) {
	std::string pager;
	if ((pager = cfg->get_configvalue("pager")) == "internal") {
		std::tr1::shared_ptr<itemlist_formaction> itemlist = std::tr1::dynamic_pointer_cast<itemlist_formaction, formaction>(get_current_formaction());
		assert(itemlist != NULL);
		std::tr1::shared_ptr<itemview_formaction> itemview(new itemview_formaction(this, itemlist, itemview_str));
		set_bindings(itemview);
		itemview->set_regexmanager(rxman);
		itemview->set_feed(f);
		itemview->set_guid(guid);
		itemview->set_parent_formaction(get_current_formaction());
		if (searchphrase.length() > 0)
			itemview->set_highlightphrase(searchphrase);
		apply_colors(itemview);
		itemview->init();
		formaction_stack.push_back(itemview);
		current_formaction = formaction_stack_size() - 1;
	} else {
		std::string filename = get_ctrl()->write_temporary_item(f->get_item_by_guid(guid));
		open_in_pager(filename);
		::unlink(filename.c_str());
	}
}

void view::view_dialogs() {
	if (get_current_formaction() != NULL && get_current_formaction()->id() != "dialogs") {
		std::tr1::shared_ptr<dialogs_formaction> dialogs(new dialogs_formaction(this, dialogs_str));
		dialogs->set_parent_formaction(get_current_formaction());
		apply_colors(dialogs);
		dialogs->init();
		formaction_stack.push_back(dialogs);
		current_formaction = formaction_stack_size() - 1;
	}
}

void view::push_help() {
	std::tr1::shared_ptr<help_formaction> helpview(new help_formaction(this, help_str));
	set_bindings(helpview);
	apply_colors(helpview);
	helpview->set_context(get_current_formaction()->id());
	helpview->set_parent_formaction(get_current_formaction());
	helpview->init();
	formaction_stack.push_back(helpview);
	current_formaction = formaction_stack_size() - 1;
}

void view::push_urlview(const std::vector<linkpair>& links) {
	std::tr1::shared_ptr<urlview_formaction> urlview(new urlview_formaction(this, urlview_str));
	set_bindings(urlview);
	apply_colors(urlview);
	urlview->set_parent_formaction(get_current_formaction());
	urlview->init();
	urlview->set_links(links);
	formaction_stack.push_back(urlview);
	current_formaction = formaction_stack_size() - 1;
}

std::string view::run_filebrowser(const std::string& default_filename, const std::string& dir) {
	std::tr1::shared_ptr<filebrowser_formaction> filebrowser(new filebrowser_formaction(this, filebrowser_str));
	set_bindings(filebrowser);
	apply_colors(filebrowser);
	filebrowser->set_dir(dir);
	filebrowser->set_default_filename(default_filename);
	filebrowser->set_parent_formaction(get_current_formaction());
	return run_modal(filebrowser, "filenametext");
}

std::string view::select_tag(const std::vector<std::string>& tags) {
	std::tr1::shared_ptr<select_formaction> selecttag(new select_formaction(this, selecttag_str));
	set_bindings(selecttag);
	apply_colors(selecttag);
	selecttag->set_parent_formaction(get_current_formaction());
	selecttag->set_type(select_formaction::SELECTTAG);
	selecttag->set_tags(tags);
	run_modal(selecttag, "");
	return selecttag->get_selected_value();
}

std::string view::select_filter(const std::vector<filter_name_expr_pair>& filters) {
	std::tr1::shared_ptr<select_formaction> selecttag(new select_formaction(this, selecttag_str));
	set_bindings(selecttag);
	apply_colors(selecttag);
	selecttag->set_parent_formaction(get_current_formaction());
	selecttag->set_type(select_formaction::SELECTFILTER);
	selecttag->set_filters(filters);
	run_modal(selecttag, "");
	return selecttag->get_selected_value();
}

char view::confirm(const std::string& prompt, const std::string& charset) {
	LOG(LOG_DEBUG, "view::confirm: charset = %s", charset.c_str());

	std::tr1::shared_ptr<formaction> f = get_current_formaction();
	formaction_stack.push_back(std::tr1::shared_ptr<formaction>());
	current_formaction = formaction_stack_size() - 1;
	f->get_form()->set("msg", prompt);

	char result = 0;

	do {
		const char * event = f->get_form()->run(0);
		LOG(LOG_DEBUG,"view::confirm: event = %s", event);
		if (!event) continue;
		if (strcmp(event, "ESC")==0 || strcmp(event, "ENTER")==0) {
			result = 0;
			LOG(LOG_DEBUG, "view::confirm: user pressed ESC or ENTER, we cancel confirmation dialog");
			break;
		}
		result = keys->get_key(event);
		LOG(LOG_DEBUG, "view::confirm: key = %c (%u)", result, result);
	} while (!result || strchr(charset.c_str(), result)==NULL);

	f->get_form()->set("msg", "");
	f->get_form()->run(-1);

	pop_current_formaction();

	return result;
}

void view::notify_itemlist_change(std::tr1::shared_ptr<rss_feed> feed) {
	for (std::vector<std::tr1::shared_ptr<formaction> >::iterator it=formaction_stack.begin();it!=formaction_stack.end();++it) {
		if (*it != NULL && (*it)->id() == "articlelist") {
			std::tr1::shared_ptr<itemlist_formaction> itemlist = std::tr1::dynamic_pointer_cast<itemlist_formaction, formaction>(*it);
			if (itemlist != NULL) {
				std::tr1::shared_ptr<rss_feed> f = itemlist->get_feed();
				if (f != NULL && f->rssurl() == feed->rssurl()) {
					itemlist->set_feed(feed);
					itemlist->set_redraw(true);
				}
			}
		}
	}
}

bool view::get_random_unread(itemlist_formaction * itemlist, itemview_formaction * itemview) {
	unsigned int feedpos;
	std::tr1::shared_ptr<feedlist_formaction> feedlist = std::tr1::dynamic_pointer_cast<feedlist_formaction, formaction>(formaction_stack[0]);
	if (!cfg->get_configvalue_as_bool("goto-next-feed")) {
		return false;
	} 
	if (feedlist->jump_to_random_unread_feed(feedpos)) {
		LOG(LOG_DEBUG, "view::get_previous_unread: found feed with unread articles");
		prepare_query_feed(feedlist->get_feed());
		itemlist->set_feed(feedlist->get_feed());
		itemlist->set_pos(feedpos);
		itemlist->init();
		if (itemlist->jump_to_random_unread_item()) {
			if (itemview) {
				itemview->init();
				itemview->set_feed(itemlist->get_feed());
				itemview->set_guid(itemlist->get_guid());
			}
			return true;
		}
	}
	return false;
}

bool view::get_previous_unread(itemlist_formaction * itemlist, itemview_formaction * itemview) {
	unsigned int feedpos;
	LOG(LOG_DEBUG, "view::get_previous_unread: trying to find previous unread");
	std::tr1::shared_ptr<feedlist_formaction> feedlist = std::tr1::dynamic_pointer_cast<feedlist_formaction, formaction>(formaction_stack[0]);
	if (itemlist->jump_to_previous_unread_item(false)) {
		LOG(LOG_DEBUG, "view::get_previous_unread: found unread article in same feed");
		if (itemview) {
			itemview->init();
			itemview->set_feed(itemlist->get_feed());
			itemview->set_guid(itemlist->get_guid());
		}
		return true;
	} else if (cfg->get_configvalue_as_bool("goto-next-feed")==false) {
		LOG(LOG_DEBUG, "view::get_previous_unread: goto-next-feed = false");
		show_error(_("No unread items."));
	} else if (feedlist->jump_to_previous_unread_feed(feedpos)) {
		LOG(LOG_DEBUG, "view::get_previous_unread: found feed with unread articles");
		prepare_query_feed(feedlist->get_feed());
		itemlist->set_feed(feedlist->get_feed());
		itemlist->set_pos(feedpos);
		itemlist->init();
		if (itemlist->jump_to_previous_unread_item(true)) {
			if (itemview) {
				itemview->init();
				itemview->set_feed(itemlist->get_feed());
				itemview->set_guid(itemlist->get_guid());
			}
			return true;
		}
	}
	return false;
}

bool view::get_next_unread_feed(itemlist_formaction * itemlist) {
	std::tr1::shared_ptr<feedlist_formaction> feedlist = std::tr1::dynamic_pointer_cast<feedlist_formaction, formaction>(formaction_stack[0]);
	unsigned int feedpos;
	assert(feedlist != NULL);
	if (feedlist->jump_to_next_unread_feed(feedpos)) {
		prepare_query_feed(feedlist->get_feed());
		itemlist->set_feed(feedlist->get_feed());
		itemlist->set_pos(feedpos);
		itemlist->init();
		return true;
	}
	return false;
}

bool view::get_prev_unread_feed(itemlist_formaction * itemlist) {
	std::tr1::shared_ptr<feedlist_formaction> feedlist = std::tr1::dynamic_pointer_cast<feedlist_formaction, formaction>(formaction_stack[0]);
	unsigned int feedpos;
	assert(feedlist != NULL);
	if (feedlist->jump_to_previous_unread_feed(feedpos)) {
		prepare_query_feed(feedlist->get_feed());
		itemlist->set_feed(feedlist->get_feed());
		itemlist->set_pos(feedpos);
		itemlist->init();
		return true;
	}
	return false;
}

bool view::get_next_unread(itemlist_formaction * itemlist, itemview_formaction * itemview) {
	unsigned int feedpos;
	std::tr1::shared_ptr<feedlist_formaction> feedlist = std::tr1::dynamic_pointer_cast<feedlist_formaction, formaction>(formaction_stack[0]);
	LOG(LOG_DEBUG, "view::get_next_unread: trying to find next unread");
	if (itemlist->jump_to_next_unread_item(false)) {
		LOG(LOG_DEBUG, "view::get_next_unread: found unread article in same feed");
		if (itemview) {
			itemview->init();
			itemview->set_feed(itemlist->get_feed());
			itemview->set_guid(itemlist->get_guid());
		}
		return true;
	} else if (cfg->get_configvalue_as_bool("goto-next-feed")==false) {
		LOG(LOG_DEBUG, "view::get_next_unread: goto-next-feed = false");
		show_error(_("No unread items."));
	} else if (feedlist->jump_to_next_unread_feed(feedpos)) {
		LOG(LOG_DEBUG, "view::get_next_unread: found feed with unread articles");
		prepare_query_feed(feedlist->get_feed());
		itemlist->set_feed(feedlist->get_feed());
		itemlist->set_pos(feedpos);
		itemlist->init();
		if (itemlist->jump_to_next_unread_item(true)) {
			if (itemview) {
				itemview->init();
				itemview->set_feed(itemlist->get_feed());
				itemview->set_guid(itemlist->get_guid());
			}
			return true;
		}
	}
	return false;
}

bool view::get_previous(itemlist_formaction * itemlist, itemview_formaction * itemview) {
	unsigned int feedpos;
	std::tr1::shared_ptr<feedlist_formaction> feedlist = std::tr1::dynamic_pointer_cast<feedlist_formaction, formaction>(formaction_stack[0]);
	if (itemlist->jump_to_previous_item(false)) {
		LOG(LOG_DEBUG, "view::get_previous: article in same feed");
		if (itemview) {
			itemview->init();
			itemview->set_feed(itemlist->get_feed());
			itemview->set_guid(itemlist->get_guid());
		}
		return true;
	} else if (cfg->get_configvalue_as_bool("goto-next-feed")==false) {
		LOG(LOG_DEBUG, "view::get_previous: goto-next-feed = false");
		show_error(_("Already on first item."));
	} else if (feedlist->jump_to_previous_feed(feedpos)) {
		LOG(LOG_DEBUG, "view::get_previous: previous feed");
		prepare_query_feed(feedlist->get_feed());
		itemlist->set_feed(feedlist->get_feed());
		itemlist->set_pos(feedpos);
		itemlist->init();
		if (itemlist->jump_to_previous_item(true)) {
			if (itemview) {
				itemview->init();
				itemview->set_feed(itemlist->get_feed());
				itemview->set_guid(itemlist->get_guid());
			}
			return true;
		}
	}
	return false;
}

bool view::get_next(itemlist_formaction * itemlist, itemview_formaction * itemview) {
	unsigned int feedpos;
	std::tr1::shared_ptr<feedlist_formaction> feedlist = std::tr1::dynamic_pointer_cast<feedlist_formaction, formaction>(formaction_stack[0]);
	if (itemlist->jump_to_next_item(false)) {
		LOG(LOG_DEBUG, "view::get_next: article in same feed");
		if (itemview) {
			itemview->init();
			itemview->set_feed(itemlist->get_feed());
			itemview->set_guid(itemlist->get_guid());
		}
		return true;
	} else if (cfg->get_configvalue_as_bool("goto-next-feed")==false) {
		LOG(LOG_DEBUG, "view::get_next: goto-next-feed = false");
		show_error(_("Already on last item."));
	} else if (feedlist->jump_to_next_feed(feedpos)) {
		LOG(LOG_DEBUG, "view::get_next: next feed");
		prepare_query_feed(feedlist->get_feed());
		itemlist->set_feed(feedlist->get_feed());
		itemlist->set_pos(feedpos);
		itemlist->init();
		if (itemlist->jump_to_next_item(true)) {
			if (itemview) {
				itemview->init();
				itemview->set_feed(itemlist->get_feed());
				itemview->set_guid(itemlist->get_guid());
			}
			return true;
		}
	}
	return false;
}

bool view::get_next_feed(itemlist_formaction * itemlist) {
	std::tr1::shared_ptr<feedlist_formaction> feedlist = std::tr1::dynamic_pointer_cast<feedlist_formaction, formaction>(formaction_stack[0]);
	unsigned int feedpos;
	assert(feedlist != NULL);
	if (feedlist->jump_to_next_feed(feedpos)) {
		prepare_query_feed(feedlist->get_feed());
		itemlist->set_feed(feedlist->get_feed());
		itemlist->set_pos(feedpos);
		itemlist->init();
		return true;
	}
	return false;
}

bool view::get_prev_feed(itemlist_formaction * itemlist) {
	std::tr1::shared_ptr<feedlist_formaction> feedlist = std::tr1::dynamic_pointer_cast<feedlist_formaction, formaction>(formaction_stack[0]);
	unsigned int feedpos;
	assert(feedlist != NULL);
	if (feedlist->jump_to_previous_feed(feedpos)) {
		prepare_query_feed(feedlist->get_feed());
		itemlist->set_feed(feedlist->get_feed());
		itemlist->set_pos(feedpos);
		itemlist->init();
		return true;
	}
	return false;
}

void view::prepare_query_feed(std::tr1::shared_ptr<rss_feed> feed) {
	if (feed->rssurl().substr(0,6) == "query:") {
		LOG(LOG_DEBUG, "view::prepare_query_feed: %s", feed->rssurl().c_str());

		set_status(_("Updating query feed..."));
		feed->update_items(ctrl->get_all_feeds());
		feed->sort(cfg->get_configvalue("article-sort-order"));
		notify_itemlist_change(feed);
		set_status("");
	}
}

void view::force_redraw() {
	std::tr1::shared_ptr<formaction> fa = get_current_formaction();
	fa->set_redraw(true);
	fa->prepare();
	fa->get_form()->run(-1);
}

void view::pop_current_formaction() {
	std::tr1::shared_ptr<formaction> f = get_current_formaction();
	std::vector<std::tr1::shared_ptr<formaction> >::iterator it=formaction_stack.begin();
	for (unsigned int i=0;i<current_formaction;i++)
		++it;
	formaction_stack.erase(it);
	if (f == NULL) {
		current_formaction = formaction_stack_size() - 1; // XXX TODO this is not correct... we'd need to return to the previous one, but NULL formactions have no parent
	} else if (formaction_stack.size() > 0) {
		// first, we set back the parent formactions of those who reference the formaction we just removed
		for (std::vector<std::tr1::shared_ptr<formaction> >::iterator it=formaction_stack.begin();it!=formaction_stack.end();++it) {
			if ((*it)->get_parent_formaction() == f) {
				(*it)->set_parent_formaction(formaction_stack[0]);
			}
		}
		// we set the new formaction based on the removed formaction's parent.
		unsigned int i=0;
		for (std::vector<std::tr1::shared_ptr<formaction> >::iterator it=formaction_stack.begin();it!=formaction_stack.end();++it,i++) {
			if (*it == f->get_parent_formaction()) {
				current_formaction = i;
				break;
			}
		}
		std::tr1::shared_ptr<formaction> f = get_current_formaction();
		if (f) {
			f->set_redraw(true);
			f->get_form()->set("msg","");
			f->recalculate_form();
		}
	}
}

void view::set_current_formaction(unsigned int pos) {
	remove_formaction(current_formaction);
	current_formaction = pos;
}

void view::remove_formaction(unsigned int pos) {
	std::tr1::shared_ptr<formaction> f = formaction_stack[pos];
	std::vector<std::tr1::shared_ptr<formaction> >::iterator it=formaction_stack.begin();
	for (unsigned int i=0;i<pos;i++)
		++it;
	formaction_stack.erase(it);
	current_formaction--;
	if (f != NULL && formaction_stack.size() > 0) {
		// we set back the parent formactions of those who reference the formaction we just removed
		for (std::vector<std::tr1::shared_ptr<formaction> >::iterator it=formaction_stack.begin();it!=formaction_stack.end();++it) {
			if ((*it)->get_parent_formaction() == f) {
				(*it)->set_parent_formaction(formaction_stack[0]);
			}
		}
	}
}

void view::set_colors(std::map<std::string,std::string>& fgc, std::map<std::string,std::string>& bgc, std::map<std::string,std::vector<std::string> >& attribs) {
	fg_colors = fgc;
	bg_colors = bgc;
	attributes = attribs;
}

void view::apply_colors_to_all_formactions() {
	for (std::vector<std::tr1::shared_ptr<formaction> >::iterator it=formaction_stack.begin();it!=formaction_stack.end();++it) {
		apply_colors(*it);
	}
	if (formaction_stack.size() > 0 && formaction_stack[current_formaction]) {
		formaction_stack[current_formaction]->set_redraw(true);
	}
}

void view::apply_colors(std::tr1::shared_ptr<formaction> fa) {
	std::map<std::string,std::string>::const_iterator fgcit = fg_colors.begin();
	std::map<std::string,std::string>::const_iterator bgcit = bg_colors.begin();
	std::map<std::string,std::vector<std::string> >::const_iterator attit = attributes.begin();
	LOG(LOG_DEBUG, "view::apply_colors: fa = %s", fa->id().c_str());

	std::string article_colorstr;

	for (;fgcit != fg_colors.end(); ++fgcit, ++bgcit, ++attit) {
		std::string colorattr;
		if (fgcit->second != "default") {
			colorattr.append("fg=");
			colorattr.append(fgcit->second);
		}
		if (bgcit->second != "default") {
			if (colorattr.length() > 0)
				colorattr.append(",");
			colorattr.append("bg=");
			colorattr.append(bgcit->second);
		}
		for (std::vector<std::string>::const_iterator it=attit->second.begin(); it!= attit->second.end(); ++it) {
			if (colorattr.length() > 0)
				colorattr.append(",");
			colorattr.append("attr=");
			colorattr.append(*it);
		} 

		if (fgcit->first == "article") {
			article_colorstr = colorattr;
			if (fa->id() == "article") {
				std::string bold = article_colorstr;
				std::string ul = article_colorstr;
				if (bold.length() > 0)
					bold.append(",");
				if (ul.length() > 0)
					ul.append(",");
				bold.append("attr=bold");
				ul.append("attr=underline");
				fa->get_form()->set("color_bold", bold.c_str());
				fa->get_form()->set("color_underline", ul.c_str());
			}
		}

		LOG(LOG_DEBUG,"view::apply_colors: %s %s %s\n", fa->id().c_str(), fgcit->first.c_str(), colorattr.c_str());

		fa->get_form()->set(fgcit->first, colorattr);

		if (fgcit->first == "article") {
			if (fa->id() == "article" || fa->id() == "help") {
				std::string styleend_str;
				if (bgcit->second != "default") {
					styleend_str.append("bg=");
					styleend_str.append(bgcit->second);
				}
				if (styleend_str.length() > 0)
					styleend_str.append(",");
				styleend_str.append("attr=bold");

				fa->get_form()->set("styleend", styleend_str.c_str());
			}
		}
	}
}

void view::feedlist_mark_pos_if_visible(unsigned int pos) {
	if (formaction_stack_size() > 0) {
		std::tr1::dynamic_pointer_cast<feedlist_formaction, formaction>(formaction_stack[0])->mark_pos_if_visible(pos);
	}
}

void view::set_regexmanager(regexmanager * r) {
	rxman = r;
}


std::vector<std::pair<unsigned int, std::string> > view::get_formaction_names() {
	std::vector<std::pair<unsigned int, std::string> > formaction_names;
	unsigned int i=0;
	for (std::vector<std::tr1::shared_ptr<formaction> >::iterator it=formaction_stack.begin();it!=formaction_stack.end();++it,i++) {
		if (*it && (*it)->id() != "dialogs") {
			formaction_names.push_back(std::pair<unsigned int, std::string>(i, (*it)->title()));
		}
	}
	return formaction_names;
}

void view::goto_next_dialog() {
	current_formaction++;
	if (current_formaction >= formaction_stack.size())
		current_formaction = 0;
}

void view::goto_prev_dialog() {
	if (current_formaction > 0) {
		current_formaction--;
	} else {
		current_formaction = formaction_stack.size() - 1;
	}
}

void view::inside_qna(bool f) {
	is_inside_qna = f;
}

void view::inside_cmdline(bool f) {
	is_inside_cmdline = f;
}

void view::clear_line(std::tr1::shared_ptr<formaction> fa) {
	fa->get_form()->set("qna_value", "");
	fa->get_form()->set("qna_value_pos", "0");
	LOG(LOG_DEBUG, "view::clear_line: cleared line");
}

void view::clear_eol(std::tr1::shared_ptr<formaction> fa) {
	unsigned int pos = utils::to_u(fa->get_form()->get("qna_value_pos"));
	std::string val = fa->get_form()->get("qna_value");
	val.erase(pos, val.length());
	fa->get_form()->set("qna_value", val);
	fa->get_form()->set("qna_value_pos", utils::to_s(val.length()));
	LOG(LOG_DEBUG, "view::clear_eol: cleared to end of line");
}

void view::cancel_input(std::tr1::shared_ptr<formaction> fa) {
	fa->process_op(OP_INT_CANCEL_QNA);
	LOG(LOG_DEBUG, "view::cancel_input: cancelled input");
}

void view::delete_word(std::tr1::shared_ptr<formaction> fa) {
	std::string::size_type curpos = utils::to_u(fa->get_form()->get("qna_value_pos"));
	std::string val = fa->get_form()->get("qna_value");
	std::string::size_type firstpos = curpos;
	LOG(LOG_DEBUG, "view::delete_word: before val = %s", val.c_str());
	if (firstpos >= val.length() || ::isspace(val[firstpos])) {
		if (firstpos != 0 && firstpos >= val.length())
			firstpos = val.length() - 1;
		while (firstpos > 0 && ::isspace(val[firstpos])) {
			--firstpos;
		}
	}
	while (firstpos > 0 && !::isspace(val[firstpos])) {
		--firstpos;
	}
	if (firstpos != 0)
		firstpos++;
	val.erase(firstpos, curpos - firstpos);
	LOG(LOG_DEBUG, "view::delete_word: after val = %s", val.c_str());
	fa->get_form()->set("qna_value", val);
	fa->get_form()->set("qna_value_pos", utils::to_s(firstpos));
}

void view::handle_cmdline_completion(std::tr1::shared_ptr<formaction> fa) {
	std::string fragment = fa->get_form()->get("qna_value");
	if (fragment != last_fragment || fragment == "") {
		last_fragment = fragment;
		suggestions = fa->get_suggestions(fragment);
		tab_count = 0;
	}
	tab_count++;
	std::string suggestion;
	switch (suggestions.size()) {
		case 0:
			LOG(LOG_DEBUG, "view::handle_cmdline_completion: found no suggestion for `%s'", fragment.c_str());
			::beep(); // direct call to ncurses - we beep to signal that there is no suggestion available, just like vim
			return;
		case 1:
			suggestion = suggestions[0];
			break;
		default:
			suggestion = suggestions[(tab_count-1) % suggestions.size()];
			break;
	}
	fa->get_form()->set("qna_value", suggestion);
	fa->get_form()->set("qna_value_pos", utils::to_s(suggestion.length()));
	last_fragment = suggestion;
}

void view::dump_current_form() {
	std::string formtext = formaction_stack[current_formaction]->get_form()->dump("", "", 0);
	char fnbuf[128];
	time_t t = ::time(NULL);
	struct tm * stm = localtime(&t);
	strftime(fnbuf, sizeof(fnbuf), "dumpform-%Y%m%d-%H%M%S.stfl", stm);
	std::fstream f(fnbuf, std::ios_base::out);
	if (!f.is_open()) {
		show_error(utils::strprintf("Error: couldn't open file %s: %s", fnbuf, strerror(errno)));
		return;
	}
	f << formtext;
	f.close();
	set_status(utils::strprintf("Dumped current form to file %s", fnbuf));
}

}

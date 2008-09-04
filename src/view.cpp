#include <feedlist.h>
#include <itemlist.h>
#include <itemview.h>
#include <help.h>
#include <filebrowser.h>
#include <urlview.h>
#include <selecttag.h>
#include <formatstring.h>

#include <formaction.h>
#include <feedlist_formaction.h>
#include <itemlist_formaction.h>
#include <itemview_formaction.h>
#include <help_formaction.h>
#include <urlview_formaction.h>
#include <select_formaction.h>

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


extern "C" {
#include <stfl.h>
}

#include <view.h>
#include <rss.h>
#include <htmlrenderer.h>
#include <cstring>
#include <cstdio>

namespace newsbeuter {

view::view(controller * c) : ctrl(c), cfg(0), keys(0), mtx(0) {
	mtx = new mutex();

	feedlist = new feedlist_formaction(this, feedlist_str);
	itemlist = new itemlist_formaction(this, itemlist_str);
	itemview = new itemview_formaction(this, itemview_str);
	helpview = new help_formaction(this, help_str);
	filebrowser = new filebrowser_formaction(this, filebrowser_str);
	urlview = new urlview_formaction(this, urlview_str);
	selecttag = new select_formaction(this, selecttag_str);
	searchresult = new itemlist_formaction(this, itemlist_str);

	// push the dialog to start with onto the stack
	formaction_stack.push_front(feedlist);
}

view::~view() {
	stfl::reset();
	delete mtx;
	delete feedlist;
	delete itemlist;
	delete itemview;
	delete helpview;
	delete filebrowser;
	delete urlview;
	delete selecttag;
	delete searchresult;
}

void view::set_config_container(configcontainer * cfgcontainer) {
	cfg = cfgcontainer;	
	if (cfg) {
		if (cfg->get_configvalue_as_bool("show-keymap-hint") == false) {
			feedlist->get_form()->set("showhint", "0");
			itemlist->get_form()->set("showhint", "0");
			itemview->get_form()->set("showhint", "0");
			helpview->get_form()->set("showhint", "0");
			filebrowser->get_form()->set("showhint", "0");
			urlview->get_form()->set("showhint", "0");
			selecttag->get_form()->set("showhint", "0");
			searchresult->get_form()->set("showhint", "0");
		}
	}
}

void view::set_keymap(keymap * k) {
	keys = k;
	set_bindings();
}



void view::set_bindings() {
	formaction * fas2bind[] = { feedlist, itemlist, itemview, helpview, filebrowser, urlview, selecttag, searchresult, NULL };
	if (keys) {
		std::string upkey("** "); upkey.append(keys->getkey(OP_SK_UP));
		std::string downkey("** "); downkey.append(keys->getkey(OP_SK_DOWN));
		std::string pgupkey("** "); pgupkey.append(keys->getkey(OP_SK_PGUP));
		std::string pgdownkey("** "); pgdownkey.append(keys->getkey(OP_SK_PGDOWN));

		std::string pgupkey_itemview("** b "); pgupkey_itemview.append(keys->getkey(OP_SK_PGUP));
		std::string pgdownkey_itemview("** SPACE "); pgdownkey_itemview.append(keys->getkey(OP_SK_PGDOWN));

		for (unsigned int i=0;fas2bind[i];++i) {
			fas2bind[i]->get_form()->set("bind_up", upkey);
			fas2bind[i]->get_form()->set("bind_down", downkey);
			if (fas2bind[i] == itemview || fas2bind[i] == helpview) { // the forms that contain textviews
				fas2bind[i]->get_form()->set("bind_page_up", pgupkey_itemview);
				fas2bind[i]->get_form()->set("bind_page_down", pgdownkey_itemview);
			} else {
				fas2bind[i]->get_form()->set("bind_page_up", pgupkey);
				fas2bind[i]->get_form()->set("bind_page_down", pgdownkey);
			}
		}
	}

	GetLogger().log(LOG_DEBUG, "view::set_bindings: itemview bind_page_up = %s bind_page_down = %s", itemview->get_form()->get("bind_page_up").c_str(), itemview->get_form()->get("bind_page_down").c_str());
}

void view::set_status_unlocked(const std::string& msg) {
	if (formaction_stack.size() > 0 && (*formaction_stack.begin()) != NULL) {
		stfl::form * form = (*formaction_stack.begin())->get_form();
		form->set("msg",msg);
		form->run(-1);
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
	/*
	 * This is the main "event" loop of newsbeuter.
	 */

	feedlist->init();

	while (formaction_stack.size() > 0) {
		// first, we take the current formaction.
		formaction * fa = *(formaction_stack.begin());

		// we signal "oh, you will receive an operation soon"
		fa->prepare();

		if (macrocmds.size() > 0) {
			// if there is any macro command left to process, we do so

			fa->get_form()->run(-1);
			fa->process_op(macrocmds[0].op, true, &macrocmds[0].args);

			macrocmds.erase(macrocmds.begin()); // remove first macro command, since it has already been processed

		} else {

			// we then receive the event and ignore timeouts.
			const char * event = fa->get_form()->run(0);
			if (!event || strcmp(event,"TIMEOUT")==0) {
				if (fa->id() == "articleview")
					itemview->update_percent();
				continue;
			}

			GetLogger().log(LOG_DEBUG, "view::run: event = %s", event);

			// retrieve operation code through the keymap
			operation op;
			
			if (have_macroprefix) {
				have_macroprefix = false;
				GetLogger().log(LOG_DEBUG, "view::run: running macro `%s'", event);
				macrocmds = keys->get_macro(event);
				set_status("");
			} else {
				op = keys->get_operation(event);

				GetLogger().log(LOG_DEBUG, "view::run: event = %s op = %u", event, op);

				// the redraw keybinding is handled globally so
				// that it doesn't need to be handled by all
				// formactions. We simply reset the screen, the
				// next time stfl_run() is called, it will be
				// reinitialized, anyway, and thus we can secure
				// that everything is redrawn.
				if (OP_REDRAW == op) {
					stfl::reset();
					continue;
				}

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

std::string view::run_modal(formaction * f, const std::string& value) {
	f->init();
	unsigned int stacksize = formaction_stack.size();

	formaction_stack.push_front(f);

	while (formaction_stack.size() > stacksize) {
		formaction * fa = *(formaction_stack.begin());

		fa->prepare();

		const char * event = fa->get_form()->run(1000);
		GetLogger().log(LOG_DEBUG, "view::run: event = %s", event);
		if (!event || strcmp(event,"TIMEOUT")==0) continue;

		operation op = keys->get_operation(event);

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
	GetLogger().log(LOG_DEBUG,"view::get_filename_suggestion: %s -> %s", s.c_str(), retval.c_str());
	return retval;	
}


void view::open_in_browser(const std::string& url) {
	formaction_stack.push_front(NULL); // we don't want a thread to write over the browser
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
	GetLogger().log(LOG_DEBUG, "view::open_in_browser: running `%s'", cmdline.c_str());
	::system(cmdline.c_str());
	formaction_stack.pop_front();
}

void view::update_visible_feeds(std::vector<std::tr1::shared_ptr<rss_feed> >& feeds) {
	scope_mutex lock(mtx);
	try {
		feedlist->update_visible_feeds(feeds);
	} catch (matcherexception e) {
		set_status_unlocked(utils::strprintf(_("Error: applying the filter failed: %s"), e.what()));
		GetLogger().log(LOG_DEBUG, "view::update_visible_feeds: inside catch: %s", e.what());
	}
}

void view::set_feedlist(std::vector<std::tr1::shared_ptr<rss_feed> >& feeds) {
	scope_mutex lock(mtx);

	for (std::vector<std::tr1::shared_ptr<rss_feed> >::iterator it=feeds.begin();it!=feeds.end();++it) {
		if ((*it)->rssurl().substr(0,6) != "query:") {
			ctrl->set_feedptrs(*it);
		}
	}

	try {
		feedlist->set_feedlist(feeds);
	} catch (matcherexception e) {
		set_status_unlocked(utils::strprintf(_("Error: applying the filter failed: %s"), e.what()));
		GetLogger().log(LOG_DEBUG, "view::set_feedlist: inside catch: %s", e.what());
	}
}


void view::set_tags(const std::vector<std::string>& t) {
	feedlist->set_tags(t);
}

void view::push_searchresult(std::tr1::shared_ptr<rss_feed> feed) {
	assert(feed != NULL);

	if (feed->items().size() > 0) {
		searchresult->set_feed(feed);
		searchresult->set_show_searchresult(true);
		searchresult->init();
		formaction_stack.push_front(searchresult);
	} else {
		show_error(_("Error: feed contains no items!"));
	}

}

void view::push_itemlist(std::tr1::shared_ptr<rss_feed> feed) {
	assert(feed != NULL);

	if (feed->rssurl().substr(0,6) == "query:") {
		set_status(_("Updating query feed..."));
		feed->update_items(ctrl->get_all_feeds());
		set_status("");
	}

	if (feed->items().size() > 0) {
		itemlist->set_feed(feed);
		itemlist->set_show_searchresult(false);
		itemlist->init();
		formaction_stack.push_front(itemlist);
	} else {
		show_error(_("Error: feed contains no items!"));
	}
}

void view::push_itemlist(unsigned int pos) {
	std::tr1::shared_ptr<rss_feed> feed = ctrl->get_feed(pos);
	GetLogger().log(LOG_DEBUG, "view::push_itemlist: retrieved feed at position %d", pos);
	itemlist->set_pos(pos);
	push_itemlist(feed);
}

void view::push_itemview(std::tr1::shared_ptr<rss_feed> f, const std::string& guid) {
	itemview->set_feed(f);
	itemview->set_guid(guid);
	itemview->init();
	formaction_stack.push_front(itemview);
}

void view::push_help() {
	formaction_stack.push_front(helpview);
	helpview->init();
}

void view::push_urlview(const std::vector<linkpair>& links) {
	urlview->init();
	urlview->set_links(links);
	formaction_stack.push_front(urlview);
}

std::string view::run_filebrowser(filebrowser_type type, const std::string& default_filename, const std::string& dir) {
	filebrowser->set_dir(dir);
	filebrowser->set_default_filename(default_filename);
	filebrowser->set_type(type);
	return run_modal(filebrowser, "filenametext");
}

std::string view::select_tag(const std::vector<std::string>& tags) {
	selecttag->set_type(select_formaction::SELECTTAG);
	selecttag->set_tags(tags);
	run_modal(selecttag, "");
	return selecttag->get_selected_value();
}

std::string view::select_filter(const std::vector<filter_name_expr_pair>& filters) {
	selecttag->set_type(select_formaction::SELECTFILTER);
	selecttag->set_filters(filters);
	run_modal(selecttag, "");
	return selecttag->get_selected_value();
}

char view::confirm(const std::string& prompt, const std::string& charset) {
	GetLogger().log(LOG_DEBUG, "view::confirm: charset = %s", charset.c_str());

	formaction * f = *formaction_stack.begin();
	formaction_stack.push_front(NULL);
	f->get_form()->set("msg", prompt);

	char result = 0;

	do {
		const char * event = f->get_form()->run(0);
		GetLogger().log(LOG_DEBUG,"view::confirm: event = %s", event);
		if (!event) continue;
		result = keys->get_key(event);
		GetLogger().log(LOG_DEBUG, "view::confirm: key = %c (%u)", result, result);
	} while (!result || strchr(charset.c_str(), result)==NULL);

	f->get_form()->set("msg", "");
	f->get_form()->run(-1);

	formaction_stack.pop_front(); // remove the NULL pointer from the formaction stack

	return result;
}

void view::notify_itemlist_change(std::tr1::shared_ptr<rss_feed>& feed) {
	std::tr1::shared_ptr<rss_feed> f = itemlist->get_feed();
	if (f != NULL && f->rssurl() == feed->rssurl()) {
		itemlist->do_update_visible_items();
		itemlist->set_redraw(true);
	}
}

bool view::get_previous_unread() {
	unsigned int feedpos;
	GetLogger().log(LOG_DEBUG, "view::get_previous_unread: trying to find previous unread");
	if (itemlist->jump_to_previous_unread_item(false)) {
		GetLogger().log(LOG_DEBUG, "view::get_previous_unread: found unread article in same feed");
		itemview->init();
		itemview->set_feed(itemlist->get_feed());
		itemview->set_guid(itemlist->get_guid());
		return true;
	} else if (cfg->get_configvalue_as_bool("goto-next-feed")==false) {
		GetLogger().log(LOG_DEBUG, "view::get_previous_unread: goto-next-feed = false");
		show_error(_("No unread items."));
	} else if (feedlist->jump_to_previous_unread_feed(feedpos)) {
		GetLogger().log(LOG_DEBUG, "view::get_previous_unread: found feed with unread articles");
		itemlist->set_feed(feedlist->get_feed());
		itemlist->set_pos(feedpos);
		itemlist->init();
		if (itemlist->jump_to_previous_unread_item(true)) {
			itemview->init();
			itemview->set_feed(itemlist->get_feed());
			itemview->set_guid(itemlist->get_guid());
			return true;
		}
	}
	return false;
}

bool view::get_next_unread_feed() {
	unsigned int feedpos;
	if (feedlist->jump_to_next_unread_feed(feedpos)) {
		itemlist->set_feed(feedlist->get_feed());
		itemlist->set_pos(feedpos);
		itemlist->init();
		return true;
	}
	return false;
}

bool view::get_prev_unread_feed() {
	unsigned int feedpos;
	if (feedlist->jump_to_previous_unread_feed(feedpos)) {
		itemlist->set_feed(feedlist->get_feed());
		itemlist->set_pos(feedpos);
		itemlist->init();
		return true;
	}
	return false;
}

bool view::get_next_unread() {
	unsigned int feedpos;
	GetLogger().log(LOG_DEBUG, "view::get_next_unread: trying to find next unread");
	if (itemlist->jump_to_next_unread_item(false)) {
		GetLogger().log(LOG_DEBUG, "view::get_next_unread: found unread article in same feed");
		itemview->init();
		itemview->set_feed(itemlist->get_feed());
		itemview->set_guid(itemlist->get_guid());
		return true;
	} else if (cfg->get_configvalue_as_bool("goto-next-feed")==false) {
		GetLogger().log(LOG_DEBUG, "view::get_next_unread: goto-next-feed = false");
		show_error(_("No unread items."));
	} else if (feedlist->jump_to_next_unread_feed(feedpos)) {
		GetLogger().log(LOG_DEBUG, "view::get_next_unread: found feed with unread articles");
		itemlist->set_feed(feedlist->get_feed());
		itemlist->set_pos(feedpos);
		itemlist->init();
		if (itemlist->jump_to_next_unread_item(true)) {
			itemview->init();
			itemview->set_feed(itemlist->get_feed());
			itemview->set_guid(itemlist->get_guid());
			return true;
		}
	}
	return false;
}

void view::pop_current_formaction() {
	formaction_stack.pop_front();
	if (formaction_stack.size() > 0) {
		formaction * f = (*formaction_stack.begin());
		if (f) {
			f->set_redraw(true);
			f->get_form()->set("msg","");
			f->recalculate_form();
		}
	}
}


void view::set_colors(std::map<std::string,std::string>& fg_colors, std::map<std::string,std::string>& bg_colors, std::map<std::string,std::vector<std::string> >& attributes) {
	std::map<std::string,std::string>::const_iterator fgcit = fg_colors.begin();
	std::map<std::string,std::string>::const_iterator bgcit = bg_colors.begin();
	std::map<std::string,std::vector<std::string> >::const_iterator attit = attributes.begin();

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

		GetLogger().log(LOG_DEBUG,"view::set_colors: %s %s\n",fgcit->first.c_str(), colorattr.c_str());

		feedlist->get_form()->set(fgcit->first, colorattr);
		itemlist->get_form()->set(fgcit->first, colorattr);
		itemview->get_form()->set(fgcit->first, colorattr);
		helpview->get_form()->set(fgcit->first, colorattr);
		filebrowser->get_form()->set(fgcit->first, colorattr);
		urlview->get_form()->set(fgcit->first, colorattr);
		selecttag->get_form()->set(fgcit->first, colorattr);
		searchresult->get_form()->set(fgcit->first, colorattr);

		if (fgcit->first == "article") {
			std::string styleend_str;
			
			if (bgcit->second != "default") {
				styleend_str.append("bg=");
				styleend_str.append(bgcit->second);
			}
			if (styleend_str.length() > 0)
				styleend_str.append(",");
			styleend_str.append("attr=bold");

			helpview->get_form()->set("styleend", styleend_str.c_str());
			itemview->get_form()->set("styleend", styleend_str.c_str());
		}
	}
}

std::string view::id() const {
	if (formaction_stack.size() > 0) {
		return (*formaction_stack.begin())->id();
	}
	return "";
}

std::string view::ask_user(const std::string& prompt) {
	std::vector<qna_pair> qna;
	qna.push_back(qna_pair(prompt, ""));
	(*formaction_stack.begin())->start_qna(qna, OP_NIL);
	return (*formaction_stack.begin())->get_qna_response(0);
}

void view::feedlist_mark_pos_if_visible(unsigned int pos) {
	feedlist->mark_pos_if_visible(pos);
}

void view::set_regexmanager(regexmanager * r) {
	itemview->set_regexmanager(r);
	itemlist->set_regexmanager(r);
	feedlist->set_regexmanager(r);
}


}

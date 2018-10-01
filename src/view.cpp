#include "view.h"

#include <assert.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <curses.h>
#include <dirent.h>
#include <fstream>
#include <grp.h>
#include <iomanip>
#include <iostream>
#include <libgen.h>
#include <ncurses.h>
#include <pwd.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

extern "C" {
#include <stfl.h>
}

#include "config.h"
#include "dialogs.h"
#include "dialogsformaction.h"
#include "exception.h"
#include "exceptions.h"
#include "feedlist.h"
#include "feedlistformaction.h"
#include "filebrowser.h"
#include "formaction.h"
#include "formatstring.h"
#include "help.h"
#include "helpformaction.h"
#include "htmlrenderer.h"
#include "itemlist.h"
#include "itemlistformaction.h"
#include "itemview.h"
#include "itemviewformaction.h"
#include "keymap.h"
#include "logger.h"
#include "regexmanager.h"
#include "reloadthread.h"
#include "rss.h"
#include "selectformaction.h"
#include "selecttag.h"
#include "strprintf.h"
#include "urlview.h"
#include "urlviewformaction.h"
#include "utils.h"

namespace {
bool ctrl_c_hit = false;
}

namespace newsboat {

View::View(Controller* c)
	: ctrl(c)
	, cfg(0)
	, keys(0)
	, current_formaction(0)
	, rxman(nullptr)
	, is_inside_qna(false)
	, is_inside_cmdline(false)
	, tab_count(0)
{
	if (getenv("ESCDELAY") == nullptr) {
		set_escdelay(25);
	}
}

View::~View()
{
	Stfl::reset();
}

void View::set_config_container(ConfigContainer* cfgcontainer)
{
	cfg = cfgcontainer;
}

void View::set_keymap(Keymap* k)
{
	keys = k;
}

void View::update_bindings()
{
	for (const auto& Form : Formaction_stack) {
		if (Form) {
			set_bindings(Form);
		}
	}
}

void View::set_bindings(std::shared_ptr<Formaction> fa)
{
	std::string upkey("** ");
	upkey.append(keys->getkey(OP_SK_UP, fa->id()));
	std::string downkey("** ");
	downkey.append(keys->getkey(OP_SK_DOWN, fa->id()));
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

	std::string homekey("** ");
	homekey.append(keys->getkey(OP_SK_HOME, fa->id()));
	std::string endkey("** ");
	endkey.append(keys->getkey(OP_SK_END, fa->id()));
	fa->get_form()->set("bind_home", homekey);
	fa->get_form()->set("bind_end", endkey);
}

std::shared_ptr<Formaction> View::get_current_formaction()
{
	if (Formaction_stack.size() > 0 &&
		current_formaction < Formaction_stack_size()) {
		return Formaction_stack[current_formaction];
	} else {
		return {};
	}
}

void View::set_status_unlocked(const std::string& msg)
{
	auto fa = get_current_formaction();
	if (fa) {
		std::shared_ptr<Stfl::Form> Form = fa->get_form();
		if (Form) {
			Form->set("msg", msg);
			Form->run(-1);
		} else {
			LOG(Level::ERROR,
				"View::set_status_unlocked: "
				"Form for Formaction of type %s is nullptr!",
				fa->id());
		}
	}
}

void View::set_status(const std::string& msg)
{
	std::lock_guard<std::mutex> lock(mtx);
	set_status_unlocked(msg);
}

void View::show_error(const std::string& msg)
{
	set_status(msg);
}

int View::run()
{
	bool have_macroprefix = false;
	std::vector<macrocmd> macrocmds;

	// create feedlist
	auto feedlist =
		std::make_shared<FeedListFormaction>(this, feedlist_str);
	set_bindings(feedlist);
	feedlist->set_RegexManager(rxman);
	feedlist->set_tags(tags);
	apply_colors(feedlist);
	Formaction_stack.push_back(feedlist);
	current_formaction = Formaction_stack_size() - 1;

	get_current_formaction()->init();

	Stfl::reset();

	curs_set(0);

	/*
	 * This is the main "Event" loop of newsboat.
	 */

	while (Formaction_stack_size() > 0) {
		// first, we take the current Formaction.
		std::shared_ptr<Formaction> fa = get_current_formaction();

		// we signal "oh, you will receive an operation soon"
		fa->prepare();

		if (!macrocmds.empty()) {
			// if there is any macro command left to process, we do
			// so

			fa->get_form()->run(-1);
			fa->process_op(
				macrocmds[0].op, true, &macrocmds[0].args);

			macrocmds.erase(macrocmds.begin()); // remove first
							    // macro command,
							    // since it has
							    // already been
							    // processed

		} else {
			// we then receive the Event and ignore timeouts.
			const char* Event = fa->get_form()->run(60000);

			if (ctrl_c_hit) {
				ctrl_c_hit = false;
				cancel_input(fa);
				if (!get_cfg()->get_configvalue_as_bool(
					    "confirm-exit") ||
					confirm(_("Do you really want to quit "
						  "(y:Yes n:No)? "),
						_("yn")) == *_("y")) {
					Stfl::reset();
					return EXIT_FAILURE;
				}
			}

			if (!Event || strcmp(Event, "TIMEOUT") == 0) {
				if (fa->id() == "article")
					std::dynamic_pointer_cast<
						ItemViewFormaction,
						Formaction>(fa)
						->update_percent();
				continue;
			}

			if (is_inside_qna) {
				LOG(Level::DEBUG,
					"View::run: we're inside QNA input");
				if (is_inside_cmdline &&
					strcmp(Event, "TAB") == 0) {
					handle_cmdline_completion(fa);
					continue;
				}
				if (strcmp(Event, "^U") == 0) {
					clear_line(fa);
					continue;
				} else if (strcmp(Event, "^K") == 0) {
					clear_eol(fa);
					continue;
				} else if (strcmp(Event, "^G") == 0) {
					cancel_input(fa);
					continue;
				} else if (strcmp(Event, "^W") == 0) {
					delete_word(fa);
					continue;
				}
			}

			LOG(Level::DEBUG, "View::run: Event = %s", Event);

			// retrieve operation code through the Keymap
			operation op;

			if (have_macroprefix) {
				have_macroprefix = false;
				LOG(Level::DEBUG,
					"View::run: running macro `%s'",
					Event);
				macrocmds = keys->get_macro(Event);
				set_status("");
			} else {
				op = keys->get_operation(Event, fa->id());

				LOG(Level::DEBUG,
					"View::run: Event = %s op = %u",
					Event,
					op);

				if (OP_MACROPREFIX == op) {
					have_macroprefix = true;
					set_status("macro-");
				}

				// now we handle the operation to the
				// Formaction.
				fa->process_op(op);
			}
		}
	}

	Stfl::reset();
	return EXIT_SUCCESS;
}

std::string View::run_modal(std::shared_ptr<Formaction> f,
	const std::string& value)
{
	f->init();
	unsigned int stacksize = Formaction_stack.size();

	Formaction_stack.push_back(f);
	current_formaction = Formaction_stack_size() - 1;

	while (Formaction_stack.size() > stacksize) {
		std::shared_ptr<Formaction> fa = get_current_formaction();

		fa->prepare();

		const char* Event = fa->get_form()->run(1000);
		LOG(Level::DEBUG, "View::run: Event = %s", Event);
		if (!Event || strcmp(Event, "TIMEOUT") == 0)
			continue;

		operation op = keys->get_operation(Event, fa->id());

		if (OP_REDRAW == op) {
			Stfl::reset();
			continue;
		}

		fa->process_op(op);
	}

	if (value == "")
		return "";
	else
		return f->get_value(value);
}

std::string View::get_filename_suggestion(const std::string& s)
{
	/*
	 * With this function, we generate normalized filenames for saving
	 * articles to files.
	 */
	std::string retval;
	for (unsigned int i = 0; i < s.length(); ++i) {
		if (isalnum(s[i]))
			retval.append(1, s[i]);
		else if (s[i] == '/' || s[i] == ' ' || s[i] == '\r' ||
			s[i] == '\n')
			retval.append(1, '_');
	}
	if (retval.length() == 0)
		retval = "article.txt";
	else
		retval.append(".txt");
	LOG(Level::DEBUG, "View::get_filename_suggestion: %s -> %s", s, retval);
	return retval;
}

void View::push_empty_formaction()
{
	Formaction_stack.push_back(std::shared_ptr<Formaction>());
	current_formaction = Formaction_stack_size() - 1;
}

void View::open_in_pager(const std::string& filename)
{
	Formaction_stack.push_back(std::shared_ptr<Formaction>());
	current_formaction = Formaction_stack_size() - 1;
	std::string cmdline;
	std::string pager = cfg->get_configvalue("pager");
	if (pager.find("%f") != std::string::npos) {
		FmtStrFormatter fmt;
		fmt.register_fmt('f', filename);
		cmdline = fmt.do_format(pager, 0);
	} else {
		const char* env_pager = nullptr;
		if (pager != "")
			cmdline.append(pager);
		else if ((env_pager = getenv("PAGER")) != nullptr)
			cmdline.append(env_pager);
		else
			cmdline.append("more");
		cmdline.append(" ");
		cmdline.append(filename);
	}
	Stfl::reset();
	Utils::run_interactively(cmdline, "View::open_in_pager");
	pop_current_formaction();
}

void View::open_in_browser(const std::string& url)
{
	Formaction_stack.push_back(std::shared_ptr<Formaction>());
	current_formaction = Formaction_stack_size() - 1;
	std::string cmdline;
	std::string browser = cfg->get_configvalue("browser");
	if (browser.find("%u") != std::string::npos) {
		FmtStrFormatter fmt;
		std::string newurl;
		newurl = Utils::replace_all(url, "'", "%27");
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
		cmdline.append(Utils::replace_all(url, "'", "%27"));
		cmdline.append("'");
	}
	Stfl::reset();
	Utils::run_interactively(cmdline, "View::open_in_browser");
	pop_current_formaction();
}

void View::update_visible_feeds(std::vector<std::shared_ptr<RssFeed>> feeds)
{
	try {
		if (Formaction_stack_size() > 0) {
			std::lock_guard<std::mutex> lock(mtx);
			std::shared_ptr<FeedListFormaction> feedlist =
				std::dynamic_pointer_cast<FeedListFormaction,
					Formaction>(Formaction_stack[0]);
			feedlist->update_visible_feeds(feeds);
		}
	} catch (const MatcherException& e) {
		set_status(StrPrintf::fmt(
			_("Error: applying the filter failed: %s"), e.what()));
		LOG(Level::DEBUG,
			"View::update_visible_feeds: inside catch: %s",
			e.what());
	}
}

void View::set_feedlist(std::vector<std::shared_ptr<RssFeed>> feeds)
{
	try {
		std::lock_guard<std::mutex> lock(mtx);

		for (const auto& feed : feeds) {
			if (!feed->is_query_feed()) {
				feed->set_feedptrs(feed);
			}
		}

		if (Formaction_stack_size() > 0) {
			std::shared_ptr<FeedListFormaction> feedlist =
				std::dynamic_pointer_cast<FeedListFormaction,
					Formaction>(Formaction_stack[0]);
			feedlist->set_feedlist(feeds);
		}
	} catch (const MatcherException& e) {
		set_status(StrPrintf::fmt(
			_("Error: applying the filter failed: %s"), e.what()));
	}
}

void View::set_tags(const std::vector<std::string>& t)
{
	tags = t;
}

void View::push_searchresult(std::shared_ptr<RssFeed> feed,
	const std::string& phrase)
{
	assert(feed != nullptr);
	LOG(Level::DEBUG, "View::push_searchresult: pushing search result");

	if (feed->total_item_count() > 0) {
		std::shared_ptr<ItemListFormaction> searchresult(
			new ItemListFormaction(this, itemlist_str));
		set_bindings(searchresult);
		searchresult->set_RegexManager(rxman);
		searchresult->set_feed(feed);
		searchresult->set_show_searchresult(true);
		searchresult->set_searchphrase(phrase);
		apply_colors(searchresult);
		searchresult->set_parent_formaction(get_current_formaction());
		searchresult->init();
		Formaction_stack.push_back(searchresult);
		current_formaction = Formaction_stack_size() - 1;
	} else {
		show_error(_("Error: feed contains no items!"));
	}
}

void View::push_itemlist(std::shared_ptr<RssFeed> feed)
{
	assert(feed != nullptr);

	feed->purge_deleted_items();

	prepare_query_feed(feed);

	if (feed->total_item_count() > 0) {
		std::shared_ptr<ItemListFormaction> itemlist(
			new ItemListFormaction(this, itemlist_str));
		set_bindings(itemlist);
		itemlist->set_RegexManager(rxman);
		itemlist->set_feed(feed);
		itemlist->set_show_searchresult(false);
		apply_colors(itemlist);
		itemlist->set_parent_formaction(get_current_formaction());
		itemlist->init();
		Formaction_stack.push_back(itemlist);
		current_formaction = Formaction_stack_size() - 1;
	} else {
		show_error(_("Error: feed contains no items!"));
	}
}

void View::push_itemlist(unsigned int pos)
{
	std::shared_ptr<RssFeed> feed =
		ctrl->get_feedcontainer()->get_feed(pos);
	LOG(Level::DEBUG,
		"View::push_itemlist: retrieved feed at position %d",
		pos);
	push_itemlist(feed);
	if (feed->total_item_count() > 0) {
		std::shared_ptr<ItemListFormaction> itemlist =
			std::dynamic_pointer_cast<ItemListFormaction,
				Formaction>(get_current_formaction());
		itemlist->set_pos(pos);
	}
}

void View::push_itemView(std::shared_ptr<RssFeed> f,
	const std::string& guid,
	const std::string& searchphrase)
{
	if (cfg->get_configvalue("pager") == "internal") {
		auto fa = get_current_formaction();

		std::shared_ptr<ItemListFormaction> itemlist =
			std::dynamic_pointer_cast<ItemListFormaction,
				Formaction>(fa);
		assert(itemlist != nullptr);
		std::shared_ptr<ItemViewFormaction> itemView(
			new ItemViewFormaction(this, itemlist, itemview_str));
		set_bindings(itemView);
		itemView->set_RegexManager(rxman);
		itemView->set_feed(f);
		itemView->set_guid(guid);
		itemView->set_parent_formaction(fa);
		if (searchphrase.length() > 0)
			itemView->set_highlightphrase(searchphrase);
		apply_colors(itemView);
		itemView->init();
		Formaction_stack.push_back(itemView);
		current_formaction = Formaction_stack_size() - 1;
	} else {
		std::shared_ptr<RssItem> item = f->get_item_by_guid(guid);
		std::string filename = get_ctrl()->write_temporary_item(item);
		open_in_pager(filename);
		try {
			bool old_unread = item->unread();
			item->set_unread(false);
			if (old_unread) {
				get_ctrl()->mark_article_read(
					item->guid(), true);
			}
		} catch (const DbException& e) {
			show_error(StrPrintf::fmt(
				_("Error while marking article as read: %s"),
				e.what()));
		}
		::unlink(filename.c_str());
	}
}

void View::View_dialogs()
{
	auto fa = get_current_formaction();
	if (fa != nullptr && fa->id() != "dialogs") {
		std::shared_ptr<DialogsFormaction> dialogs(
			new DialogsFormaction(this, dialogs_str));
		dialogs->set_parent_formaction(fa);
		apply_colors(dialogs);
		dialogs->init();
		Formaction_stack.push_back(dialogs);
		current_formaction = Formaction_stack_size() - 1;
	}
}

void View::push_help()
{
	auto fa = get_current_formaction();

	std::shared_ptr<HelpFormaction> helpView(
		new HelpFormaction(this, help_str));
	set_bindings(helpView);
	apply_colors(helpView);
	helpView->set_context(fa->id());
	helpView->set_parent_formaction(fa);
	helpView->init();
	Formaction_stack.push_back(helpView);
	current_formaction = Formaction_stack_size() - 1;
}

void View::push_urlView(const std::vector<linkpair>& links,
	std::shared_ptr<RssFeed>& feed)
{
	std::shared_ptr<UrlViewFormaction> urlView(
		new UrlViewFormaction(this, feed, urlview_str));
	set_bindings(urlView);
	apply_colors(urlView);
	urlView->set_parent_formaction(get_current_formaction());
	urlView->init();
	urlView->set_links(links);
	Formaction_stack.push_back(urlView);
	current_formaction = Formaction_stack_size() - 1;
}

std::string View::run_filebrowser(const std::string& default_filename,
	const std::string& dir)
{
	std::shared_ptr<FileBrowserFormaction> filebrowser(
		new FileBrowserFormaction(this, filebrowser_str));
	set_bindings(filebrowser);
	apply_colors(filebrowser);
	filebrowser->set_dir(dir);
	filebrowser->set_default_filename(default_filename);
	filebrowser->set_parent_formaction(get_current_formaction());
	return run_modal(filebrowser, "filenametext");
}

std::string View::select_tag()
{
	if (tags.size() == 0) {
		show_error(_("No tags defined."));
		return "";
	}
	std::shared_ptr<SelectFormaction> selecttag(
		new SelectFormaction(this, selecttag_str));
	selecttag->set_type(SelectFormaction::SelectionType::TAG);
	set_bindings(selecttag);
	apply_colors(selecttag);
	selecttag->set_parent_formaction(get_current_formaction());
	selecttag->set_tags(tags);
	run_modal(selecttag, "");
	return selecttag->get_selected_value();
}

std::string View::select_filter(
	const std::vector<filter_name_expr_pair>& filters)
{
	std::shared_ptr<SelectFormaction> selecttag(
		new SelectFormaction(this, selecttag_str));
	selecttag->set_type(SelectFormaction::SelectionType::FILTER);
	set_bindings(selecttag);
	apply_colors(selecttag);
	selecttag->set_parent_formaction(get_current_formaction());
	selecttag->set_filters(filters);
	run_modal(selecttag, "");
	return selecttag->get_selected_value();
}

char View::confirm(const std::string& prompt, const std::string& charset)
{
	LOG(Level::DEBUG, "View::confirm: charset = %s", charset);

	std::shared_ptr<Formaction> f = get_current_formaction();
	Formaction_stack.push_back(std::shared_ptr<Formaction>());
	current_formaction = Formaction_stack_size() - 1;
	f->get_form()->set("msg", prompt);

	char result = 0;

	do {
		const char* Event = f->get_form()->run(0);
		LOG(Level::DEBUG, "View::confirm: Event = %s", Event);
		if (!Event)
			continue;
		if (strcmp(Event, "ESC") == 0 || strcmp(Event, "ENTER") == 0) {
			result = 0;
			LOG(Level::DEBUG,
				"View::confirm: user pressed ESC or ENTER, we "
				"cancel confirmation dialog");
			break;
		}
		result = keys->get_key(Event);
		LOG(Level::DEBUG,
			"View::confirm: key = %c (%u)",
			result,
			result);
	} while (!result || strchr(charset.c_str(), result) == nullptr);

	f->get_form()->set("msg", "");
	f->get_form()->run(-1);

	pop_current_formaction();

	return result;
}

void View::notify_itemlist_change(std::shared_ptr<RssFeed> feed)
{
	for (const auto& Form : Formaction_stack) {
		if (Form != nullptr && Form->id() == "articlelist") {
			std::shared_ptr<ItemListFormaction> itemlist =
				std::dynamic_pointer_cast<ItemListFormaction,
					Formaction>(Form);
			if (itemlist != nullptr) {
				std::shared_ptr<RssFeed> f =
					itemlist->get_feed();
				if (f != nullptr &&
					f->rssurl() == feed->rssurl()) {
					itemlist->set_feed(feed);
					itemlist->set_redraw(true);
				}
			}
		}
	}
}

bool View::get_random_unread(ItemListFormaction* itemlist,
	ItemViewFormaction* itemView)
{
	unsigned int feedpos;
	std::shared_ptr<FeedListFormaction> feedlist =
		std::dynamic_pointer_cast<FeedListFormaction, Formaction>(
			Formaction_stack[0]);
	if (!cfg->get_configvalue_as_bool("goto-next-feed")) {
		return false;
	}
	if (feedlist->jump_to_random_unread_feed(feedpos)) {
		LOG(Level::DEBUG,
			"View::get_previous_unread: found feed with unread "
			"articles");
		prepare_query_feed(feedlist->get_feed());
		itemlist->set_feed(feedlist->get_feed());
		itemlist->set_pos(feedpos);
		itemlist->init();
		if (itemlist->jump_to_random_unread_item()) {
			if (itemView) {
				itemView->init();
				itemView->set_feed(itemlist->get_feed());
				itemView->set_guid(itemlist->get_guid());
			}
			return true;
		}
	}
	return false;
}

bool View::get_previous_unread(ItemListFormaction* itemlist,
	ItemViewFormaction* itemView)
{
	unsigned int feedpos;
	LOG(Level::DEBUG,
		"View::get_previous_unread: trying to find previous unread");
	std::shared_ptr<FeedListFormaction> feedlist =
		std::dynamic_pointer_cast<FeedListFormaction, Formaction>(
			Formaction_stack[0]);
	if (itemlist->jump_to_previous_unread_item(false)) {
		LOG(Level::DEBUG,
			"View::get_previous_unread: found unread article in "
			"same "
			"feed");
		if (itemView) {
			itemView->init();
			itemView->set_feed(itemlist->get_feed());
			itemView->set_guid(itemlist->get_guid());
		}
		return true;
	} else if (cfg->get_configvalue_as_bool("goto-next-feed") == false) {
		LOG(Level::DEBUG,
			"View::get_previous_unread: goto-next-feed = false");
		show_error(_("No unread items."));
	} else if (feedlist->jump_to_previous_unread_feed(feedpos)) {
		LOG(Level::DEBUG,
			"View::get_previous_unread: found feed with unread "
			"articles");
		prepare_query_feed(feedlist->get_feed());
		itemlist->set_feed(feedlist->get_feed());
		itemlist->set_pos(feedpos);
		itemlist->init();
		if (itemlist->jump_to_previous_unread_item(true)) {
			if (itemView) {
				itemView->init();
				itemView->set_feed(itemlist->get_feed());
				itemView->set_guid(itemlist->get_guid());
			}
			return true;
		}
	}
	return false;
}

bool View::get_next_unread_feed(ItemListFormaction* itemlist)
{
	std::shared_ptr<FeedListFormaction> feedlist =
		std::dynamic_pointer_cast<FeedListFormaction, Formaction>(
			Formaction_stack[0]);
	unsigned int feedpos;
	assert(feedlist != nullptr);
	if (feedlist->jump_to_next_unread_feed(feedpos)) {
		prepare_query_feed(feedlist->get_feed());
		itemlist->set_feed(feedlist->get_feed());
		itemlist->set_pos(feedpos);
		itemlist->init();
		return true;
	}
	return false;
}

bool View::get_prev_unread_feed(ItemListFormaction* itemlist)
{
	std::shared_ptr<FeedListFormaction> feedlist =
		std::dynamic_pointer_cast<FeedListFormaction, Formaction>(
			Formaction_stack[0]);
	unsigned int feedpos;
	assert(feedlist != nullptr);
	if (feedlist->jump_to_previous_unread_feed(feedpos)) {
		prepare_query_feed(feedlist->get_feed());
		itemlist->set_feed(feedlist->get_feed());
		itemlist->set_pos(feedpos);
		itemlist->init();
		return true;
	}
	return false;
}

bool View::get_next_unread(ItemListFormaction* itemlist,
	ItemViewFormaction* itemView)
{
	unsigned int feedpos;
	std::shared_ptr<FeedListFormaction> feedlist =
		std::dynamic_pointer_cast<FeedListFormaction, Formaction>(
			Formaction_stack[0]);
	LOG(Level::DEBUG, "View::get_next_unread: trying to find next unread");
	if (itemlist->jump_to_next_unread_item(false)) {
		LOG(Level::DEBUG,
			"View::get_next_unread: found unread article in same "
			"feed");
		if (itemView) {
			itemView->init();
			itemView->set_feed(itemlist->get_feed());
			itemView->set_guid(itemlist->get_guid());
		}
		return true;
	} else if (cfg->get_configvalue_as_bool("goto-next-feed") == false) {
		LOG(Level::DEBUG,
			"View::get_next_unread: goto-next-feed = false");
		show_error(_("No unread items."));
	} else if (feedlist->jump_to_next_unread_feed(feedpos)) {
		LOG(Level::DEBUG,
			"View::get_next_unread: found feed with unread "
			"articles");
		prepare_query_feed(feedlist->get_feed());
		itemlist->set_feed(feedlist->get_feed());
		itemlist->set_pos(feedpos);
		itemlist->init();
		if (itemlist->jump_to_next_unread_item(true)) {
			if (itemView) {
				itemView->init();
				itemView->set_feed(itemlist->get_feed());
				itemView->set_guid(itemlist->get_guid());
			}
			return true;
		}
	}
	return false;
}

bool View::get_previous(ItemListFormaction* itemlist,
	ItemViewFormaction* itemView)
{
	unsigned int feedpos;
	std::shared_ptr<FeedListFormaction> feedlist =
		std::dynamic_pointer_cast<FeedListFormaction, Formaction>(
			Formaction_stack[0]);
	if (itemlist->jump_to_previous_item(false)) {
		LOG(Level::DEBUG, "View::get_previous: article in same feed");
		if (itemView) {
			itemView->init();
			itemView->set_feed(itemlist->get_feed());
			itemView->set_guid(itemlist->get_guid());
		}
		return true;
	} else if (cfg->get_configvalue_as_bool("goto-next-feed") == false) {
		LOG(Level::DEBUG, "View::get_previous: goto-next-feed = false");
		show_error(_("Already on first item."));
	} else if (feedlist->jump_to_previous_feed(feedpos)) {
		LOG(Level::DEBUG, "View::get_previous: previous feed");
		prepare_query_feed(feedlist->get_feed());
		itemlist->set_feed(feedlist->get_feed());
		itemlist->set_pos(feedpos);
		itemlist->init();
		if (itemlist->jump_to_previous_item(true)) {
			if (itemView) {
				itemView->init();
				itemView->set_feed(itemlist->get_feed());
				itemView->set_guid(itemlist->get_guid());
			}
			return true;
		}
	}
	return false;
}

bool View::get_next(ItemListFormaction* itemlist,
	ItemViewFormaction* itemView)
{
	unsigned int feedpos;
	std::shared_ptr<FeedListFormaction> feedlist =
		std::dynamic_pointer_cast<FeedListFormaction, Formaction>(
			Formaction_stack[0]);
	if (itemlist->jump_to_next_item(false)) {
		LOG(Level::DEBUG, "View::get_next: article in same feed");
		if (itemView) {
			itemView->init();
			itemView->set_feed(itemlist->get_feed());
			itemView->set_guid(itemlist->get_guid());
		}
		return true;
	} else if (cfg->get_configvalue_as_bool("goto-next-feed") == false) {
		LOG(Level::DEBUG, "View::get_next: goto-next-feed = false");
		show_error(_("Already on last item."));
	} else if (feedlist->jump_to_next_feed(feedpos)) {
		LOG(Level::DEBUG, "View::get_next: next feed");
		prepare_query_feed(feedlist->get_feed());
		itemlist->set_feed(feedlist->get_feed());
		itemlist->set_pos(feedpos);
		itemlist->init();
		if (itemlist->jump_to_next_item(true)) {
			if (itemView) {
				itemView->init();
				itemView->set_feed(itemlist->get_feed());
				itemView->set_guid(itemlist->get_guid());
			}
			return true;
		}
	}
	return false;
}

bool View::get_next_feed(ItemListFormaction* itemlist)
{
	std::shared_ptr<FeedListFormaction> feedlist =
		std::dynamic_pointer_cast<FeedListFormaction, Formaction>(
			Formaction_stack[0]);
	unsigned int feedpos;
	assert(feedlist != nullptr);
	if (feedlist->jump_to_next_feed(feedpos)) {
		prepare_query_feed(feedlist->get_feed());
		itemlist->set_feed(feedlist->get_feed());
		itemlist->set_pos(feedpos);
		itemlist->init();
		return true;
	}
	return false;
}

bool View::get_prev_feed(ItemListFormaction* itemlist)
{
	std::shared_ptr<FeedListFormaction> feedlist =
		std::dynamic_pointer_cast<FeedListFormaction, Formaction>(
			Formaction_stack[0]);
	unsigned int feedpos;
	assert(feedlist != nullptr);
	if (feedlist->jump_to_previous_feed(feedpos)) {
		prepare_query_feed(feedlist->get_feed());
		itemlist->set_feed(feedlist->get_feed());
		itemlist->set_pos(feedpos);
		itemlist->init();
		return true;
	}
	return false;
}

void View::prepare_query_feed(std::shared_ptr<RssFeed> feed)
{
	if (feed->is_query_feed()) {
		LOG(Level::DEBUG,
			"View::prepare_query_feed: %s",
			feed->rssurl());

		set_status(_("Updating query feed..."));
		feed->update_items(ctrl->get_feedcontainer()->get_all_feeds());
		feed->sort(cfg->get_article_sort_strategy());
		notify_itemlist_change(feed);
		set_status("");
	}
}

void View::force_redraw()
{
	std::shared_ptr<Formaction> fa = get_current_formaction();
	if (fa != nullptr) {
		fa->set_redraw(true);
		fa->prepare();
		fa->get_form()->run(-1);
	}
}

void View::pop_current_formaction()
{
	std::shared_ptr<Formaction> f = get_current_formaction();
	auto it = Formaction_stack.begin();
	for (unsigned int i = 0; i < current_formaction; i++)
		++it;
	Formaction_stack.erase(it);
	if (f == nullptr) {
		current_formaction = Formaction_stack_size() -
			1; // XXX TODO this is not correct...
			   // we'd need to return to the previous
			   // one, but nullptr Formactions have
			   // no parent
	} else if (Formaction_stack.size() > 0) {
		// first, we set back the parent Formactions of those who
		// reference the Formaction we just removed
		for (const auto& Form : Formaction_stack) {
			if (Form->get_parent_formaction() == f) {
				Form->set_parent_formaction(
					Formaction_stack[0]);
			}
		}
		// we set the new Formaction based on the removed Formaction's
		// parent.
		unsigned int i = 0;
		for (const auto& Form : Formaction_stack) {
			if (Form == f->get_parent_formaction()) {
				current_formaction = i;
				break;
			}
			i++;
		}
		std::shared_ptr<Formaction> f = get_current_formaction();
		if (f) {
			f->set_redraw(true);
			f->get_form()->set("msg", "");
			f->recalculate_form();
		}
	}
}

void View::set_current_formaction(unsigned int pos)
{
	remove_formaction(current_formaction);
	current_formaction = pos;
}

void View::remove_formaction(unsigned int pos)
{
	std::shared_ptr<Formaction> f = Formaction_stack[pos];
	auto it = Formaction_stack.begin();
	for (unsigned int i = 0; i < pos; i++)
		++it;
	Formaction_stack.erase(it);
	current_formaction--;
	if (f != nullptr && Formaction_stack.size() > 0) {
		// we set back the parent Formactions of those who reference the
		// Formaction we just removed
		for (const auto& Form : Formaction_stack) {
			if (Form->get_parent_formaction() == f) {
				Form->set_parent_formaction(
					Formaction_stack[0]);
			}
		}
	}
}

void View::set_colors(std::map<std::string, std::string>& fgc,
	std::map<std::string, std::string>& bgc,
	std::map<std::string, std::vector<std::string>>& attribs)
{
	fg_colors = fgc;
	bg_colors = bgc;
	attributes = attribs;
}

void View::apply_colors_to_all_formactions()
{
	for (const auto& Form : Formaction_stack) {
		apply_colors(Form);
	}
	if (Formaction_stack.size() > 0 &&
		Formaction_stack[current_formaction]) {
		Formaction_stack[current_formaction]->set_redraw(true);
	}
}

void View::apply_colors(std::shared_ptr<Formaction> fa)
{
	auto fgcit = fg_colors.begin();
	auto bgcit = bg_colors.begin();
	auto attit = attributes.begin();

	LOG(Level::DEBUG, "View::apply_colors: fa = %s", fa->id());

	std::string article_colorstr;

	for (; fgcit != fg_colors.end(); ++fgcit, ++bgcit, ++attit) {
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
		for (const auto& attr : attit->second) {
			if (colorattr.length() > 0)
				colorattr.append(",");
			colorattr.append("attr=");
			colorattr.append(attr);
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
				fa->get_form()->set(
					"color_underline", ul.c_str());
			}
		}

		LOG(Level::DEBUG,
			"View::apply_colors: %s %s %s\n",
			fa->id(),
			fgcit->first,
			colorattr);

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

				fa->get_form()->set(
					"styleend", styleend_str.c_str());
			}
		}
	}
}

void View::feedlist_mark_pos_if_visible(unsigned int pos)
{
	if (Formaction_stack_size() > 0) {
		std::dynamic_pointer_cast<FeedListFormaction, Formaction>(
			Formaction_stack[0])
			->mark_pos_if_visible(pos);
	}
}

void View::set_RegexManager(RegexManager* r)
{
	rxman = r;
}

std::vector<std::pair<unsigned int, std::string>> View::get_formaction_names()
{
	std::vector<std::pair<unsigned int, std::string>> Formaction_names;
	unsigned int i = 0;
	for (const auto& Form : Formaction_stack) {
		if (Form && Form->id() != "dialogs") {
			Formaction_names.push_back(
				std::pair<unsigned int, std::string>(
					i, Form->title()));
		}
		i++;
	}
	return Formaction_names;
}

void View::goto_next_dialog()
{
	current_formaction++;
	if (current_formaction >= Formaction_stack.size())
		current_formaction = 0;
}

void View::goto_prev_dialog()
{
	if (current_formaction > 0) {
		current_formaction--;
	} else {
		current_formaction = Formaction_stack.size() - 1;
	}
}

void View::inside_qna(bool f)
{
	curs_set(f ? 1 : 0);
	is_inside_qna = f;
}

void View::inside_cmdline(bool f)
{
	is_inside_cmdline = f;
}

void View::clear_line(std::shared_ptr<Formaction> fa)
{
	fa->get_form()->set("qna_value", "");
	fa->get_form()->set("qna_value_pos", "0");
	LOG(Level::DEBUG, "View::clear_line: cleared line");
}

void View::clear_eol(std::shared_ptr<Formaction> fa)
{
	unsigned int pos = Utils::to_u(fa->get_form()->get("qna_value_pos"), 0);
	std::string val = fa->get_form()->get("qna_value");
	val.erase(pos, val.length());
	fa->get_form()->set("qna_value", val);
	fa->get_form()->set("qna_value_pos", std::to_string(val.length()));
	LOG(Level::DEBUG, "View::clear_eol: cleared to end of line");
}

void View::cancel_input(std::shared_ptr<Formaction> fa)
{
	fa->process_op(OP_INT_CANCEL_QNA);
	LOG(Level::DEBUG, "View::cancel_input: cancelled input");
}

void View::delete_word(std::shared_ptr<Formaction> fa)
{
	std::string::size_type curpos =
		Utils::to_u(fa->get_form()->get("qna_value_pos"), 0);
	std::string val = fa->get_form()->get("qna_value");
	std::string::size_type firstpos = curpos;
	LOG(Level::DEBUG, "View::delete_word: before val = %s", val);
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
	LOG(Level::DEBUG, "View::delete_word: after val = %s", val);
	fa->get_form()->set("qna_value", val);
	fa->get_form()->set("qna_value_pos", std::to_string(firstpos));
}

void View::handle_cmdline_completion(std::shared_ptr<Formaction> fa)
{
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
		LOG(Level::DEBUG,
			"View::handle_cmdline_completion: found no suggestion "
			"for "
			"`%s'",
			fragment);
		::beep(); // direct call to ncurses - we beep to signal that
			  // there is no suggestion available, just like vim
		return;
	case 1:
		suggestion = suggestions[0];
		break;
	default:
		suggestion = suggestions[(tab_count - 1) % suggestions.size()];
		break;
	}
	fa->get_form()->set("qna_value", suggestion);
	fa->get_form()->set(
		"qna_value_pos", std::to_string(suggestion.length()));
	last_fragment = suggestion;
}

void View::dump_current_form()
{
	std::string Formtext =
		Formaction_stack[current_formaction]->get_form()->dump(
			"", "", 0);
	char fnbuf[128];
	time_t t = time(nullptr);
	struct tm* stm = localtime(&t);
	strftime(fnbuf, sizeof(fnbuf), "dumpForm-%Y%m%d-%H%M%S.Stfl", stm);
	std::fstream f(fnbuf, std::ios_base::out);
	if (!f.is_open()) {
		show_error(StrPrintf::fmt("Error: couldn't open file %s: %s",
			fnbuf,
			strerror(errno)));
		return;
	}
	f << Formtext;
	f.close();
	set_status(StrPrintf::fmt("Dumped current Form to file %s", fnbuf));
}

void View::ctrl_c_action(int /* sig */)
{
	LOG(Level::DEBUG, "caught SIGINT");
	ctrl_c_hit = true;
}

} // namespace newsboat

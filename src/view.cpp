#include "view.h"

#include <assert.h>
#include <cstring>
#include <dirent.h>
#include <grp.h>
#include <iostream>
#include <libgen.h>
#include <limits.h>
#include <ncurses.h>
#include <optional>
#include <pwd.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>

extern "C" {
#include <stfl.h>
}

#include "config.h"
#include "colormanager.h"
#include "controller.h"
#include "configcontainer.h"
#include "dbexception.h"
#include "dialogs.h"
#include "dialogsformaction.h"
#include "dirbrowserformaction.h"
#include "emptyformaction.h"
#include "empty.h"
#include "feedlist.h"
#include "feedlistformaction.h"
#include "filebrowser.h"
#include "filebrowserformaction.h"
#include "fmtstrformatter.h"
#include "formaction.h"
#include "help.h"
#include "helpformaction.h"
#include "itemlist.h"
#include "itemlistformaction.h"
#include "itemview.h"
#include "itemviewformaction.h"
#include "keycombination.h"
#include "keymap.h"
#include "logger.h"
#include "matcherexception.h"
#include "rssfeed.h"
#include "selectformaction.h"
#include "selecttag.h"
#include "strprintf.h"
#include "urlview.h"
#include "urlviewformaction.h"
#include "utils.h"
#include "searchresultslistformaction.h"

namespace {
bool ctrl_c_hit = false;
}

namespace newsboat {

View::View(Controller& c)
	: ctrl(c)
	, cfg(0)
	, keys(0)
	, current_formaction(0)
	, status_line(*this)
	, rxman(c.get_regexmanager())
	, is_inside_qna(false)
	, is_inside_cmdline(false)
	, rsscache(nullptr)
	, filters(ctrl.get_filtercontainer())
	, colorman(ctrl.get_colormanager())
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

void View::set_keymap(KeyMap* k)
{
	keys = k;
}

std::shared_ptr<FormAction> View::get_current_formaction()
{
	if (formaction_stack.size() > 0 &&
		current_formaction < formaction_stack_size()) {
		return formaction_stack[current_formaction];
	} else {
		return {};
	}
}

void View::set_status(const std::string& msg)
{
	std::lock_guard<std::mutex> lock(mtx);

	auto fa = get_current_formaction();
	if (fa != nullptr
		&& std::dynamic_pointer_cast<EmptyFormAction>(fa) == nullptr) {
		fa->set_status(msg);
		fa->draw_form();
	}
}

void View::show_error(const std::string& msg)
{
	set_status(msg);
}

StatusLine& View::get_statusline()
{
	return status_line;
}

bool View::run_commands(const std::vector<MacroCmd>& commands, BindingType binding_type)
{
	for (auto command : commands) {
		if (formaction_stack_size() == 0) {
			return true;
		}
		std::shared_ptr<FormAction> fa = get_current_formaction();
		fa->prepare();
		fa->draw_form();
		if (!fa->process_op(command.op, command.args, binding_type)) {
			// Operation failed, abort
			return false;
		}
	}
	return true;
}

int View::run()
{
	bool have_macroprefix = false;

	feedlist_form = std::make_shared<FeedListFormAction>(
			*this, feedlist_str, rsscache, filters, cfg, rxman);
	apply_colors(feedlist_form);
	formaction_stack.push_back(feedlist_form);
	current_formaction = formaction_stack_size() - 1;

	get_current_formaction()->init();

	Stfl::reset();

	curs_set(0);

	if (!run_commands(keys->get_startup_operation_sequence(), BindingType::Macro)) {
		Stfl::reset();
		std::cerr << _("Error: failed to execute startup commands") << std::endl;
		return EXIT_FAILURE;
	}

	/*
	 * This is the main "event" loop of newsboat.
	 */

	std::vector<KeyCombination> key_sequence;
	while (formaction_stack_size() > 0) {
		// first, we take the current formaction.
		std::shared_ptr<FormAction> fa = get_current_formaction();

		// we signal "oh, you will receive an operation soon"
		fa->prepare();

		// we then receive the event and ignore timeouts.
		const std::string event = fa->draw_form_wait_for_event(INT_MAX);

		if (ctrl_c_hit) {
			ctrl_c_hit = false;
			fa->cancel_qna();
			if (!get_cfg()->get_configvalue_as_bool(
					"confirm-exit") ||
				confirm(_("Do you really want to quit "
						"(y:Yes n:No)? "),
					_("yn")) == *_("y")) {
				Stfl::reset();
				return EXIT_FAILURE;
			}
		}

		if (event.empty() || event == "TIMEOUT") {
			continue;
		}

		if (event == "RESIZE") {
			handle_resize();
			continue;
		}

		if (handle_qna_event(event, fa)) {
			continue;
		}

		LOG(Level::DEBUG, "View::run: event = %s", event);

		const auto key_combination = KeyCombination::from_bindkey(event);
		if (have_macroprefix) {
			have_macroprefix = false;
			status_line.show_message("");
			LOG(Level::DEBUG,
				"View::run: running macro `%s'",
				event);
			run_commands(keys->get_macro(key_combination), BindingType::Macro);
		} else {
			if (key_combination == KeyCombination("ESC") && !key_sequence.empty()) {
				key_sequence.clear();
			} else {
				key_sequence.push_back(key_combination);
			}
			auto binding_state = MultiKeyBindingState::NotFound;
			BindingType type = BindingType::Bind;
			auto cmds = keys->get_operation(key_sequence, fa->id(), binding_state, type);

			if (binding_state != MultiKeyBindingState::MoreInputNeeded) {
				key_sequence.clear();

				run_commands(cmds, type);

				if (cmds.size() >= 1 && cmds.back().op == OP_MACROPREFIX) {
					have_macroprefix = true;
					status_line.show_message("macro-");
				}
			}
		}
	}

	feedlist_form.reset();

	Stfl::reset();
	return EXIT_SUCCESS;
}

std::string View::run_modal(std::shared_ptr<FormAction> f,
	const std::string& value)
{
	// Modal dialogs should not allow changing to a different dialog (except by
	// closing the modal dialog)
	const std::set<Operation> ignoredOperations = {
		OP_VIEWDIALOGS,
		OP_NEXTDIALOG,
		OP_PREVDIALOG,
	};

	f->init();
	unsigned int stacksize = formaction_stack.size();

	formaction_stack.push_back(f);
	current_formaction = formaction_stack_size() - 1;

	std::vector<KeyCombination> key_sequence;
	while (formaction_stack.size() > stacksize) {
		std::shared_ptr<FormAction> fa = get_current_formaction();

		fa->prepare();

		const std::string event = fa->draw_form_wait_for_event(INT_MAX);
		LOG(Level::DEBUG, "View::run: event = %s", event);
		if (event.empty() || event == "TIMEOUT") {
			continue;
		}

		if (event == "RESIZE") {
			handle_resize();
			continue;
		}

		if (handle_qna_event(event, fa)) {
			continue;
		}

		const auto key_combination = KeyCombination::from_bindkey(event);
		if (key_combination == KeyCombination("ESC") && !key_sequence.empty()) {
			key_sequence.clear();
		} else {
			key_sequence.push_back(key_combination);
		}

		auto binding_state = MultiKeyBindingState::NotFound;
		BindingType type = BindingType::Bind;
		auto cmds = keys->get_operation(key_sequence, fa->id(), binding_state, type);

		if (binding_state != MultiKeyBindingState::MoreInputNeeded) {
			key_sequence.clear();

			for (auto command : cmds) {
				if (formaction_stack_size() == 0) {
					break;
				}

				if (OP_REDRAW == command.op) {
					Stfl::reset();
					continue;
				}

				if (ignoredOperations.count(command.op)) {
					status_line.show_message(_("Operation ignored in modal dialog"));
					break;
				}

				std::shared_ptr<FormAction> fa = get_current_formaction();
				fa->prepare();
				fa->draw_form();
				if (!fa->process_op(command.op, command.args, type)) {
					// Operation failed, don't run further commands
					break;
				}

				if (formaction_stack.size() <= stacksize) {
					// Stop running further commands if the current modal FormAction gets closed
					break;
				}
			}
		}
	}

	if (value.empty()) {
		return "";
	} else {
		return f->get_value(value);
	}
}

Filepath View::get_filename_suggestion(const std::string& s)
{
	/*
	 * With this function, we generate normalized filenames for saving
	 * articles to files if the setting `restrict-filename` is enabled.
	 */
	std::string suggestion;
	if (cfg->get_configvalue_as_bool("restrict-filename")) {
		for (unsigned int i = 0; i < s.length(); ++i) {
			if (isalnum(s[i])) {
				suggestion.push_back(s[i]);
			} else if (s[i] == '/' || s[i] == ' ' || s[i] == '\r' || s[i] == '\n') {
				suggestion.push_back('_');
			}
		}
	} else {
		suggestion = s;
	}

	Filepath retval;
	if (suggestion.empty()) {
		retval = "article.txt"_path;
	} else {
		retval = Filepath::from_locale_string(suggestion);
		retval.add_extension("txt");
	}
	LOG(Level::DEBUG, "View::get_filename_suggestion: %s -> %s", s, retval);
	return retval;
}

void View::drop_queued_input()
{
	flushinp();
}

void View::open_in_pager(const Filepath& filename)
{
	std::string cmdline;
	const auto pager = cfg->get_configvalue_as_filepath("pager").to_locale_string();
	if (pager.find("%f") != std::string::npos) {
		FmtStrFormatter fmt;
		fmt.register_fmt('f', filename.to_locale_string());
		cmdline = fmt.do_format(pager, 0);
	} else {
		const char* env_pager = nullptr;
		if (pager != "") {
			cmdline.append(pager);
		} else if ((env_pager = getenv("PAGER")) != nullptr) {
			cmdline.append(env_pager);
		} else {
			cmdline.append("more");
		}
		cmdline.append(" ");
		cmdline.append(filename.to_locale_string());
	}
	push_empty_formaction();
	Stfl::reset();
	utils::run_interactively(cmdline, "View::open_in_pager");
	drop_queued_input();
	pop_current_formaction();
}

std::optional<std::uint8_t> View::open_in_browser(const std::string& url,
	const std::string& feedurl, const std::string& type, const std::string& title,
	bool interactive)
{
	std::string cmdline;
	const auto browser = cfg->get_configvalue_as_filepath("browser");
	const std::string escaped_url = "'" + utils::replace_all(url, "'", "%27") + "'";
	const std::string escaped_feedurl = "'" + utils::replace_all(feedurl, "'",
			"%27") + "'";
	const std::string quoted_type = "'" + type + "'";
	const std::string escaped_title = utils::preserve_quotes(title);

	const auto browser_str = browser.to_locale_string();
	if (browser_str.find("%u") != std::string::npos
		|| browser_str.find("%F") != std::string::npos
		|| browser_str.find("%t") != std::string::npos
		|| browser_str.find("%T") != std::string::npos) {
		cmdline = utils::replace_all(browser_str, {
			{"%u", escaped_url},
			{"%F", escaped_feedurl},
			{"%t", quoted_type},
			{"%T", escaped_title}
		});
	} else {
		if (browser != Filepath()) {
			cmdline = browser_str;
		} else {
			cmdline = "lynx";
		}
		cmdline.append(" " + escaped_url);
	}

	if (interactive) {
		push_empty_formaction();
		Stfl::reset();
		const auto ret = utils::run_interactively(cmdline, "View::open_in_browser");
		drop_queued_input();
		pop_current_formaction();
		return ret;
	} else {
		const std::shared_ptr<AutoDiscardMessage> message =
			status_line.show_message_until_finished(strprintf::fmt(_("Running browser: %s"), cmdline));
		return utils::run_non_interactively(cmdline, "View::open_in_browser");
	}
}

void View::update_visible_feeds(std::vector<std::shared_ptr<RssFeed>> feeds)
{
	try {
		if (feedlist_form != nullptr) {
			std::lock_guard<std::mutex> lock(mtx);
			feedlist_form->update_visible_feeds(feeds);
		}
	} catch (const MatcherException& e) {
		status_line.show_message(strprintf::fmt(
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

		if (feedlist_form != nullptr) {
			feedlist_form->set_feedlist(feeds);
		}
	} catch (const MatcherException& e) {
		status_line.show_message(strprintf::fmt(
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
		if (this->get_current_formaction()->id() != "searchresultslist") {
			auto searchresult = std::make_shared<SearchResultsListFormAction>(
					*this, itemlist_str, rsscache, filters, cfg, rxman);
			apply_colors(searchresult);
			searchresult->set_parent_formaction(get_current_formaction());
			searchresult->add_to_history(feed, phrase);
			searchresult->init();
			formaction_stack.push_back(searchresult);
			current_formaction = formaction_stack_size() - 1;
		} else {
			auto searchresult = std::static_pointer_cast<SearchResultsListFormAction>
				(this->get_current_formaction());
			searchresult->add_to_history(feed, phrase);
		}
	} else {
		status_line.show_error(_("Error: feed contains no items!"));
	}
}

std::shared_ptr<ItemListFormAction> View::push_itemlist(
	std::shared_ptr<RssFeed> feed)
{
	assert(feed != nullptr);

	feed->purge_deleted_items();

	if (!try_prepare_query_feed(feed)) {
		return nullptr;
	}

	if (feed->total_item_count() > 0) {
		auto itemlist = std::make_shared<ItemListFormAction>(
				*this, itemlist_str, rsscache, filters, cfg, rxman);
		itemlist->set_feed(feed);
		apply_colors(itemlist);
		itemlist->set_parent_formaction(get_current_formaction());
		itemlist->init();
		formaction_stack.push_back(itemlist);
		current_formaction = formaction_stack_size() - 1;
		return itemlist;
	} else {
		status_line.show_error(_("Error: feed contains no items!"));
		return nullptr;
	}
}

void View::push_itemlist(unsigned int pos)
{
	std::shared_ptr<RssFeed> feed =
		ctrl.get_feedcontainer()->get_feed(pos);
	LOG(Level::DEBUG,
		"View::push_itemlist: retrieved feed at position %d",
		pos);
	auto itemlist = push_itemlist(feed);
	if (itemlist) {
		itemlist->set_pos(pos);
	}
}

void View::push_itemview(std::shared_ptr<RssFeed> f,
	const std::string& guid,
	const std::string& searchphrase)
{
	if (cfg->get_configvalue_as_filepath("pager") == "internal"_path) {
		auto fa = get_current_formaction();

		std::shared_ptr<ItemListFormAction> itemlist =
			std::dynamic_pointer_cast<ItemListFormAction,
			FormAction>(fa);
		assert(itemlist != nullptr);
		auto itemview = std::make_shared<ItemViewFormAction>(
				*this, itemlist, itemview_str, rsscache, cfg, rxman);
		itemview->set_feed(f);
		itemview->set_guid(guid);
		itemview->set_parent_formaction(fa);
		if (searchphrase.length() > 0) {
			itemview->set_highlightphrase(searchphrase);
		}
		apply_colors(itemview);
		itemview->init();
		formaction_stack.push_back(itemview);
		current_formaction = formaction_stack_size() - 1;
	} else {
		std::shared_ptr<RssItem> item = f->get_item_by_guid(guid);
		Filepath filename = ctrl.write_temporary_item(*item);
		open_in_pager(filename);
		try {
			bool old_unread = item->unread();
			item->set_unread(false);
			if (old_unread) {
				ctrl.mark_article_read(
					item->guid(), true);
			}
		} catch (const DbException& e) {
			status_line.show_error(strprintf::fmt(
					_("Error while marking article as read: %s"),
					e.what()));
		}
		::unlink(filename.to_locale_string().c_str());
	}
}

void View::view_dialogs()
{
	auto fa = get_current_formaction();
	if (fa != nullptr && fa->id() != "dialogs") {
		auto dialogs = std::make_shared<DialogsFormAction>(
				*this, dialogs_str, cfg, rxman);
		dialogs->set_parent_formaction(fa);
		apply_colors(dialogs);
		dialogs->init();
		formaction_stack.push_back(dialogs);
		current_formaction = formaction_stack_size() - 1;
	}
}

void View::push_empty_formaction()
{
	auto fa = get_current_formaction();

	auto empty_view = std::make_shared<EmptyFormAction>(
			*this, empty_str, cfg);
	empty_view->set_parent_formaction(fa);
	empty_view->init();
	formaction_stack.push_back(empty_view);
	current_formaction = formaction_stack_size() - 1;
}

void View::push_help()
{
	auto fa = get_current_formaction();

	auto helpview = std::make_shared<HelpFormAction>(
			*this, help_str, cfg, fa->id());
	apply_colors(helpview);
	helpview->set_parent_formaction(fa);
	helpview->init();
	formaction_stack.push_back(helpview);
	current_formaction = formaction_stack_size() - 1;
}

void View::push_urlview(const Links& links,
	std::shared_ptr<RssFeed>& feed)
{
	auto urlview = std::make_shared<UrlViewFormAction>(
			*this, feed, urlview_str, cfg);
	apply_colors(urlview);
	urlview->set_parent_formaction(get_current_formaction());
	urlview->init();
	urlview->set_links(links);
	formaction_stack.push_back(urlview);
	current_formaction = formaction_stack_size() - 1;
}

std::optional<Filepath> View::run_filebrowser(const Filepath& default_filename)
{
	auto filebrowser = std::make_shared<FileBrowserFormAction>(
			*this, filebrowser_str, cfg);
	apply_colors(filebrowser);
	filebrowser->set_default_filename(default_filename);
	filebrowser->set_parent_formaction(get_current_formaction());
	const std::string res = run_modal(filebrowser, "filenametext");
	if (res.empty()) {
		return std::nullopt;
	}
	return Filepath::from_locale_string(res);
}

std::optional<Filepath> View::run_dirbrowser()
{
	auto dirbrowser = std::make_shared<DirBrowserFormAction>(
			*this, filebrowser_str, cfg);
	apply_colors(dirbrowser);
	dirbrowser->set_parent_formaction(get_current_formaction());
	std::string res = run_modal(dirbrowser, "filenametext");
	if (res.empty()) {
		return std::nullopt;
	}
	return Filepath::from_locale_string(res);
}

std::string View::select_tag(const std::string& current_tag)
{
	if (tags.size() == 0) {
		status_line.show_error(_("No tags defined."));
		return "";
	}
	auto selecttag = std::make_shared<SelectFormAction>(
			*this, selecttag_str, cfg);
	selecttag->set_type(SelectFormAction::SelectionType::TAG);
	apply_colors(selecttag);
	selecttag->set_parent_formaction(get_current_formaction());
	selecttag->set_tags(tags);
	selecttag->set_selected_value(current_tag);
	run_modal(selecttag, "");
	return selecttag->get_selected_value();
}

std::string View::select_filter(const std::vector<FilterNameExprPair>& filters)
{
	auto selecttag = std::make_shared<SelectFormAction>(
			*this, selecttag_str, cfg);
	selecttag->set_type(SelectFormAction::SelectionType::FILTER);
	apply_colors(selecttag);
	selecttag->set_parent_formaction(get_current_formaction());
	selecttag->set_filters(filters);
	run_modal(selecttag, "");
	return selecttag->get_selected_value();
}

char View::confirm(const std::string& prompt, const std::string& charset)
{
	LOG(Level::DEBUG, "View::confirm: charset = %s", charset);

	std::shared_ptr<FormAction> f = get_current_formaction();
	// Push empty formaction so our status message is not overwritten on form `f`
	push_empty_formaction();
	f->set_status(prompt);

	char result = 0;

	do {
		const std::string event = f->draw_form_wait_for_event(0);
		LOG(Level::DEBUG, "View::confirm: event = %s", event);
		if (event.empty()) {
			continue;
		}
		if (event == "ESC" || event == "ENTER") {
			result = 0;
			LOG(Level::DEBUG,
				"View::confirm: user pressed ESC or ENTER, we "
				"cancel confirmation dialog");
			break;
		}
		result = keys->get_key(event);
		LOG(Level::DEBUG,
			"View::confirm: key = %c (%u)",
			result,
			result);
	} while (!result || strchr(charset.c_str(), result) == nullptr);

	f->set_status("");
	f->draw_form();

	pop_current_formaction();

	return result;
}

void View::notify_itemlist_change(std::shared_ptr<RssFeed> feed)
{
	for (const auto& form : formaction_stack) {
		if (form != nullptr && form->id() == "articlelist") {
			std::shared_ptr<ItemListFormAction> itemlist =
				std::dynamic_pointer_cast<ItemListFormAction,
				FormAction>(form);
			if (itemlist != nullptr) {
				std::shared_ptr<RssFeed> f =
					itemlist->get_feed();
				if (f != nullptr &&
					f->rssurl() == feed->rssurl()) {
					itemlist->set_feed(feed);
					itemlist->set_redraw(true);
					itemlist->invalidate_list();
				}
			}
		}
	}
}

bool View::get_random_unread(ItemListFormAction& itemlist,
	ItemViewFormAction* itemview)
{
	unsigned int feedpos;
	if (!cfg->get_configvalue_as_bool("goto-next-feed")) {
		return false;
	}
	if (feedlist_form->jump_to_random_unread_feed(feedpos)) {
		LOG(Level::DEBUG,
			"View::get_random_unread: found feed with unread "
			"articles");
		if (!try_prepare_query_feed(feedlist_form->get_feed())) {
			return false;
		}
		itemlist.set_feed(feedlist_form->get_feed());
		itemlist.set_pos(feedpos);
		itemlist.init();
		if (itemlist.jump_to_random_unread_item()) {
			if (itemview) {
				itemview->set_feed(itemlist.get_feed());
				itemview->set_guid(itemlist.get_guid());
				itemview->init();
			}
			return true;
		}
	}
	return false;
}

bool View::get_previous_unread(ItemListFormAction& itemlist,
	ItemViewFormAction* itemview)
{
	unsigned int feedpos;
	LOG(Level::DEBUG,
		"View::get_previous_unread: trying to find previous unread");
	if (itemlist.jump_to_previous_unread_item(false)) {
		LOG(Level::DEBUG,
			"View::get_previous_unread: found unread article in "
			"same "
			"feed");
		if (itemview) {
			itemview->set_feed(itemlist.get_feed());
			itemview->set_guid(itemlist.get_guid());
			itemview->init();
		}
		return true;
	} else if (cfg->get_configvalue_as_bool("goto-next-feed") == false) {
		LOG(Level::DEBUG,
			"View::get_previous_unread: goto-next-feed = false");
		status_line.show_error(_("No unread items."));
	} else if (feedlist_form->jump_to_previous_unread_feed(feedpos)) {
		LOG(Level::DEBUG,
			"View::get_previous_unread: found feed with unread "
			"articles");
		if (!try_prepare_query_feed(feedlist_form->get_feed())) {
			return false;
		}
		itemlist.set_feed(feedlist_form->get_feed());
		itemlist.set_pos(feedpos);
		itemlist.init();
		if (itemlist.jump_to_previous_unread_item(true)) {
			if (itemview) {
				itemview->set_feed(itemlist.get_feed());
				itemview->set_guid(itemlist.get_guid());
				itemview->init();
			}
			return true;
		}
	}
	return false;
}

bool View::get_next_unread_feed(ItemListFormAction& itemlist)
{
	unsigned int feedpos;
	if (feedlist_form->jump_to_next_unread_feed(feedpos)) {
		if (!try_prepare_query_feed(feedlist_form->get_feed())) {
			return false;
		}
		itemlist.set_feed(feedlist_form->get_feed());
		itemlist.set_pos(feedpos);
		itemlist.init();
		return true;
	}
	return false;
}

bool View::get_prev_unread_feed(ItemListFormAction& itemlist)
{
	unsigned int feedpos;
	if (feedlist_form->jump_to_previous_unread_feed(feedpos)) {
		if (!try_prepare_query_feed(feedlist_form->get_feed())) {
			return false;
		}
		itemlist.set_feed(feedlist_form->get_feed());
		itemlist.set_pos(feedpos);
		itemlist.init();
		return true;
	}
	return false;
}

bool View::get_next_unread(ItemListFormAction& itemlist,
	ItemViewFormAction* itemview)
{
	unsigned int feedpos;
	LOG(Level::DEBUG, "View::get_next_unread: trying to find next unread");
	if (itemlist.jump_to_next_unread_item(false)) {
		LOG(Level::DEBUG,
			"View::get_next_unread: found unread article in same "
			"feed");
		if (itemview) {
			itemview->set_feed(itemlist.get_feed());
			itemview->set_guid(itemlist.get_guid());
			itemview->init();
		}
		return true;
	} else if (cfg->get_configvalue_as_bool("goto-next-feed") == false) {
		LOG(Level::DEBUG,
			"View::get_next_unread: goto-next-feed = false");
		status_line.show_error(_("No unread items."));
	} else if (feedlist_form->jump_to_next_unread_feed(feedpos)) {
		LOG(Level::DEBUG,
			"View::get_next_unread: found feed with unread "
			"articles");
		if (!try_prepare_query_feed(feedlist_form->get_feed())) {
			return false;
		}
		itemlist.set_feed(feedlist_form->get_feed());
		itemlist.set_pos(feedpos);
		itemlist.init();
		if (itemlist.jump_to_next_unread_item(true)) {
			if (itemview) {
				itemview->set_feed(itemlist.get_feed());
				itemview->set_guid(itemlist.get_guid());
				itemview->init();
			}
			return true;
		}
	}
	return false;
}

bool View::get_previous(ItemListFormAction& itemlist,
	ItemViewFormAction* itemview)
{
	unsigned int feedpos;
	if (itemlist.jump_to_previous_item(false)) {
		LOG(Level::DEBUG, "View::get_previous: article in same feed");
		if (itemview) {
			itemview->set_feed(itemlist.get_feed());
			itemview->set_guid(itemlist.get_guid());
			itemview->init();
		}
		return true;
	} else if (cfg->get_configvalue_as_bool("goto-next-feed") == false) {
		LOG(Level::DEBUG, "View::get_previous: goto-next-feed = false");
		status_line.show_error(_("Already on first item."));
	} else if (feedlist_form->jump_to_previous_feed(feedpos)) {
		LOG(Level::DEBUG, "View::get_previous: previous feed");
		if (!try_prepare_query_feed(feedlist_form->get_feed())) {
			return false;
		}
		itemlist.set_feed(feedlist_form->get_feed());
		itemlist.set_pos(feedpos);
		itemlist.init();
		if (itemlist.jump_to_previous_item(true)) {
			if (itemview) {
				itemview->set_feed(itemlist.get_feed());
				itemview->set_guid(itemlist.get_guid());
				itemview->init();
			}
			return true;
		}
	}
	return false;
}

bool View::get_next(ItemListFormAction& itemlist, ItemViewFormAction* itemview)
{
	unsigned int feedpos;
	if (itemlist.jump_to_next_item(false)) {
		LOG(Level::DEBUG, "View::get_next: article in same feed");
		if (itemview) {
			itemview->set_feed(itemlist.get_feed());
			itemview->set_guid(itemlist.get_guid());
			itemview->init();
		}
		return true;
	} else if (cfg->get_configvalue_as_bool("goto-next-feed") == false) {
		LOG(Level::DEBUG, "View::get_next: goto-next-feed = false");
		status_line.show_error(_("Already on last item."));
	} else if (feedlist_form->jump_to_next_feed(feedpos)) {
		LOG(Level::DEBUG, "View::get_next: next feed");
		if (!try_prepare_query_feed(feedlist_form->get_feed())) {
			return false;
		}
		itemlist.set_feed(feedlist_form->get_feed());
		itemlist.set_pos(feedpos);
		itemlist.init();
		if (itemlist.jump_to_next_item(true)) {
			if (itemview) {
				itemview->set_feed(itemlist.get_feed());
				itemview->set_guid(itemlist.get_guid());
				itemview->init();
			}
			return true;
		}
	}
	return false;
}

bool View::get_next_feed(ItemListFormAction& itemlist)
{
	unsigned int feedpos;
	if (feedlist_form->jump_to_next_feed(feedpos)) {
		if (!try_prepare_query_feed(feedlist_form->get_feed())) {
			return false;
		}
		itemlist.set_feed(feedlist_form->get_feed());
		itemlist.set_pos(feedpos);
		itemlist.init();
		return true;
	}
	return false;
}

bool View::get_prev_feed(ItemListFormAction& itemlist)
{
	unsigned int feedpos;
	if (feedlist_form->jump_to_previous_feed(feedpos)) {
		if (!try_prepare_query_feed(feedlist_form->get_feed())) {
			return false;
		}
		itemlist.set_feed(feedlist_form->get_feed());
		itemlist.set_pos(feedpos);
		itemlist.init();
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

		const std::shared_ptr<AutoDiscardMessage> message =
			status_line.show_message_until_finished(_("Updating query feed..."));
		feed->update_items(ctrl.get_feedcontainer()->get_all_feeds());
		feed->sort(cfg->get_article_sort_strategy());
		notify_itemlist_change(feed);
	}
}

bool View::try_prepare_query_feed(std::shared_ptr<RssFeed> feed)
{
	try {
		prepare_query_feed(feed);
		return true;
	} catch (const MatcherException& e) {
		const auto msg = strprintf::fmt(_("Error: couldn't prepare query feed: %s"), e.what());
		status_line.show_error(msg);
		return false;
	}
}

void View::force_redraw()
{
	std::shared_ptr<FormAction> fa = get_current_formaction();
	if (fa != nullptr
		&& std::dynamic_pointer_cast<EmptyFormAction>(fa) == nullptr) {
		fa->set_redraw(true);
		fa->prepare();
		fa->draw_form();
	}
}

void View::pop_current_formaction()
{
	std::shared_ptr<FormAction> f = get_current_formaction();
	auto it = formaction_stack.begin();
	for (unsigned int i = 0; i < current_formaction; i++) {
		++it;
	}
	formaction_stack.erase(it);
	if (f == nullptr) {
		// TODO this is not correct... we'd need to return to the previous one, but nullptr formactions have no parent
		current_formaction = formaction_stack_size() -
			1;
	} else if (formaction_stack.size() > 0) {
		// first, we set back the parent formactions of those who
		// reference the formaction we just removed
		for (const auto& form : formaction_stack) {
			if (form->get_parent_formaction() == f) {
				form->set_parent_formaction(
					formaction_stack[0]);
			}
		}
		// we set the new formaction based on the removed formaction's
		// parent.
		unsigned int i = 0;
		for (const auto& form : formaction_stack) {
			if (form == f->get_parent_formaction()) {
				current_formaction = i;
				break;
			}
			i++;
		}

		// Skip cleanup steps when returning from a transient EmptyFormAction
		if (std::dynamic_pointer_cast<EmptyFormAction>(f) == nullptr) {
			std::shared_ptr<FormAction> fa = get_current_formaction();
			if (fa) {
				fa->set_redraw(true);
				f->set_status("");
				fa->recalculate_widget_dimensions();
			}
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
	std::shared_ptr<FormAction> f = formaction_stack[pos];
	auto it = formaction_stack.begin();
	for (unsigned int i = 0; i < pos; i++) {
		++it;
	}
	formaction_stack.erase(it);
	current_formaction--;
	if (f != nullptr && formaction_stack.size() > 0) {
		// we set back the parent formactions of those who reference the
		// formaction we just removed
		for (const auto& form : formaction_stack) {
			if (form->get_parent_formaction() == f) {
				form->set_parent_formaction(
					formaction_stack[0]);
			}
		}
	}
}

void View::apply_colors_to_all_formactions()
{
	for (const auto& form : formaction_stack) {
		apply_colors(form);
	}
	if (formaction_stack.size() > 0 &&
		formaction_stack[current_formaction]) {
		formaction_stack[current_formaction]->set_redraw(true);
	}
}

void View::apply_colors(std::shared_ptr<FormAction> fa)
{
	LOG(Level::DEBUG, "View::apply_colors: fa = %s", fa->id());

	const auto stfl_value_setter = [&](const std::string& name,
	const std::string& value) {
		fa->set_value(name, value);
	};
	colorman.apply_colors(stfl_value_setter);
}

void View::feedlist_mark_pos_if_visible(unsigned int pos)
{
	if (feedlist_form != nullptr) {
		feedlist_form->mark_pos_if_visible(pos);
	}
}

void View::set_cache(Cache* c)
{
	rsscache = c;
}

std::vector<std::pair<unsigned int, std::string>> View::get_formaction_names()
{
	std::vector<std::pair<unsigned int, std::string>> formaction_names;
	unsigned int i = 0;
	for (const auto& form : formaction_stack) {
		if (form && form->id() != "dialogs") {
			formaction_names.push_back(
				std::pair<unsigned int, std::string>(
					i, form->title()));
		}
		i++;
	}
	return formaction_names;
}

void View::goto_next_dialog()
{
	current_formaction++;
	if (current_formaction >= formaction_stack.size()) {
		current_formaction = 0;
	}
}

void View::goto_prev_dialog()
{
	if (current_formaction > 0) {
		current_formaction--;
	} else {
		current_formaction = formaction_stack.size() - 1;
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

bool View::handle_qna_event(const std::string& event,
	std::shared_ptr<FormAction> fa)
{
	if (is_inside_qna) {
		LOG(Level::DEBUG, "View::handle_qna_event: we're inside QNA input");
		fa->handle_qna_event(event, is_inside_cmdline);

		return true;
	}
	return false;
}

void View::handle_resize()
{
	for (const auto& form : formaction_stack) {
		if (form != nullptr) {
			// Recalculate width and height of stfl widgets
			form->recalculate_widget_dimensions();
			form->set_redraw(true);
		}
	}
}

void View::ctrl_c_action(int /* sig */)
{
	LOG(Level::DEBUG, "caught SIGINT");
	ctrl_c_hit = true;
}

} // namespace newsboat

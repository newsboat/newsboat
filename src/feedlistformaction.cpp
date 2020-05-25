#include "feedlistformaction.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <langinfo.h>
#include <sstream>
#include <string>

#include "config.h"
#include "dbexception.h"
#include "feedcontainer.h"
#include "fmtstrformatter.h"
#include "listformatter.h"
#include "logger.h"
#include "reloader.h"
#include "rssfeed.h"
#include "scopemeasure.h"
#include "strprintf.h"
#include "utils.h"
#include "view.h"

namespace newsboat {

FeedListFormAction::FeedListFormAction(View* vv,
	std::string formstr,
	Cache* cc,
	FilterContainer* f,
	ConfigContainer* cfg,
	RegexManager& r)
	: ListFormAction(vv, formstr, cfg)
	, zero_feedpos(false)
	, feeds_shown(0)
	, quit(false)
	, apply_filter(false)
	, search_dummy_feed(new RssFeed(cc))
	, filterpos(0)
	, set_filterpos(false)
	, rxman(r)
	, old_width(0)
	, unread_feeds(0)
	, total_feeds(0)
	, filters(f)
	, feeds_list("feeds", FormAction::f)
{
	valid_cmds.push_back("tag");
	valid_cmds.push_back("goto");
	std::sort(valid_cmds.begin(), valid_cmds.end());
	old_sort_order = cfg->get_configvalue("feed-sort-order");
	search_dummy_feed->set_search_feed(true);
	register_format_styles();
}

void FeedListFormAction::init()
{
	set_keymap_hints();

	f.run(-3); // compute all widget dimensions

	if (v->get_ctrl()->get_refresh_on_start()) {
		v->get_ctrl()->get_reloader()->start_reload_all_thread();
	}
	v->get_ctrl()->update_feedlist();

	/*
	 * This is kind of a hack.
	 * The FeedListFormAction is responsible for starting up the
	 * ReloadThread, which is responsible for regularly spawning
	 * DownloadThreads.
	 */
	v->get_ctrl()->get_reloader()->spawn_reloadthread();
}

FeedListFormAction::~FeedListFormAction() {}

void FeedListFormAction::prepare()
{
	unsigned int width = utils::to_u(f.get("items:w"));

	if (old_width != width) {
		do_redraw = true;
		old_width = width;
		LOG(Level::DEBUG,
			"FeedListFormAction::prepare: apparent resize");
	}

	std::string sort_order = cfg->get_configvalue("feed-sort-order");
	if (sort_order != old_sort_order) {
		v->get_ctrl()->get_feedcontainer()->sort_feeds(
			cfg->get_feed_sort_strategy());
		old_sort_order = sort_order;
		do_redraw = true;
	}

	if (do_redraw) {
		LOG(Level::DEBUG, "FeedListFormAction::prepare: doing redraw");
		v->get_ctrl()->update_feedlist();
		set_pos();
		do_redraw = false;
	}
}

bool FeedListFormAction::process_operation(Operation op,
	bool automatic,
	std::vector<std::string>* args)
{
	std::string feedpos = f.get("feedposname");
	unsigned int pos = utils::to_u(feedpos);
REDO:
	switch (op) {
	case OP_OPEN: {
		if (f.get_focus() == "feeds") {
			if (automatic && args->size() > 0) {
				pos = utils::to_u((*args)[0]);
			}
			LOG(Level::INFO,
				"FeedListFormAction: opening feed at position "
				"`%s'",
				feedpos);
			if (feeds_shown > 0 && feedpos.length() > 0) {
				v->push_itemlist(pos);
			} else {
				// should not happen
				v->show_error(_("No feed selected!"));
			}
		}
	}
	break;
	case OP_RELOAD: {
		LOG(Level::INFO,
			"FeedListFormAction: reloading feed at position `%s'",
			feedpos);
		if (feeds_shown > 0 && feedpos.length() > 0) {
			v->get_ctrl()->get_reloader()->reload(pos);
		} else {
			v->show_error(
				_("No feed selected!")); // should not happen
		}
	}
	break;
	case OP_INT_RESIZE:
		do_redraw = true;
		break;
	case OP_RELOADURLS:
		v->get_ctrl()->reload_urls_file();
		break;
	case OP_SK_UP:
		feeds_list.move_up(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_SK_DOWN:
		feeds_list.move_down(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_SK_HOME:
		feeds_list.move_to_first();
		break;
	case OP_SK_END:
		feeds_list.move_to_last();
		break;
	case OP_SK_PGUP:
		feeds_list.move_page_up(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_SK_PGDOWN:
		feeds_list.move_page_down(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_SORT: {
		/// This string is related to the letters in parentheses in the
		/// "Sort by (f)irsttag/..." and "Reverse Sort by
		/// (f)irsttag/..." messages
		std::string input_options = _("ftauln");
		char c = v->confirm(
				_("Sort by "
					"(f)irsttag/(t)itle/(a)rticlecount/"
					"(u)nreadarticlecount/(l)astupdated/(n)one?"),
				input_options);
		if (!c) {
			break;
		}

		// Check that the number of translated answers is the same as the
		// number of answers we expect to handle. If it doesn't, just give up.
		// That'll prevent this function from sorting anything, so users will
		// complain, and we'll ask them to update the translation. A bit lame,
		// but it's better than mishandling the answer.
		const auto n_options = ((std::string) "ftaun").length();
		if (input_options.length() < n_options) {
			break;
		}

		if (c == input_options.at(0)) {
			cfg->set_configvalue(
				"feed-sort-order", "firsttag-desc");
		} else if (c == input_options.at(1)) {
			cfg->set_configvalue("feed-sort-order", "title-desc");
		} else if (c == input_options.at(2)) {
			cfg->set_configvalue(
				"feed-sort-order", "articlecount-desc");
		} else if (c == input_options.at(3)) {
			cfg->set_configvalue(
				"feed-sort-order", "unreadarticlecount-desc");
		} else if (c == input_options.at(4)) {
			cfg->set_configvalue(
				"feed-sort-order", "lastupdated-desc");
		} else if (c == input_options.at(5)) {
			cfg->set_configvalue("feed-sort-order", "none-desc");
		}
	}
	break;
	case OP_REVSORT: {
		std::string input_options = _("ftauln");
		char c = v->confirm(
				_("Reverse Sort by "
					"(f)irsttag/(t)itle/(a)rticlecount/"
					"(u)nreadarticlecount/(l)astupdated/(n)one?"),
				input_options);
		if (!c) {
			break;
		}

		// Check that the number of translated answers is the same as the
		// number of answers we expect to handle. If it doesn't, just give up.
		// That'll prevent this function from sorting anything, so users will
		// complain, and we'll ask them to update the translation. A bit lame,
		// but it's better than mishandling the answer.
		const auto n_options = ((std::string) "ftaun").length();
		if (input_options.length() < n_options) {
			break;
		}

		if (c == input_options.at(0)) {
			cfg->set_configvalue("feed-sort-order", "firsttag-asc");
		} else if (c == input_options.at(1)) {
			cfg->set_configvalue("feed-sort-order", "title-asc");
		} else if (c == input_options.at(2)) {
			cfg->set_configvalue(
				"feed-sort-order", "articlecount-asc");
		} else if (c == input_options.at(3)) {
			cfg->set_configvalue(
				"feed-sort-order", "unreadarticlecount-asc");
		} else if (c == input_options.at(4)) {
			cfg->set_configvalue(
				"feed-sort-order", "lastupdated-asc");
		} else if (c == input_options.at(5)) {
			cfg->set_configvalue("feed-sort-order", "none-asc");
		}
	}
	break;
	case OP_OPENINBROWSER:
		if (feeds_shown > 0 && feedpos.length() > 0) {
			std::shared_ptr<RssFeed> feed =
				v->get_ctrl()->get_feedcontainer()->get_feed(
					pos);
			if (feed) {
				if (!feed->is_query_feed()) {
					LOG(Level::INFO,
						"FeedListFormAction: opening "
						"feed "
						"at position `%s': %s",
						feedpos,
						feed->link());
					if (!feed->link().empty()) {
						if (int err = v->open_in_browser(feed->link())) {
							v->show_error(strprintf::fmt(_("Browser returned error code %i"), err));
							return false;
						}
					} else if (!feed->rssurl().empty()) {
						if (int err = v->open_in_browser(feed->rssurl())) {
							v->show_error(strprintf::fmt(_("Browser returned error code %i"), err));
							return false;
						}

					} else {
						// rssurl can't be empty, so if we got to this branch,
						// something is clearly wrong with Newsboat internals.
						// That's why we write a message to the log, and not
						// just display it to the user.
						LOG(Level::INFO,
							"FeedListFormAction: cannot open feed in browser "
							"because both `link' and `rssurl' fields are "
							"empty");
					}
				} else {
					v->show_error(
						_("Cannot open query feeds in "
							"the browser!"));
				}
			}
		} else {
			v->show_error(_("No feed selected!"));
		}
		break;
	case OP_OPENALLUNREADINBROWSER:
		if (feeds_shown > 0 && feedpos.length() > 0) {
			std::shared_ptr<RssFeed> feed =
				v->get_ctrl()->get_feedcontainer()->get_feed(
					pos);
			if (feed) {
				LOG(Level::INFO,
					"FeedListFormAction: opening all "
					"unread "
					"items in feed at position `%s'",
					feedpos.c_str());
				if (int err = open_unread_items_in_browser(feed, false)) {
					v->show_error(strprintf::fmt(_("Browser returned error code %i"), err));
					return false;
				}

			}
		} else {
			v->show_error(_("No feed selected!"));
		}
		break;
	case OP_OPENALLUNREADINBROWSER_AND_MARK:
		if (feeds_shown > 0 && feedpos.length() > 0) {
			std::shared_ptr<RssFeed> feed =
				v->get_ctrl()->get_feedcontainer()->get_feed(
					pos);
			if (feed) {
				LOG(Level::INFO,
					"FeedListFormAction: opening all "
					"unread "
					"items in feed at position `%s' and "
					"marking read",
					feedpos.c_str());

				if (int err = open_unread_items_in_browser(feed, true)) {
					v->show_error(strprintf::fmt(_("Browser returned error code %i"), err));
					return false;
				}

				do_redraw = true;
			}
		}
		break;
	case OP_RELOADALL:
		LOG(Level::INFO, "FeedListFormAction: reloading all feeds");
		{
			bool reload_only_visible_feeds =
				cfg->get_configvalue_as_bool(
					"reload-only-visible-feeds");
			std::vector<int> idxs;
			for (const auto& feed : visible_feeds) {
				idxs.push_back(feed.second);
			}
			v->get_ctrl()->get_reloader()->start_reload_all_thread(
				reload_only_visible_feeds ? &idxs : nullptr);
		}
		break;
	case OP_MARKFEEDREAD: {
		LOG(Level::INFO,
			"FeedListFormAction: marking feed read at position "
			"`%s'",
			feedpos);
		if (feeds_shown > 0 && feedpos.length() > 0) {
			v->set_status(_("Marking feed read..."));
			try {
				v->get_ctrl()->mark_all_read(pos);
				do_redraw = true;
				v->set_status("");
				bool show_read = cfg->get_configvalue_as_bool("show-read-feeds");
				if (feeds_shown > (pos + 1) && show_read) {
					f.set("feeds_pos",
						std::to_string(pos + 1));
				}
			} catch (const DbException& e) {
				v->show_error(strprintf::fmt(
						_("Error: couldn't mark feed read: %s"),
						e.what()));
			}
		} else {
			v->show_error(
				_("No feed selected!")); // should not happen
		}
	}
	break;
	case OP_TOGGLESHOWREAD:
		LOG(Level::INFO,
			"FeedListFormAction: toggling show-read-feeds");
		if (cfg->get_configvalue_as_bool("show-read-feeds")) {
			cfg->set_configvalue("show-read-feeds", "no");
		} else {
			cfg->set_configvalue("show-read-feeds", "yes");
		}
		save_filterpos();
		do_redraw = true;
		break;
	case OP_NEXTUNREAD: {
		unsigned int local_tmp;
		LOG(Level::INFO,
			"FeedListFormAction: jumping to next unread feed");
		if (!jump_to_next_unread_feed(local_tmp)) {
			v->show_error(_("No feeds with unread items."));
		}
	}
	break;
	case OP_PREVUNREAD: {
		unsigned int local_tmp;
		LOG(Level::INFO,
			"FeedListFormAction: jumping to previous unread feed");
		if (!jump_to_previous_unread_feed(local_tmp)) {
			v->show_error(_("No feeds with unread items."));
		}
	}
	break;
	case OP_NEXT: {
		unsigned int local_tmp;
		LOG(Level::INFO, "FeedListFormAction: jumping to next feed");
		if (!jump_to_next_feed(local_tmp)) {
			v->show_error(_("Already on last feed."));
		}
	}
	break;
	case OP_PREV: {
		unsigned int local_tmp;
		LOG(Level::INFO,
			"FeedListFormAction: jumping to previous feed");
		if (!jump_to_previous_feed(local_tmp)) {
			v->show_error(_("Already on first feed."));
		}
	}
	break;
	case OP_RANDOMUNREAD: {
		unsigned int local_tmp;
		LOG(Level::INFO,
			"FeedListFormAction: jumping to random unread feed");
		if (!jump_to_random_unread_feed(local_tmp)) {
			v->show_error(_("No feeds with unread items."));
		}
	}
	break;
	case OP_MARKALLFEEDSREAD:
		LOG(Level::INFO, "FeedListFormAction: marking all feeds read");
		v->set_status(_("Marking all feeds read..."));
		if (tag == "") {
			v->get_ctrl()->mark_all_read("");
		} else {
			// we're in tag view, so let's only touch feeds that are
			// visible
			for (const auto& feedptr_pos_pair : visible_feeds) {
				auto rss_feed_ptr = feedptr_pos_pair.first;
				auto feedurl = rss_feed_ptr->rssurl();
				v->get_ctrl()->mark_all_read(feedurl);
			}
		}
		v->set_status("");
		do_redraw = true;
		break;
	case OP_CLEARTAG:
		tag = "";
		do_redraw = true;
		zero_feedpos = true;
		break;
	case OP_SETTAG: {
		std::string newtag;
		if (automatic && args->size() > 0) {
			newtag = (*args)[0];
		} else {
			newtag = v->select_tag();
		}
		if (newtag != "") {
			tag = newtag;
			do_redraw = true;
			zero_feedpos = true;
		}
	}
	break;
	case OP_SELECTFILTER:
		if (filters->size() > 0) {
			std::string newfilter;
			if (automatic && args->size() > 0) {
				newfilter = (*args)[0];
			} else {
				newfilter = v->select_filter(
						filters->get_filters());
			}
			if (newfilter != "") {
				filterhistory.add_line(newfilter);
				if (newfilter.length() > 0) {
					if (!matcher.parse(newfilter)) {
						v->show_error(strprintf::fmt(
								_("Error: couldn't "
									"parse filter "
									"command `%s': %s"),
								newfilter,
								matcher.get_parse_error()));
					} else {
						save_filterpos();
						apply_filter = true;
						do_redraw = true;
					}
				}
			}
		} else {
			v->show_error(_("No filters defined."));
		}
		break;
	case OP_SEARCH:
		if (automatic && args->size() > 0) {
			qna_responses.clear();
			// when in automatic mode, we manually fill the
			// qna_responses vector from the arguments and then run
			// the finished_qna() by ourselves to simulate a "Q&A"
			// session that is in fact macro-driven.
			qna_responses.push_back((*args)[0]);
			finished_qna(OP_INT_START_SEARCH);
		} else {
			std::vector<QnaPair> qna;
			qna.push_back(QnaPair(_("Search for: "), ""));
			this->start_qna(
				qna, OP_INT_START_SEARCH, &searchhistory);
		}
		break;
	case OP_CLEARFILTER:
		apply_filter = false;
		do_redraw = true;
		save_filterpos();
		break;
	case OP_SETFILTER:
		if (automatic && args->size() > 0) {
			qna_responses.clear();
			qna_responses.push_back((*args)[0]);
			finished_qna(OP_INT_END_SETFILTER);
		} else {
			std::vector<QnaPair> qna;
			qna.push_back(QnaPair(_("Filter: "), ""));
			this->start_qna(
				qna, OP_INT_END_SETFILTER, &filterhistory);
		}
		break;
	case OP_EDIT_URLS:
		v->get_ctrl()->edit_urls_file();
		break;
	case OP_QUIT:
		if (tag != "") {
			op = OP_CLEARTAG;
			goto REDO;
		}
		LOG(Level::INFO, "FeedListFormAction: quitting");
		if (automatic ||
			!cfg->get_configvalue_as_bool("confirm-exit") ||
			v->confirm(
				_("Do you really want to quit (y:Yes n:No)? "),
				_("yn")) == *_("y")) {
			quit = true;
		}
		break;
	case OP_HARDQUIT:
		LOG(Level::INFO, "FeedListFormAction: hard quitting");
		quit = true;
		break;
	case OP_HELP:
		v->push_help();
		break;
	default:
		ListFormAction::process_operation(op, automatic, args);
		break;
	}
	if (quit) {
		while (v->formaction_stack_size() > 0) {
			v->pop_current_formaction();
		}
	}
	return true;
}

void FeedListFormAction::update_visible_feeds(
	std::vector<std::shared_ptr<RssFeed>>& feeds)
{
	assert(cfg != nullptr); // must not happen

	visible_feeds.clear();

	bool show_read = cfg->get_configvalue_as_bool("show-read-feeds");

	unsigned int i = 0;
	for (const auto& feed : feeds) {
		feed->set_index(i + 1);
		if ((tag == "" || feed->matches_tag(tag)) &&
			(show_read || feed->unread_item_count() > 0) &&
			(!apply_filter || matcher.matches(feed.get())) &&
			!feed->hidden()) {
			visible_feeds.push_back(FeedPtrPosPair(feed, i));
		}
		i++;
	}

	feeds_shown = visible_feeds.size();
}

void FeedListFormAction::set_feedlist(
	std::vector<std::shared_ptr<RssFeed>>& feeds)
{
	assert(cfg != nullptr); // must not happen

	unsigned int width = utils::to_u(f.get("feeds:w"));

	unread_feeds = 0;

	std::string feedlist_format = cfg->get_configvalue("feedlist-format");

	ListFormatter listfmt(&rxman, "feedlist");

	update_visible_feeds(feeds);

	for (const auto& feed : visible_feeds) {
		if (feed.first->unread_item_count() > 0) {
			++unread_feeds;
		}

		listfmt.add_line(format_line(feedlist_format,
				feed.first,
				feed.second,
				width),
			std::to_string(feed.second));
	}

	unsigned int total_unread = 0;
	for (const auto& feed : feeds) {
		if (feed->unread_item_count() > 0) {
			total_unread++;
		}
	}

	total_feeds = visible_feeds.size();

	feeds_list.stfl_replace_lines(listfmt);

	std::string title_format =
		cfg->get_configvalue("feedlist-title-format");

	FmtStrFormatter fmt;
	fmt.register_fmt('T', tag);
	fmt.register_fmt('N', PROGRAM_NAME);
	fmt.register_fmt('V', utils::program_version());
	fmt.register_fmt('u', std::to_string(unread_feeds));
	fmt.register_fmt('t', std::to_string(visible_feeds.size()));
	fmt.register_fmt('U', std::to_string(total_unread));
	fmt.register_fmt('a', std::to_string(feeds.size()));

	f.set("head", fmt.do_format(title_format, width));
}

void FeedListFormAction::set_tags(const std::vector<std::string>& t)
{
	tags = t;
}

KeyMapHintEntry* FeedListFormAction::get_keymap_hint()
{
	static KeyMapHintEntry hints[] = {{OP_QUIT, _("Quit")},
		{OP_OPEN, _("Open")},
		{OP_NEXTUNREAD, _("Next Unread")},
		{OP_RELOAD, _("Reload")},
		{OP_RELOADALL, _("Reload All")},
		{OP_MARKFEEDREAD, _("Mark Read")},
		{OP_MARKALLFEEDSREAD, _("Mark All Read")},
		{OP_SEARCH, _("Search")},
		{OP_HELP, _("Help")},
		{OP_NIL, nullptr}
	};
	return hints;
}

bool FeedListFormAction::jump_to_previous_unread_feed(unsigned int& feedpos)
{
	unsigned int curpos = utils::to_u(f.get("feeds_pos"));
	LOG(Level::DEBUG,
		"FeedListFormAction::jump_to_previous_unread_feed: searching "
		"for "
		"unread feed");

	for (int i = curpos - 1; i >= 0; --i) {
		LOG(Level::DEBUG,
			"FeedListFormAction::jump_to_previous_unread_feed: "
			"visible_feeds[%u] unread items: %u",
			i,
			visible_feeds[i].first->unread_item_count());
		if (visible_feeds[i].first->unread_item_count() > 0) {
			LOG(Level::DEBUG,
				"FeedListFormAction::jump_to_previous_unread_"
				"feed:"
				" hit");
			f.set("feeds_pos", std::to_string(i));
			feedpos = visible_feeds[i].second;
			return true;
		}
	}
	for (int i = visible_feeds.size() - 1; i >= static_cast<int>(curpos);
		--i) {
		LOG(Level::DEBUG,
			"FeedListFormAction::jump_to_previous_unread_feed: "
			"visible_feeds[%u] unread items: %u",
			i,
			visible_feeds[i].first->unread_item_count());
		if (visible_feeds[i].first->unread_item_count() > 0) {
			LOG(Level::DEBUG,
				"FeedListFormAction::jump_to_previous_unread_"
				"feed:"
				" hit");
			f.set("feeds_pos", std::to_string(i));
			feedpos = visible_feeds[i].second;
			return true;
		}
	}
	return false;
}

void FeedListFormAction::goto_feed(const std::string& str)
{
	unsigned int curpos = utils::to_u(f.get("feeds_pos"));
	LOG(Level::DEBUG,
		"FeedListFormAction::goto_feed: curpos = %u str = `%s'",
		curpos,
		str);
	for (unsigned int i = curpos + 1; i < visible_feeds.size(); ++i) {
		if (strcasestr(visible_feeds[i].first->title().c_str(),
				str.c_str()) != nullptr) {
			f.set("feeds_pos", std::to_string(i));
			return;
		}
	}
	for (unsigned int i = 0; i <= curpos; ++i) {
		if (strcasestr(visible_feeds[i].first->title().c_str(),
				str.c_str()) != nullptr) {
			f.set("feeds_pos", std::to_string(i));
			return;
		}
	}
}

bool FeedListFormAction::jump_to_random_unread_feed(unsigned int& feedpos)
{
	bool unread_feeds_available = false;
	for (unsigned int i = 0; i < visible_feeds.size(); ++i) {
		if (visible_feeds[i].first->unread_item_count() > 0) {
			unread_feeds_available = true;
			break;
		}
	}
	if (unread_feeds_available) {
		for (;;) {
			unsigned int pos =
				utils::get_random_value(visible_feeds.size());
			if (visible_feeds[pos].first->unread_item_count() > 0) {
				f.set("feeds_pos", std::to_string(pos));
				feedpos = visible_feeds[pos].second;
				break;
			}
		}
	}
	return unread_feeds_available;
}

bool FeedListFormAction::jump_to_next_unread_feed(unsigned int& feedpos)
{
	unsigned int curpos = utils::to_u(f.get("feeds_pos"));
	LOG(Level::DEBUG,
		"FeedListFormAction::jump_to_next_unread_feed: searching for "
		"unread feed");

	for (unsigned int i = curpos + 1; i < visible_feeds.size(); ++i) {
		LOG(Level::DEBUG,
			"FeedListFormAction::jump_to_next_unread_feed: "
			"visible_feeds[%u] unread items: %u",
			i,
			visible_feeds[i].first->unread_item_count());
		if (visible_feeds[i].first->unread_item_count() > 0) {
			LOG(Level::DEBUG,
				"FeedListFormAction::jump_to_next_unread_feed:"
				" "
				"hit");
			f.set("feeds_pos", std::to_string(i));
			feedpos = visible_feeds[i].second;
			return true;
		}
	}
	for (unsigned int i = 0; i <= curpos; ++i) {
		LOG(Level::DEBUG,
			"FeedListFormAction::jump_to_next_unread_feed: "
			"visible_feeds[%u] unread items: %u",
			i,
			visible_feeds[i].first->unread_item_count());
		if (visible_feeds[i].first->unread_item_count() > 0) {
			LOG(Level::DEBUG,
				"FeedListFormAction::jump_to_next_unread_feed:"
				" "
				"hit");
			f.set("feeds_pos", std::to_string(i));
			feedpos = visible_feeds[i].second;
			return true;
		}
	}
	return false;
}

bool FeedListFormAction::jump_to_previous_feed(unsigned int& feedpos)
{
	unsigned int curpos = utils::to_u(f.get("feeds_pos"));

	if (curpos > 0) {
		unsigned int i = curpos - 1;
		LOG(Level::DEBUG,
			"FeedListFormAction::jump_to_previous_feed: "
			"visible_feeds[%u]",
			i);
		f.set("feeds_pos", std::to_string(i));
		feedpos = visible_feeds[i].second;
		return true;
	}
	return false;
}

bool FeedListFormAction::jump_to_next_feed(unsigned int& feedpos)
{
	unsigned int curpos = utils::to_u(f.get("feeds_pos"));

	if ((curpos + 1) < visible_feeds.size()) {
		unsigned int i = curpos + 1;
		LOG(Level::DEBUG,
			"FeedListFormAction::jump_to_next_feed: "
			"visible_feeds[%u]",
			i);
		f.set("feeds_pos", std::to_string(i));
		feedpos = visible_feeds[i].second;
		return true;
	}
	return false;
}

std::shared_ptr<RssFeed> FeedListFormAction::get_feed()
{
	unsigned int curpos = utils::to_u(f.get("feeds_pos"));
	return visible_feeds[curpos].first;
}

int FeedListFormAction::get_pos(unsigned int realidx)
{
	for (unsigned int i = 0; i < visible_feeds.size(); ++i) {
		if (visible_feeds[i].second == realidx) {
			return i;
		}
	}
	return -1;
}

void FeedListFormAction::handle_cmdline(const std::string& cmd)
{
	unsigned int idx = 0;
	/*
	 * this handle_cmdline is a bit different than the other ones.
	 * Since we want to use ":30" to jump to the 30th entry, we first
	 * need to check whether the command parses as unsigned integer,
	 * and if so, jump to the entered entry. Otherwise, we try to
	 * handle it as a normal command.
	 */
	if (1 == sscanf(cmd.c_str(), "%u", &idx)) {
		handle_cmdline_num(idx);
	} else {
		// hand over all other commands to formaction
		std::vector<std::string> tokens =
			utils::tokenize_quoted(cmd, " \t");
		if (!tokens.empty()) {
			if (tokens[0] == "tag") {
				if (tokens.size() >= 2 && tokens[1] != "") {
					tag = tokens[1];
					do_redraw = true;
					zero_feedpos = true;
				}
			} else if (tokens[0] == "goto") {
				if (tokens.size() >= 2 && tokens[1] != "") {
					goto_feed(tokens[1]);
				}
			} else {
				FormAction::handle_cmdline(cmd);
			}
		}
	}
}

void FeedListFormAction::finished_qna(Operation op)
{
	FormAction::finished_qna(op); // important!

	switch (op) {
	case OP_INT_END_SETFILTER:
		op_end_setfilter();
		break;
	case OP_INT_START_SEARCH:
		op_start_search();
		break;
	default:
		break;
	}
}

void FeedListFormAction::mark_pos_if_visible(unsigned int pos)
{
	ScopeMeasure m1("FeedListFormAction::mark_pos_if_visible");
	unsigned int vpos = 0;
	v->get_ctrl()->update_visible_feeds();
	for (const auto& feed : visible_feeds) {
		if (feed.second == pos) {
			LOG(Level::DEBUG,
				"FeedListFormAction::mark_pos_if_visible: "
				"match, "
				"setting position to %u",
				vpos);
			f.set("feeds_pos", std::to_string(vpos));
			return;
		}
		vpos++;
	}
	vpos = 0;
	pos = v->get_ctrl()->get_feedcontainer()->get_pos_of_next_unread(pos);
	for (const auto& feed : visible_feeds) {
		if (feed.second == pos) {
			LOG(Level::DEBUG,
				"FeedListFormAction::mark_pos_if_visible: "
				"match "
				"in 2nd try, setting position to %u",
				vpos);
			f.set("feeds_pos", std::to_string(vpos));
			return;
		}
		vpos++;
	}
}

void FeedListFormAction::save_filterpos()
{
	unsigned int i = utils::to_u(f.get("feeds_pos"));
	if (i < visible_feeds.size()) {
		filterpos = visible_feeds[i].second;
		set_filterpos = true;
	}
}

void FeedListFormAction::register_format_styles()
{
	const std::string attrstr = rxman.get_attrs_stfl_string("feedlist", true);
	const std::string textview = strprintf::fmt(
			"{!list[feeds] .expand:vh style_normal[listnormal]: "
			"style_focus[listfocus]:fg=yellow,bg=blue,attr=bold "
			"pos_name[feedposname]: pos[feeds_pos]:0 %s richtext:1}",
			attrstr);
	feeds_list.stfl_replace_list(0, textview);
}

void FeedListFormAction::op_end_setfilter()
{
	std::string filtertext = qna_responses[0];
	filterhistory.add_line(filtertext);
	if (filtertext.length() > 0) {
		if (!matcher.parse(filtertext)) {
			v->show_error(
				_("Error: couldn't parse filter command!"));
		} else {
			save_filterpos();
			apply_filter = true;
			do_redraw = true;
		}
	}
}

void FeedListFormAction::op_start_search()
{
	std::string searchphrase = qna_responses[0];
	LOG(Level::DEBUG,
		"FeedListFormAction::op_start_search: starting search for "
		"`%s'",
		searchphrase);
	if (searchphrase.length() > 0) {
		v->set_status(_("Searching..."));
		searchhistory.add_line(searchphrase);
		std::vector<std::shared_ptr<RssItem>> items;
		try {
			std::string utf8searchphrase = utils::convert_text(
					searchphrase, "utf-8", nl_langinfo(CODESET));
			items = v->get_ctrl()->search_for_items(
					utf8searchphrase, nullptr);
		} catch (const DbException& e) {
			v->show_error(strprintf::fmt(
					_("Error while searching for `%s': %s"),
					searchphrase,
					e.what()));
			return;
		}
		if (!items.empty()) {
			search_dummy_feed->item_mutex.lock();
			search_dummy_feed->clear_items();
			search_dummy_feed->add_items(items);
			search_dummy_feed->item_mutex.unlock();
			v->push_searchresult(search_dummy_feed, searchphrase);
		} else {
			v->show_error(_("No results."));
		}
	}
}

void FeedListFormAction::handle_cmdline_num(unsigned int idx)
{
	if (idx > 0 &&
		idx <= (visible_feeds[visible_feeds.size() - 1].second + 1)) {
		int i = get_pos(idx - 1);
		if (i == -1) {
			v->show_error(_("Position not visible!"));
		} else {
			f.set("feeds_pos", std::to_string(i));
		}
	} else {
		v->show_error(_("Invalid position!"));
	}
}

void FeedListFormAction::set_pos()
{
	if (set_filterpos) {
		set_filterpos = false;
		unsigned int i = 0;
		for (const auto& feed : visible_feeds) {
			if (feed.second == filterpos) {
				f.set("feeds_pos", std::to_string(i));
				return;
			}
			i++;
		}
		f.set("feeds_pos", "0");
	} else if (zero_feedpos) {
		f.set("feeds_pos", "0");
		zero_feedpos = false;
	}
}

std::string FeedListFormAction::get_title(std::shared_ptr<RssFeed> feed)
{
	std::string title = feed->title();
	utils::remove_soft_hyphens(title);
	if (title.length() == 0) {
		title = utils::censor_url(feed->rssurl());
	}
	if (title.length() == 0) {
		title = "<no title>";
	}
	return title;
}

std::string FeedListFormAction::format_line(const std::string& feedlist_format,
	std::shared_ptr<RssFeed> feed,
	unsigned int pos,
	unsigned int width)
{
	FmtStrFormatter fmt;
	unsigned int unread_count = feed->unread_item_count();

	fmt.register_fmt('i', strprintf::fmt("%u", pos + 1));
	fmt.register_fmt('u',
		strprintf::fmt("(%u/%u)",
			unread_count,
			static_cast<unsigned int>(feed->total_item_count())));
	fmt.register_fmt('U', std::to_string(unread_count));
	fmt.register_fmt('c', std::to_string(feed->total_item_count()));
	fmt.register_fmt('n', unread_count > 0 ? "N" : " ");
	fmt.register_fmt('S', feed->get_status());
	fmt.register_fmt('t', get_title(feed));
	fmt.register_fmt('T', feed->get_firsttag());
	fmt.register_fmt('l', utils::censor_url(feed->link()));
	fmt.register_fmt('L', utils::censor_url(feed->rssurl()));
	fmt.register_fmt('d', utils::utf8_to_locale(feed->description()));

	auto formattedLine = fmt.do_format(feedlist_format, width);
	formattedLine = utils::quote_for_stfl(formattedLine);
	if (unread_count > 0) {
		formattedLine = strprintf::fmt("<unread>%s</>", formattedLine);
	}

	return formattedLine;
}

std::string FeedListFormAction::title()
{
	return strprintf::fmt(_("Feed List - %u unread, %u total"),
			unread_feeds,
			total_feeds);
}

} // namespace newsboat

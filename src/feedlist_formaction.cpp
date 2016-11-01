#include <feedlist_formaction.h>
#include <view.h>
#include <config.h>
#include <cassert>
#include <logger.h>
#include <reloadthread.h>
#include <exceptions.h>
#include <utils.h>
#include <formatstring.h>

#include <listformatter.h>

#include <sstream>
#include <string>
#include <algorithm>

#include <cstring>

#include <langinfo.h>

#define FILTER_UNREAD_FEEDS "unread_count != \"0\""

namespace newsbeuter {

feedlist_formaction::feedlist_formaction(view * vv, std::string formstr)
	: list_formaction(vv,formstr), zero_feedpos(false), feeds_shown(0),
	  quit(false), apply_filter(false), search_dummy_feed(new rss_feed(v->get_ctrl()->get_cache())),
	  filterpos(0), set_filterpos(false), rxman(0), old_width(0) {
	assert(true==m.parse(FILTER_UNREAD_FEEDS));
	valid_cmds.push_back("tag");
	valid_cmds.push_back("goto");
	std::sort(valid_cmds.begin(), valid_cmds.end());
	old_sort_order = v->get_cfg()->get_configvalue("feed-sort-order");
}

void feedlist_formaction::init() {
	set_keymap_hints();

	f->run(-3); // compute all widget dimensions

	if (v->get_ctrl()->get_refresh_on_start()) {
		v->get_ctrl()->start_reload_all_thread();
	}
	v->get_ctrl()->update_feedlist();

	/*
	 * This is kind of a hack.
	 * The feedlist_formaction is responsible for starting up the reloadthread, which is responsible
	 * for regularly spawning downloadthreads.
	 */
	std::thread t {reloadthread(v->get_ctrl(), v->get_cfg())};
	t.detach();

	apply_filter = !(v->get_cfg()->get_configvalue_as_bool("show-read-feeds"));
}

feedlist_formaction::~feedlist_formaction() { }

void feedlist_formaction::prepare() {
	unsigned int width = utils::to_u(f->get("items:w"));

	if (old_width != width) {
		do_redraw = true;
		old_width = width;
		LOG(LOG_DEBUG, "feedlist_formaction::prepare: apparent resize");
	}

	std::string sort_order = v->get_cfg()->get_configvalue("feed-sort-order");
	if (sort_order != old_sort_order) {
		v->get_ctrl()->sort_feeds();
		old_sort_order = sort_order;
		do_redraw = true;
	}

	if (do_redraw) {
		LOG(LOG_DEBUG, "feedlist_formaction::prepare: doing redraw");
		v->get_ctrl()->update_feedlist();
		set_pos();
		do_redraw = false;
	}
}

void feedlist_formaction::process_operation(operation op, bool automatic, std::vector<std::string> * args) {
	std::string feedpos = f->get("feedposname");
	unsigned int pos = utils::to_u(feedpos);
REDO:
	switch (op) {
	case OP_OPEN: {
		if (f->get_focus() == "feeds") {
			if (automatic && args->size() > 0) {
				pos = utils::to_u((*args)[0]);
			}
			LOG(LOG_INFO, "feedlist_formaction: opening feed at position `%s'",feedpos.c_str());
			if (feeds_shown > 0 && feedpos.length() > 0) {
				v->push_itemlist(pos);
			} else {
				v->show_error(_("No feed selected!")); // should not happen
			}
		}
	}
	break;
	case OP_RELOAD: {
		LOG(LOG_INFO, "feedlist_formaction: reloading feed at position `%s'",feedpos.c_str());
		if (feeds_shown > 0 && feedpos.length() > 0) {
			v->get_ctrl()->reload(pos);
		} else {
			v->show_error(_("No feed selected!")); // should not happen
		}
	}
	break;
	case OP_INT_RESIZE:
		do_redraw = true;
		break;
	case OP_RELOADURLS:
		v->get_ctrl()->reload_urls_file();
		break;
	case OP_SORT: {
		/// This string is related to the letters in parentheses in the
		/// "Sort by (f)irsttag/..." and "Reverse Sort by (f)irsttag/..." messages
		std::string input_options = _("ftaun");
		char c = v->confirm(_("Sort by (f)irsttag/(t)itle/(a)rticlecount/(u)nreadarticlecount/(n)one?"), input_options);
		if (!c) break;
		unsigned int n_options = ((std::string) "ftaun").length();
		if (input_options.length() < n_options) break;
		if (c == input_options.at(0)) {
			v->get_cfg()->set_configvalue("feed-sort-order", "firsttag-desc");
		} else if (c == input_options.at(1)) {
			v->get_cfg()->set_configvalue("feed-sort-order", "title-desc");
		} else if (c == input_options.at(2)) {
			v->get_cfg()->set_configvalue("feed-sort-order", "articlecount-desc");
		} else if (c == input_options.at(3)) {
			v->get_cfg()->set_configvalue("feed-sort-order", "unreadarticlecount-desc");
		} else if (c == input_options.at(4)) {
			v->get_cfg()->set_configvalue("feed-sort-order", "none-desc");
		}
	}
	break;
	case OP_REVSORT: {
		std::string input_options = _("ftaun");
		char c = v->confirm(_("Reverse Sort by (f)irsttag/(t)itle/(a)rticlecount/(u)nreadarticlecount/(n)one?"), input_options);
		if (!c) break;
		unsigned int n_options = ((std::string) "ftaun").length();
		if (input_options.length() < n_options) break;
		if (c == input_options.at(0)) {
			v->get_cfg()->set_configvalue("feed-sort-order", "firsttag-asc");
		} else if (c == input_options.at(1)) {
			v->get_cfg()->set_configvalue("feed-sort-order", "title-asc");
		} else if (c == input_options.at(2)) {
			v->get_cfg()->set_configvalue("feed-sort-order", "articlecount-asc");
		} else if (c == input_options.at(3)) {
			v->get_cfg()->set_configvalue("feed-sort-order", "unreadarticlecount-asc");
		} else if (c == input_options.at(4)) {
			v->get_cfg()->set_configvalue("feed-sort-order", "none-asc");
		}
	}
	break;
	case OP_OPENINBROWSER:
		if (feeds_shown > 0 && feedpos.length() > 0) {
			std::shared_ptr<rss_feed> feed = v->get_ctrl()->get_feed(pos);
			if (feed) {
				if (feed->rssurl().substr(0,6) != "query:") {
					LOG(LOG_INFO, "feedlist_formaction: opening feed at position `%s': %s", feedpos.c_str(), feed->link().c_str());
					v->open_in_browser(feed->link());
				} else {
					v->show_error(_("Cannot open query feeds in the browser!"));
				}
			}
		} else {
			v->show_error(_("No feed selected!"));
		}
		break;
	case OP_OPENALLUNREADINBROWSER:
		if (feeds_shown > 0 && feedpos.length() > 0) {
			std::shared_ptr<rss_feed> feed = v->get_ctrl()->get_feed(pos);
			if (feed) {
				LOG(LOG_INFO, "feedlist_formaction: opening all unread items in feed at position `%s'", feedpos.c_str());
				open_unread_items_in_browser(feed , false);
			}
		} else {
			v->show_error(_("No feed selected!"));
		}
		break;
	case OP_OPENALLUNREADINBROWSER_AND_MARK:
		if (feeds_shown > 0 && feedpos.length() > 0) {
			std::shared_ptr<rss_feed> feed = v->get_ctrl()->get_feed(pos);
			if (feed) {
				LOG(LOG_INFO, "feedlist_formaction: opening all unread items in feed at position `%s' and marking read", feedpos.c_str());
				open_unread_items_in_browser(feed , true);
				do_redraw = true;
			}
		}
		break;
	case OP_RELOADALL:
		LOG(LOG_INFO, "feedlist_formaction: reloading all feeds");
		{
			bool reload_only_visible_feeds = v->get_cfg()->get_configvalue_as_bool("reload-only-visible-feeds");
			std::vector<int> idxs;
			for (auto feed : visible_feeds) {
				idxs.push_back(feed.second);
			}
			v->get_ctrl()->start_reload_all_thread(reload_only_visible_feeds ? &idxs : nullptr);
		}
		break;
	case OP_MARKFEEDREAD: {
		LOG(LOG_INFO, "feedlist_formaction: marking feed read at position `%s'",feedpos.c_str());
		if (feeds_shown > 0 && feedpos.length() > 0) {
			v->set_status(_("Marking feed read..."));
			try {
				v->get_ctrl()->mark_all_read(pos);
				do_redraw = true;
				v->set_status("");
				if (feeds_shown > (pos + 1) && !apply_filter) {
					f->set("feedpos", utils::to_string<unsigned int>(pos + 1));
				}
			} catch (const dbexception& e) {
				v->show_error(utils::strprintf(_("Error: couldn't mark feed read: %s"), e.what()));
			}
		} else {
			v->show_error(_("No feed selected!")); // should not happen
		}
	}
	break;
	case OP_TOGGLESHOWREAD:
		m.parse(FILTER_UNREAD_FEEDS);
		LOG(LOG_INFO, "feedlist_formaction: toggling show-read-feeds");
		if (v->get_cfg()->get_configvalue_as_bool("show-read-feeds")) {
			v->get_cfg()->set_configvalue("show-read-feeds","no");
			apply_filter = true;
		} else {
			v->get_cfg()->set_configvalue("show-read-feeds","yes");
			apply_filter = false;
		}
		save_filterpos();
		do_redraw = true;
		break;
	case OP_NEXTUNREAD: {
		unsigned int local_tmp;
		LOG(LOG_INFO, "feedlist_formaction: jumping to next unread feed");
		if (!jump_to_next_unread_feed(local_tmp)) {
			v->show_error(_("No feeds with unread items."));
		}
	}
	break;
	case OP_PREVUNREAD: {
		unsigned int local_tmp;
		LOG(LOG_INFO, "feedlist_formaction: jumping to previous unread feed");
		if (!jump_to_previous_unread_feed(local_tmp)) {
			v->show_error(_("No feeds with unread items."));
		}
	}
	break;
	case OP_NEXT: {
		unsigned int local_tmp;
		LOG(LOG_INFO, "feedlist_formaction: jumping to next feed");
		if (!jump_to_next_feed(local_tmp)) {
			v->show_error(_("Already on last feed."));
		}
	}
	break;
	case OP_PREV: {
		unsigned int local_tmp;
		LOG(LOG_INFO, "feedlist_formaction: jumping to previous feed");
		if (!jump_to_previous_feed(local_tmp)) {
			v->show_error(_("Already on first feed."));
		}
	}
	break;
	case OP_RANDOMUNREAD: {
		unsigned int local_tmp;
		LOG(LOG_INFO, "feedlist_formaction: jumping to random unread feed");
		if (!jump_to_random_unread_feed(local_tmp)) {
			v->show_error(_("No feeds with unread items."));
		}
	}
	break;
	case OP_MARKALLFEEDSREAD:
		LOG(LOG_INFO, "feedlist_formaction: marking all feeds read");
		v->set_status(_("Marking all feeds read..."));
		if (tag == "") {
			v->get_ctrl()->catchup_all("");
		} else {
			// we're in tag view, so let's only touch feeds that are visible
			for (const auto& feedptr_pos_pair : visible_feeds) {
				auto rss_feed_ptr = feedptr_pos_pair.first;
				auto feedurl = rss_feed_ptr->rssurl();
				v->get_ctrl()->catchup_all(feedurl);
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
		if (v->get_ctrl()->get_filters().size() > 0) {
			std::string newfilter;
			if (automatic && args->size() > 0) {
				newfilter = (*args)[0];
			} else {
				newfilter = v->select_filter(v->get_ctrl()->get_filters().get_filters());
			}
			if (newfilter != "") {
				filterhistory.add_line(newfilter);
				if (newfilter.length() > 0) {
					if (!m.parse(newfilter)) {
						v->show_error(utils::strprintf(_("Error: couldn't parse filter command `%s': %s"), newfilter.c_str(), m.get_parse_error().c_str()));
						m.parse(FILTER_UNREAD_FEEDS);
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
			// when in automatic mode, we manually fill the qna_responses vector from the arguments
			// and then run the finished_qna() by ourselves to simulate a "Q&A" session that is
			// in fact macro-driven.
			qna_responses.push_back((*args)[0]);
			finished_qna(OP_INT_START_SEARCH);
		} else {
			std::vector<qna_pair> qna;
			qna.push_back(qna_pair(_("Search for: "), ""));
			this->start_qna(qna, OP_INT_START_SEARCH, &searchhistory);
		}
		break;
	case OP_CLEARFILTER:
		apply_filter = !(v->get_cfg()->get_configvalue_as_bool("show-read-feeds"));
		m.parse(FILTER_UNREAD_FEEDS);
		do_redraw = true;
		save_filterpos();
		break;
	case OP_SETFILTER:
		if (automatic && args->size() > 0) {
			qna_responses.clear();
			qna_responses.push_back((*args)[0]);
			finished_qna(OP_INT_END_SETFILTER);
		} else {
			std::vector<qna_pair> qna;
			qna.push_back(qna_pair(_("Filter: "), ""));
			this->start_qna(qna, OP_INT_END_SETFILTER, &filterhistory);
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
		LOG(LOG_INFO, "feedlist_formaction: quitting");
		if (automatic || !v->get_cfg()->get_configvalue_as_bool("confirm-exit") || v->confirm(_("Do you really want to quit (y:Yes n:No)? "), _("yn")) == *_("y")) {
			quit = true;
		}
		break;
	case OP_HARDQUIT:
		LOG(LOG_INFO, "feedlist_formaction: hard quitting");
		quit = true;
		break;
	case OP_HELP:
		v->push_help();
		break;
	default:
		break;
	}
	if (quit) {
		while (v->formaction_stack_size() > 0)
			v->pop_current_formaction();
	}
}

void feedlist_formaction::update_visible_feeds(std::vector<std::shared_ptr<rss_feed>>& feeds) {
	assert(v->get_cfg() != nullptr); // must not happen

	visible_feeds.clear();

	unsigned int i = 0;

	for (auto feed : feeds) {
		feed->set_index(i+1);
		if ((tag == "" || feed->matches_tag(tag)) && (!apply_filter || m.matches(feed.get())) && !feed->hidden()) {
			visible_feeds.push_back(feedptr_pos_pair(feed,i));
		}
		i++;
	}

	feeds_shown = visible_feeds.size();
}

void feedlist_formaction::set_feedlist(std::vector<std::shared_ptr<rss_feed>>& feeds) {
	assert(v->get_cfg() != nullptr); // must not happen

	unsigned int width = utils::to_u(f->get("feeds:w"));

	unsigned int i = 0;
	unread_feeds = 0;

	std::string feedlist_format = v->get_cfg()->get_configvalue("feedlist-format");

	listformatter listfmt;

	update_visible_feeds(feeds);

	for (auto feed : visible_feeds) {
		std::string title = get_title(feed.first);

		if (feed.first->unread_item_count() > 0)
			++unread_feeds;

		listfmt.add_line(format_line(feedlist_format, feed.first, feed.second, width), feed.second);
		i++;
	}

	total_feeds = i;

	f->modify("feeds","replace_inner",listfmt.format_list(rxman, "feedlist"));

	std::string title_format = v->get_cfg()->get_configvalue("feedlist-title-format");

	fmtstr_formatter fmt;
	fmt.register_fmt('T', tag);
	fmt.register_fmt('N', PROGRAM_NAME);
	fmt.register_fmt('V', PROGRAM_VERSION);
	fmt.register_fmt('u', utils::to_string<unsigned int>(unread_feeds));
	fmt.register_fmt('t', utils::to_string<unsigned int>(i));

	f->set("head", fmt.do_format(title_format, width));
}

void feedlist_formaction::set_tags(const std::vector<std::string>& t) {
	tags = t;
}

keymap_hint_entry * feedlist_formaction::get_keymap_hint() {
	static keymap_hint_entry hints[] = {
		{ OP_QUIT, _("Quit") },
		{ OP_OPEN, _("Open") },
		{ OP_NEXTUNREAD, _("Next Unread") },
		{ OP_RELOAD, _("Reload") },
		{ OP_RELOADALL, _("Reload All") },
		{ OP_MARKFEEDREAD, _("Mark Read") },
		{ OP_MARKALLFEEDSREAD, _("Catchup All") },
		{ OP_SEARCH, _("Search") },
		{ OP_HELP, _("Help") },
		{ OP_NIL, nullptr }
	};
	return hints;
}

bool feedlist_formaction::jump_to_previous_unread_feed(unsigned int& feedpos) {
	unsigned int curpos = utils::to_u(f->get("feedpos"));
	LOG(LOG_DEBUG, "feedlist_formaction::jump_to_previous_unread_feed: searching for unread feed");

	for (int i=curpos-1; i>=0; --i) {
		LOG(LOG_DEBUG, "feedlist_formaction::jump_to_previous_unread_feed: visible_feeds[%u] unread items: %u", i, visible_feeds[i].first->unread_item_count());
		if (visible_feeds[i].first->unread_item_count() > 0) {
			LOG(LOG_DEBUG, "feedlist_formaction::jump_to_previous_unread_feed: hit");
			f->set("feedpos", utils::to_string<unsigned int>(i));
			feedpos = visible_feeds[i].second;
			return true;
		}
	}
	for (int i=visible_feeds.size()-1; i>=static_cast<int>(curpos); --i) {
		LOG(LOG_DEBUG, "feedlist_formaction::jump_to_previous_unread_feed: visible_feeds[%u] unread items: %u", i, visible_feeds[i].first->unread_item_count());
		if (visible_feeds[i].first->unread_item_count() > 0) {
			LOG(LOG_DEBUG, "feedlist_formaction::jump_to_previous_unread_feed: hit");
			f->set("feedpos", utils::to_string<unsigned int>(i));
			feedpos = visible_feeds[i].second;
			return true;
		}
	}
	return false;
}

void feedlist_formaction::goto_feed(const std::string& str) {
	unsigned int curpos = utils::to_u(f->get("feedpos"));
	LOG(LOG_DEBUG, "feedlist_formaction::goto_feed: curpos = %u str = `%s'", curpos, str.c_str());
	for (unsigned int i=curpos+1; i<visible_feeds.size(); ++i) {
		if (strcasestr(visible_feeds[i].first->title().c_str(), str.c_str()) != nullptr) {
			f->set("feedpos", utils::to_string<unsigned int>(i));
			return;
		}
	}
	for (unsigned int i=0; i<=curpos; ++i) {
		if (strcasestr(visible_feeds[i].first->title().c_str(), str.c_str()) != nullptr) {
			f->set("feedpos", utils::to_string<unsigned int>(i));
			return;
		}
	}
}

bool feedlist_formaction::jump_to_random_unread_feed(unsigned int& feedpos) {
	bool unread_feeds_available = false;
	for (unsigned int i=0; i<visible_feeds.size(); ++i) {
		if (visible_feeds[i].first->unread_item_count() > 0) {
			unread_feeds_available = true;
			break;
		}
	}
	if (unread_feeds_available) {
		for (;;) {
			unsigned int pos = utils::get_random_value(visible_feeds.size());
			if (visible_feeds[pos].first->unread_item_count() > 0) {
				f->set("feedpos", utils::to_string<unsigned int>(pos));
				feedpos = visible_feeds[pos].second;
				break;
			}
		}
	}
	return unread_feeds_available;
}

bool feedlist_formaction::jump_to_next_unread_feed(unsigned int& feedpos) {
	unsigned int curpos = utils::to_u(f->get("feedpos"));
	LOG(LOG_DEBUG, "feedlist_formaction::jump_to_next_unread_feed: searching for unread feed");

	for (unsigned int i=curpos+1; i<visible_feeds.size(); ++i) {
		LOG(LOG_DEBUG, "feedlist_formaction::jump_to_next_unread_feed: visible_feeds[%u] unread items: %u", i, visible_feeds[i].first->unread_item_count());
		if (visible_feeds[i].first->unread_item_count() > 0) {
			LOG(LOG_DEBUG, "feedlist_formaction::jump_to_next_unread_feed: hit");
			f->set("feedpos", utils::to_string<unsigned int>(i));
			feedpos = visible_feeds[i].second;
			return true;
		}
	}
	for (unsigned int i=0; i<=curpos; ++i) {
		LOG(LOG_DEBUG, "feedlist_formaction::jump_to_next_unread_feed: visible_feeds[%u] unread items: %u", i, visible_feeds[i].first->unread_item_count());
		if (visible_feeds[i].first->unread_item_count() > 0) {
			LOG(LOG_DEBUG, "feedlist_formaction::jump_to_next_unread_feed: hit");
			f->set("feedpos", utils::to_string<unsigned int>(i));
			feedpos = visible_feeds[i].second;
			return true;
		}
	}
	return false;
}

bool feedlist_formaction::jump_to_previous_feed(unsigned int& feedpos) {
	unsigned int curpos = utils::to_u(f->get("feedpos"));

	if (curpos > 0) {
		unsigned int i = curpos-1;
		LOG(LOG_DEBUG, "feedlist_formaction::jump_to_previous_feed: visible_feeds[%u]", i);
		f->set("feedpos", utils::to_string<unsigned int>(i));
		feedpos = visible_feeds[i].second;
		return true;
	}
	return false;
}

bool feedlist_formaction::jump_to_next_feed(unsigned int& feedpos) {
	unsigned int curpos = utils::to_u(f->get("feedpos"));

	if ((curpos+1)<visible_feeds.size()) {
		unsigned int i = curpos+1;
		LOG(LOG_DEBUG, "feedlist_formaction::jump_to_next_feed: visible_feeds[%u]", i);
		f->set("feedpos", utils::to_string<unsigned int>(i));
		feedpos = visible_feeds[i].second;
		return true;
	}
	return false;
}

std::shared_ptr<rss_feed> feedlist_formaction::get_feed() {
	unsigned int curpos = utils::to_u(f->get("feedpos"));
	return visible_feeds[curpos].first;
}

int feedlist_formaction::get_pos(unsigned int realidx) {
	for (unsigned int i=0; i<visible_feeds.size(); ++i) {
		if (visible_feeds[i].second == realidx)
			return i;
	}
	return -1;
}

void feedlist_formaction::handle_cmdline(const std::string& cmd) {
	unsigned int idx = 0;
	/*
	 * this handle_cmdline is a bit different than the other ones.
	 * Since we want to use ":30" to jump to the 30th entry, we first
	 * need to check whether the command parses as unsigned integer,
	 * and if so, jump to the entered entry. Otherwise, we try to
	 * handle it as a normal command.
	 */
	if (1==sscanf(cmd.c_str(),"%u",&idx)) {
		handle_cmdline_num(idx);
	} else {
		// hand over all other commands to formaction
		std::vector<std::string> tokens = utils::tokenize_quoted(cmd, " \t");
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
				formaction::handle_cmdline(cmd);
			}
		}
	}
}

void feedlist_formaction::finished_qna(operation op) {
	formaction::finished_qna(op); // important!

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

void feedlist_formaction::mark_pos_if_visible(unsigned int pos) {
	scope_measure m1("feedlist_formaction::mark_pos_if_visible");
	unsigned int vpos = 0;
	v->get_ctrl()->update_visible_feeds();
	for (auto feed : visible_feeds) {
		if (feed.second == pos) {
			LOG(LOG_DEBUG, "feedlist_formaction::mark_pos_if_visible: match, setting position to %u", vpos);
			f->set("feedpos", utils::to_string<unsigned int>(vpos));
			return;
		}
		vpos++;
	}
	vpos = 0;
	pos = v->get_ctrl()->get_pos_of_next_unread(pos);
	for (auto feed : visible_feeds) {
		if (feed.second == pos) {
			LOG(LOG_DEBUG, "feedlist_formaction::mark_pos_if_visible: match in 2nd try, setting position to %u", vpos);
			f->set("feedpos", utils::to_string<unsigned int>(vpos));
			return;
		}
		vpos++;
	}
}

void feedlist_formaction::save_filterpos() {
	unsigned int i = utils::to_u(f->get("feedpos"));
	if (i<visible_feeds.size()) {
		filterpos = visible_feeds[i].second;
		set_filterpos = true;
	}
}

void feedlist_formaction::set_regexmanager(regexmanager * r) {
	rxman = r;
	std::vector<std::string>& attrs = r->get_attrs("feedlist");
	unsigned int i=0;
	std::string attrstr;
	for (auto attribute : attrs) {
		attrstr.append(utils::strprintf("@style_%u_normal:%s ", i, attribute.c_str()));
		attrstr.append(utils::strprintf("@style_%u_focus:%s ", i, attribute.c_str()));
		i++;
	}
	std::string textview = utils::strprintf("{!list[feeds] .expand:vh style_normal[listnormal]: style_focus[listfocus]:fg=yellow,bg=blue,attr=bold pos_name[feedposname]: pos[feedpos]:0 %s richtext:1}", attrstr.c_str());
	f->modify("feeds", "replace", textview);
}


void feedlist_formaction::op_end_setfilter() {
	std::string filtertext = qna_responses[0];
	filterhistory.add_line(filtertext);
	if (filtertext.length() > 0) {
		if (!m.parse(filtertext)) {
			v->show_error(_("Error: couldn't parse filter command!"));
			m.parse(FILTER_UNREAD_FEEDS);
		} else {
			save_filterpos();
			apply_filter = true;
			do_redraw = true;
		}
	}
}


void feedlist_formaction::op_start_search() {
	std::string searchphrase = qna_responses[0];
	LOG(LOG_DEBUG, "feedlist_formaction::op_start_search: starting search for `%s'", searchphrase.c_str());
	if (searchphrase.length() > 0) {
		v->set_status(_("Searching..."));
		searchhistory.add_line(searchphrase);
		std::vector<std::shared_ptr<rss_item>> items;
		try {
			std::string utf8searchphrase = utils::convert_text(searchphrase, "utf-8", nl_langinfo(CODESET));
			items = v->get_ctrl()->search_for_items(utf8searchphrase, nullptr);
		} catch (const dbexception& e) {
			v->show_error(utils::strprintf(_("Error while searching for `%s': %s"), searchphrase.c_str(), e.what()));
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

void feedlist_formaction::handle_cmdline_num(unsigned int idx) {
	if (idx > 0 && idx <= (visible_feeds[visible_feeds.size()-1].second + 1)) {
		int i = get_pos(idx - 1);
		if (i == -1) {
			v->show_error(_("Position not visible!"));
		} else {
			f->set("feedpos", utils::to_string<unsigned int>(i));
		}
	} else {
		v->show_error(_("Invalid position!"));
	}
}

void feedlist_formaction::set_pos() {
	if (set_filterpos) {
		set_filterpos = false;
		unsigned int i = 0;
		for (auto feed : visible_feeds) {
			if (feed.second == filterpos) {
				f->set("feedpos", utils::to_string<unsigned int>(i));
				return;
			}
			i++;
		}
		f->set("feedpos", "0");
	} else if (zero_feedpos) {
		f->set("feedpos","0");
		zero_feedpos = false;
	}
}

std::string feedlist_formaction::get_title(std::shared_ptr<rss_feed> feed) {
	std::string title = feed->title();
	if (title.length()==0)
		title = utils::censor_url(feed->rssurl());
	if (title.length()==0)
		title = "<no title>";
	return title;
}

std::string feedlist_formaction::format_line(const std::string& feedlist_format, std::shared_ptr<rss_feed> feed, unsigned int pos, unsigned int width) {
	fmtstr_formatter fmt;
	unsigned int unread_count = feed->unread_item_count();
	std::string tmp_feedlist_format = feedlist_format;

	fmt.register_fmt('i', utils::strprintf("%u", pos + 1));
	fmt.register_fmt('u', utils::strprintf("(%u/%u)",unread_count,static_cast<unsigned int>(feed->total_item_count())));
	fmt.register_fmt('U', utils::to_string(unread_count));
	fmt.register_fmt('c', utils::to_string(feed->total_item_count()));
	fmt.register_fmt('n', unread_count > 0 ? "N" : " ");
	fmt.register_fmt('S', feed->get_status());
	fmt.register_fmt('t', get_title(feed));
	fmt.register_fmt('T', feed->get_firsttag());
	fmt.register_fmt('l', utils::censor_url(feed->link()));
	fmt.register_fmt('L', utils::censor_url(feed->rssurl()));
	fmt.register_fmt('d', feed->description());

	if (unread_count > 0) {
		tmp_feedlist_format = utils::strprintf(
		                          "<unread>%s</>",
		                          feedlist_format.c_str());
	}

	return fmt.do_format(tmp_feedlist_format, width);
}

std::string feedlist_formaction::title() {
	return utils::strprintf(_("Feed List - %u unread, %u total"), unread_feeds, total_feeds);
}

}

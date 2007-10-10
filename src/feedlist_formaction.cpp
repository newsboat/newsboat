#include <feedlist_formaction.h>
#include <view.h>
#include <config.h>
#include <cassert>
#include <logger.h>
#include <reloadthread.h>
#include <exceptions.h>
#include <utils.h>
#include <formatstring.h>

#include <sstream>
#include <cassert>
#include <string>

#define FILTER_UNREAD_FEEDS "unread_count != \"0\""

namespace newsbeuter {

feedlist_formaction::feedlist_formaction(view * vv, std::string formstr) 
	: formaction(vv,formstr), zero_feedpos(false), feeds_shown(0),
		auto_open(false), quit(false), apply_filter(false), search_dummy_feed(v->get_ctrl()->get_cache()) {
	assert(true==m.parse(FILTER_UNREAD_FEEDS));
}

void feedlist_formaction::init() {
	set_keymap_hints();

	f->run(-3); // compute all widget dimensions

	if(v->get_ctrl()->get_refresh_on_start()) {
		f->run(-1); // FRUN
		v->get_ctrl()->start_reload_all_thread();
	}
	v->get_ctrl()->update_feedlist();

	/*
	 * This is kind of a hack.
	 * The feedlist_formaction is responsible for starting up the reloadthread, which is responsible
	 * for regularly spawning downloadthreads.
	 */
	unsigned int reload_cycle = 60 * static_cast<unsigned int>(v->get_cfg()->get_configvalue_as_int("reload-time"));
	if (v->get_cfg()->get_configvalue_as_bool("auto-reload") == true) {
		f->run(-1); // FRUN
		reloadthread  * rt = new reloadthread(v->get_ctrl(), reload_cycle, v->get_cfg());
		rt->start();
	}

	apply_filter = !(v->get_cfg()->get_configvalue_as_bool("show-read-feeds"));
}

feedlist_formaction::~feedlist_formaction() { }

void feedlist_formaction::prepare() {
	static unsigned int old_width = 0;

	std::string listwidth = f->get("items:w");
	std::istringstream is(listwidth);
	unsigned int width;
	is >> width;

	if (old_width != width) {
		do_redraw = true;
		old_width = width;
		GetLogger().log(LOG_DEBUG, "feedlist_formaction::prepare: apparent resize");
	}

	if (do_redraw) {
		GetLogger().log(LOG_DEBUG, "feedlist_formaction::prepare: doing redraw");
		do_redraw = false;
		v->get_ctrl()->update_feedlist();
		if (zero_feedpos) {
			f->set("feedpos","0");
			zero_feedpos = false;
		}
	}
}

void feedlist_formaction::process_operation(operation op) {
	std::string feedpos = f->get("feedposname");
	std::istringstream posname(feedpos);
	unsigned int pos = 0;
	posname >> pos;
	switch (op) {
		case OP_OPEN: {
				if (f->get_focus() == "feeds") {
					GetLogger().log(LOG_INFO, "feedlist_formaction: opening feed at position `%s'",feedpos.c_str());
					if (feeds_shown > 0 && feedpos.length() > 0) {
						v->push_itemlist(pos);
					} else {
						v->show_error(_("No feed selected!")); // should not happen
					}
				}
			}
			break;
		case OP_RELOAD: {
				GetLogger().log(LOG_INFO, "feedlist_formaction: reloading feed at position `%s'",feedpos.c_str());
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
		case OP_RELOADALL:
			GetLogger().log(LOG_INFO, "feedlist_formaction: reloading all feeds");
			v->get_ctrl()->start_reload_all_thread();
			break;
		case OP_MARKFEEDREAD: {
				GetLogger().log(LOG_INFO, "feedlist_formaction: marking feed read at position `%s'",feedpos.c_str());
				if (feeds_shown > 0 && feedpos.length() > 0) {
					v->set_status(_("Marking feed read..."));
					try {
						v->get_ctrl()->mark_all_read(pos);
						do_redraw = true;
						v->set_status("");
					} catch (const dbexception& e) {
						char buf[1024];
						snprintf(buf, sizeof(buf), _("Error: couldn't mark feed read: %s"), e.what());
						v->show_error(buf);
					}
				} else {
					v->show_error(_("No feed selected!")); // should not happen
				}
			}
			break;
		case OP_TOGGLESHOWREAD:
			m.parse(FILTER_UNREAD_FEEDS);
			GetLogger().log(LOG_INFO, "feedlist_formaction: toggling show-read-feeds");
			if (v->get_cfg()->get_configvalue_as_bool("show-read-feeds")) {
				v->get_cfg()->set_configvalue("show-read-feeds","no");
				apply_filter = true;
				f->set("feedpos", "0");
			} else {
				v->get_cfg()->set_configvalue("show-read-feeds","yes");
				apply_filter = false;
			}
			do_redraw = true;
			break;
		case OP_NEXTUNREAD: {
				unsigned int feedpos;
				GetLogger().log(LOG_INFO, "feedlist_formaction: jumping to next unred feed");
				if (!jump_to_next_unread_feed(feedpos)) {
					v->show_error(_("No feeds with unread items."));
				}
			}
			break;
		case OP_PREVUNREAD: {
				unsigned int feedpos;
				GetLogger().log(LOG_INFO, "feedlist_formaction: jumping to previous unred feed");
				if (!jump_to_previous_unread_feed(feedpos)) {
					v->show_error(_("No feeds with unread items."));
				}
			}
			break;
		case OP_MARKALLFEEDSREAD:
			GetLogger().log(LOG_INFO, "feedlist_formaction: marking all feeds read");
			v->set_status(_("Marking all feeds read..."));
			v->get_ctrl()->catchup_all();
			v->set_status("");
			do_redraw = true;
			break;
		case OP_CLEARTAG:
			tag = "";
			do_redraw = true;
			zero_feedpos = true;
			break;
		case OP_SETTAG: 
			if (tags.size() > 0) {
				std::string newtag = v->select_tag(tags);
				if (newtag != "") {
					tag = newtag;
					do_redraw = true;
					zero_feedpos = true;
				}
			} else {
				v->show_error(_("No tags defined."));
			}
			break;
		case OP_SELECTFILTER:
			if (v->get_ctrl()->get_filters().size() > 0) {
				std::string newfilter = v->select_filter(v->get_ctrl()->get_filters().get_filters());
				if (newfilter != "") {
					filterhistory.add_line(newfilter);
					if (newfilter.length() > 0) {
						if (!m.parse(newfilter)) {
							v->show_error(_("Error: couldn't parse filter command!"));
							m.parse(FILTER_UNREAD_FEEDS);
						} else {
							apply_filter = true;
							do_redraw = true;
						}
					}
				}
			} else {
				v->show_error(_("No filters defined."));
			}
			break;
		case OP_SEARCH: {
				std::vector<qna_pair> qna;
				qna.push_back(qna_pair(_("Search for: "), ""));
				this->start_qna(qna, OP_INT_START_SEARCH, &searchhistory);
			}
			break;
		case OP_CLEARFILTER:
			apply_filter = !(v->get_cfg()->get_configvalue_as_bool("show-read-feeds"));
			m.parse(FILTER_UNREAD_FEEDS);
			do_redraw = true;
			break;
		case OP_SETFILTER: {
				std::vector<qna_pair> qna;
				qna.push_back(qna_pair(_("Filter: "), ""));
				this->start_qna(qna, OP_INT_END_SETFILTER, &filterhistory);
			}
			break;
		case OP_QUIT:
			GetLogger().log(LOG_INFO, "feedlist_formaction: quitting");
			if (!v->get_cfg()->get_configvalue_as_bool("confirm-exit") || v->confirm(_("Do you really want to quit (y:Yes n:No)? "), _("yn")) == *_("y")) {
				quit = true;
			}
			break;
		case OP_HELP:
			v->push_help();
			break;
		default:
			break;
	}
	if (quit) {
		v->pop_current_formaction();
	}
}

void feedlist_formaction::set_feedlist(std::vector<rss_feed>& feeds) {
	std::string code = "{list";
	
	assert(v->get_cfg() != NULL); // must not happen

	std::string listwidth = f->get("feeds:w");
	std::istringstream is(listwidth);
	unsigned int width;
	is >> width;

	feeds_shown = 0;
	unsigned int i = 0;
	unsigned short feedlist_number = 1;
	unsigned int unread_feeds = 0;

	if (visible_feeds.size() > 0)
		visible_feeds.erase(visible_feeds.begin(), visible_feeds.end());

	std::string feedlist_format = v->get_cfg()->get_configvalue("feedlist-format");

	for (std::vector<rss_feed>::iterator it = feeds.begin(); it != feeds.end(); ++it, ++i, ++feedlist_number) {
		rss_feed feed = *it;
		std::string title = it->title();
		if (title.length()==0) {
			title = it->rssurl(); // rssurl must always be present.
			if (title.length()==0) {
				title = "<no title>"; // shouldn't happen
			}
		}

		// TODO: refactor
		char sbuf[20];
		char idxbuf[5];
		unsigned int unread_count = 0;
		if (it->items().size() > 0) {
			unread_count = it->unread_item_count();
		}
		if (unread_count > 0)
			++unread_feeds;

		/*
		 * we only display an entry in the feedlist if:
		 *   - no tag is active, or the entry matches the currently selected tag
		 *   - no filter shall be applied, or the entry matches the currently set filter
		 */
		if ((tag == "" || it->matches_tag(tag)) && (!apply_filter || m.matches(&(*it)))) {
			visible_feeds.push_back(feedptr_pos_pair(&(*it),i));

			fmtstr_formatter fmt;

			snprintf(idxbuf, sizeof(idxbuf),"%u", feedlist_number);
			snprintf(sbuf,sizeof(sbuf),"(%u/%u)",unread_count,static_cast<unsigned int>(it->items().size()));

			fmt.register_fmt('i', idxbuf);
			fmt.register_fmt('u', sbuf);
			fmt.register_fmt('n', unread_count > 0 ? "N" : " ");
			fmt.register_fmt('t', title);
			fmt.register_fmt('l', it->link());
			fmt.register_fmt('L', it->rssurl());
			fmt.register_fmt('d', it->description());

			std::string line = "{listitem[";
			std::ostringstream num;
			num << i;
			line.append(num.str());
			line.append("] text:");
			line.append(stfl::quote(fmt.do_format(feedlist_format, width)));
			line.append("}");

			code.append(line);

			++feeds_shown;
		}
	}

	code.append("}");

	f->modify("feeds","replace_inner",code);

	std::string title_format = v->get_cfg()->get_configvalue("feedlist-title-format");

	fmtstr_formatter fmt;
	fmt.register_fmt('T', tag);
	fmt.register_fmt('N', PROGRAM_NAME);
	fmt.register_fmt('V', PROGRAM_VERSION);
	fmt.register_fmt('u', utils::to_s(unread_feeds));
	fmt.register_fmt('t', utils::to_s(i));

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
		{ OP_NIL, NULL }
	};
	return hints;
}

bool feedlist_formaction::jump_to_previous_unread_feed(unsigned int& feedpos) {
	unsigned int curpos;
	std::istringstream is(f->get("feedpos"));
	is >> curpos;
	GetLogger().log(LOG_DEBUG, "feedlist_formaction::jump_to_previous_unread_feed: searching for unread feed");

	for (int i=curpos-1;i>=0;--i) {
		GetLogger().log(LOG_DEBUG, "feedlist_formaction::jump_to_previous_unread_feed: visible_feeds[%u] unread items: %u", i, visible_feeds[i].first->unread_item_count());
		if (visible_feeds[i].first->unread_item_count() > 0) {
			GetLogger().log(LOG_DEBUG, "feedlist_formaction::jump_to_previous_unread_feed: hit");
			std::ostringstream os;
			os << i;
			f->set("feedpos", os.str());
			feedpos = visible_feeds[i].second;
			return true;
		}
	}
	for (int i=visible_feeds.size()-1;i>=static_cast<int>(curpos);--i) {
		GetLogger().log(LOG_DEBUG, "feedlist_formaction::jump_to_previous_unread_feed: visible_feeds[%u] unread items: %u", i, visible_feeds[i].first->unread_item_count());
		if (visible_feeds[i].first->unread_item_count() > 0) {
			GetLogger().log(LOG_DEBUG, "feedlist_formaction::jump_to_previous_unread_feed: hit");
			std::ostringstream os;
			os << i;
			f->set("feedpos", os.str());
			return true;
		}
	}
	return false;
}

bool feedlist_formaction::jump_to_next_unread_feed(unsigned int& feedpos) {
	unsigned int curpos;
	std::istringstream is(f->get("feedpos"));
	is >> curpos;
	GetLogger().log(LOG_DEBUG, "feedlist_formaction::jump_to_next_unread_feed: searching for unread feed");

	for (unsigned int i=curpos+1;i<visible_feeds.size();++i) {
		GetLogger().log(LOG_DEBUG, "feedlist_formaction::jump_to_next_unread_feed: visible_feeds[%u] unread items: %u", i, visible_feeds[i].first->unread_item_count());
		if (visible_feeds[i].first->unread_item_count() > 0) {
			GetLogger().log(LOG_DEBUG, "feedlist_formaction::jump_to_next_unread_feed: hit");
			std::ostringstream os;
			os << i;
			f->set("feedpos", os.str());
			feedpos = visible_feeds[i].second;
			return true;
		}
	}
	for (unsigned int i=0;i<=curpos;++i) {
		GetLogger().log(LOG_DEBUG, "feedlist_formaction::jump_to_next_unread_feed: visible_feeds[%u] unread items: %u", i, visible_feeds[i].first->unread_item_count());
		if (visible_feeds[i].first->unread_item_count() > 0) {
			GetLogger().log(LOG_DEBUG, "feedlist_formaction::jump_to_next_unread_feed: hit");
			std::ostringstream os;
			os << i;
			f->set("feedpos", os.str());
			return true;
		}
	}
	return false;
}

rss_feed * feedlist_formaction::get_feed() {
	unsigned int curpos;
	std::istringstream is(f->get("feedpos"));
	is >> curpos;
	return visible_feeds[curpos].first;
}

int feedlist_formaction::get_pos(unsigned int realidx) {
	for (unsigned int i=0;i<visible_feeds.size();++i) {
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
		if (idx > 0 && idx <= (visible_feeds[visible_feeds.size()-1].second + 1)) {
			int i = get_pos(idx - 1);
			if (i == -1) {
				v->show_error(_("Position not visible!"));
			} else {
				std::ostringstream idxstr;
				idxstr << i;
				f->set("feedpos", idxstr.str());
			}
		} else {
			v->show_error(_("Invalid position!"));
		}
	} else {
		// hand over all other commands to formaction
		std::vector<std::string> tokens = utils::tokenize_quoted(cmd, " \t");
		if (tokens.size() > 0) {
			if (tokens[0] == "tag") {
				if (tokens.size() >= 2 && tokens[1] != "") {
					tag = tokens[1];
					do_redraw = true;
					zero_feedpos = true;
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
		case OP_INT_END_SETFILTER: {
				std::string filtertext = qna_responses[0];
				filterhistory.add_line(filtertext);
				if (filtertext.length() > 0) {
					if (!m.parse(filtertext)) {
						v->show_error(_("Error: couldn't parse filter command!"));
						m.parse(FILTER_UNREAD_FEEDS);
					} else {
						f->set("feedpos", "0");
						apply_filter = true;
						do_redraw = true;
					}
				}
			}
			break;
		case OP_INT_START_SEARCH: {
				std::string searchphrase = qna_responses[0];
				if (searchphrase.length() > 0) {
					v->set_status(_("Searching..."));
					searchhistory.add_line(searchphrase);
					std::vector<rss_item> items;
					try {
						items = v->get_ctrl()->search_for_items(searchphrase, "");
					} catch (const dbexception& e) {
						char buf[1024];
						snprintf(buf, sizeof(buf), _("Error while searching for `%s': %s"), searchphrase.c_str(), e.what());
						v->show_error(buf);
						return;
					}
					if (items.size() > 0) {
						search_dummy_feed.items() = items;
						v->push_searchresult(&search_dummy_feed);
					} else {
						v->show_error(_("No results."));
					}
				}
			}
			break;
		default:
			break;
	}
}

}

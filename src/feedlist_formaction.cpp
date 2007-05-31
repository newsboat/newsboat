#include <feedlist_formaction.h>
#include <view.h>
#include <config.h>
#include <cassert>
#include <logger.h>
#include <reloadthread.h>

#include <sstream>
#include <cassert>
#include <string>

namespace newsbeuter {

feedlist_formaction::feedlist_formaction(view * vv, std::string formstr) 
	: formaction(vv,formstr), zero_feedpos(false), feeds_shown(0),
		auto_open(false), quit(false) {
	assert(true==m.parse("unread_count != \"0\""));
}

void feedlist_formaction::init() {
	set_keymap_hints();

	// XXX: does this even work with no element on the formaction_stack ?
	if(v->get_ctrl()->get_refresh_on_start()) {
		f->run(-1);
		v->get_ctrl()->start_reload_all_thread();
	}

	unsigned int reload_cycle = 60 * static_cast<unsigned int>(v->get_cfg()->get_configvalue_as_int("reload-time"));
	if (v->get_cfg()->get_configvalue_as_bool("auto-reload") == true) {
		f->run(-1);
		reloadthread  * rt = new reloadthread(v->get_ctrl(), reload_cycle, v->get_cfg());
		rt->start();
	}
}

feedlist_formaction::~feedlist_formaction() { }

void feedlist_formaction::prepare() {
	if (do_redraw) {
		do_redraw = false;
		v->get_ctrl()->update_feedlist();
		if (zero_feedpos) {
			f->set("feedpos","0");
			zero_feedpos = false;
		}
	}
}

void feedlist_formaction::process_operation(operation op, int raw_char) {
	switch (op) {
		case OP_OPEN: {
				if (f->get_focus() == "feeds") {
					std::string feedpos = f->get("feedposname");
					GetLogger().log(LOG_INFO, "feedlist_formaction: opening feed at position `%s'",feedpos.c_str());
					if (feeds_shown > 0 && feedpos.length() > 0) {
						std::istringstream posname(feedpos);
						unsigned int pos = 0;
						posname >> pos;
						v->push_itemlist(pos);
					} else {
						v->show_error(_("No feed selected!")); // should not happen
					}
				}
			}
			break;
		case OP_RELOAD: {
				std::string feedposname = f->get("feedposname");
				GetLogger().log(LOG_INFO, "feedlist_formaction: reloading feed at position `%s'",feedposname.c_str());
				if (feeds_shown > 0 && feedposname.length() > 0) {
					std::istringstream posname(feedposname);
					unsigned int pos = 0;
					posname >> pos;
					v->get_ctrl()->reload(pos);
				} else {
					v->show_error(_("No feed selected!")); // should not happen
				}
			}
			break;
		case OP_RELOADALL:
			GetLogger().log(LOG_INFO, "feedlist_formaction: reloading all feeds");
			v->get_ctrl()->start_reload_all_thread();
			break;
		case OP_MARKFEEDREAD: {
				std::string feedposname = f->get("feedposname");
				GetLogger().log(LOG_INFO, "feedlist_formaction: marking feed read at position `%s'",feedposname.c_str());
				if (feeds_shown > 0 && feedposname.length() > 0) {
					v->set_status(_("Marking feed read..."));
					std::istringstream posname(feedposname);
					unsigned int pos = 0;
					posname >> pos;
					v->get_ctrl()->mark_all_read(pos);
					do_redraw = true;
					v->set_status("");
				} else {
					v->show_error(_("No feed selected!")); // should not happen
				}
			}
			break;
		case OP_TOGGLESHOWREAD:
			GetLogger().log(LOG_INFO, "feedlist_formaction: toggling show-read-feeds");
			if (v->get_cfg()->get_configvalue_as_bool("show-read-feeds")) {
				v->get_cfg()->set_configvalue("show-read-feeds","no");
			} else {
				v->get_cfg()->set_configvalue("show-read-feeds","yes");
			}
			do_redraw = true;
			break;
		case OP_NEXTUNREAD:
			GetLogger().log(LOG_INFO, "feedlist_formaction: jumping to next unred feed");
			if (!jump_to_next_unread_feed()) {
				v->show_error(_("No feeds with unread items."));
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
		case OP_SEARCH:
			v->run_search();
			break;
		case OP_QUIT:
			GetLogger().log(LOG_INFO, "feedlist_formaction: quitting");
			if (!v->get_cfg()->get_configvalue_as_bool("confirm-exit") || v->confirm(_("Do you really want to quit (y:Yes n:No)? "), "yn") == 'y') {
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
	char buf[1024];
	
	assert(v->get_cfg() != NULL); // must not happen
	
	bool show_read_feeds = v->get_cfg()->get_configvalue_as_bool("show-read-feeds");
	
	// std::cerr << "show-read-feeds" << (show_read_feeds?"true":"false") << std::endl;

	feeds_shown = 0;
	unsigned int i = 0;
	unsigned short feedlist_number = 1;
	unsigned int unread_feeds = 0;

	if (visible_feeds.size() > 0)
		visible_feeds.erase(visible_feeds.begin(), visible_feeds.end());

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
		char buf[20];
		char buf2[20];
		unsigned int unread_count = 0;
		if (it->items().size() > 0) {
			unread_count = it->unread_item_count();
		}
		if (unread_count > 0)
			++unread_feeds;


		if ((tag == "" || it->matches_tag(tag)) && (show_read_feeds || m.matches(&(*it)))) {
			visible_feeds.push_back(std::pair<rss_feed *, unsigned int>(&(*it),i));

			snprintf(buf,sizeof(buf),"(%u/%u) ",unread_count,static_cast<unsigned int>(it->items().size()));
			snprintf(buf2,sizeof(buf2),"%4u %c %11s",feedlist_number, unread_count > 0 ? 'N' : ' ',buf);
			std::string newtitle(buf2);
			newtitle.append(title);
			title = newtitle;

			std::string line = "{listitem[";
			std::ostringstream num;
			num << i;
			line.append(num.str());
			line.append("] text:");
			line.append(stfl::quote(title));
			line.append("}");

			code.append(line);

			++feeds_shown;
		}
	}

	code.append("}");

	f->modify("feeds","replace_inner",code);

	if (tag.length() > 0) {
		snprintf(buf, sizeof(buf), _("%s %s - Your feeds (%u unread, %u total) - tag `%s'"), PROGRAM_NAME, PROGRAM_VERSION, unread_feeds, i, tag.c_str());
	} else {
		snprintf(buf, sizeof(buf), _("%s %s - Your feeds (%u unread, %u total)"), PROGRAM_NAME, PROGRAM_VERSION, unread_feeds, i);
	}

	f->set("head", buf);
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

bool feedlist_formaction::jump_to_next_unread_feed() {
	unsigned int curpos;
	std::istringstream is(f->get("feedpos"));
	is >> curpos;

	for (unsigned int i=curpos+1;i<visible_feeds.size();++i) {
		if (visible_feeds[i].first->unread_item_count() > 0) {
			std::ostringstream os;
			os << i;
			f->set("feedpos", os.str());
			return true;
		}
	}
	for (unsigned int i=0;i<=curpos;++i) {
		if (visible_feeds[i].first->unread_item_count() > 0) {
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
		formaction::handle_cmdline(cmd);
	}
}

}

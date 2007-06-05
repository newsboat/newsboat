#include <itemlist_formaction.h>
#include <view.h>
#include <config.h>
#include <logger.h>

#include <sstream>

namespace newsbeuter {

itemlist_formaction::itemlist_formaction(view * vv, std::string formstr)
	: formaction(vv,formstr), feed(0), apply_filter(false), update_visible_items(true) { 
}

itemlist_formaction::~itemlist_formaction() { }

void itemlist_formaction::process_operation(operation op) {
	bool quit = false;
	switch (op) {
		case OP_INT_END_SETFILTER: {
				std::string filtertext = f->get("filtertext");
				f->modify("lastline","replace","{hbox[lastline] .expand:0 {label[msglabel] .expand:h text[msg]:\"\"}}");
				if (filtertext.length() > 0) {
					if (!m.parse(filtertext)) {
						v->show_error(_("Error: couldn't parse filter command!"));
					} else {
						apply_filter = true;
						update_visible_items = true;
						do_redraw = true;
					}
				}
			}
			break;
		case OP_OPEN: {
				std::string itemposname = f->get("itempos");
				GetLogger().log(LOG_INFO, "itemlist_formaction: opening item at pos `%s'", itemposname.c_str());
				if (itemposname.length() > 0) {
					std::istringstream posname(itemposname);
					unsigned int itempos = 0;
					posname >> itempos;
					v->push_itemview(feed, visible_items[itempos].first->guid());
					do_redraw = true;
				} else {
					v->show_error(_("No item selected!")); // should not happen
				}
			}
			break;
		case OP_OPENINBROWSER: {
				std::string itemposname = f->get("itempos");
				GetLogger().log(LOG_INFO, "itemlist_formaction: opening item at pos `%s'", itemposname.c_str());
				if (itemposname.length() > 0) {
					std::istringstream posname(itemposname);
					unsigned int itempos = 0;
					posname >> itempos;
					if (itempos < visible_items.size()) {
						v->open_in_browser(visible_items[itempos].first->link());
						do_redraw = true;
					}
				} else {
					v->show_error(_("No item selected!")); // should not happen
				}
			}
			break;
		case OP_SAVE: 
			{
				char buf[1024];
				std::string itemposname = f->get("itempos");
				GetLogger().log(LOG_INFO, "itemlist_formaction: saving item at pos `%s'", itemposname.c_str());
				if (itemposname.length() > 0) {
					std::istringstream posname(itemposname);
					unsigned int itempos = 0;
					posname >> itempos;
					
					std::string filename = v->run_filebrowser(FBT_SAVE,v->get_filename_suggestion(visible_items[itempos].first->title()));
					if (filename == "") {
						v->show_error(_("Aborted saving."));
					} else {
						try {
							v->write_item(*visible_items[itempos].first, filename);
							snprintf(buf, sizeof(buf), _("Saved article to %s"), filename.c_str());
							v->show_error(buf);
						
						} catch (...) {
							snprintf(buf, sizeof(buf), _("Error: couldn't save article to %s"), filename.c_str());
							v->show_error(buf);
						}
					}
				} else {
					v->show_error(_("Error: no item selected!"));
				}
			}
			break;
		case OP_HELP:
			v->push_help();
			break;
		case OP_RELOAD:
			GetLogger().log(LOG_INFO, "itemlist_formaction: reloading current feed");
			v->get_ctrl()->reload(pos);
			// feed = v->get_ctrl()->get_feed(pos);
			update_visible_items = true;
			do_redraw = true;
			break;
		case OP_QUIT:
			GetLogger().log(LOG_INFO, "itemlist_formaction: quitting");
			quit = true;
			break;
		case OP_NEXTUNREAD:
			GetLogger().log(LOG_INFO, "itemlist_formaction: jumping to next unread item");
			if (!jump_to_next_unread_item(false)) {
				v->show_error(_("No unread items."));
			}
			break;
		case OP_MARKFEEDREAD:
			GetLogger().log(LOG_INFO, "itemlist_formaction: marking feed read");
			v->set_status(_("Marking feed read..."));
			v->get_ctrl()->mark_all_read(pos);
			v->set_status("");
			do_redraw = true;
			break;
		case OP_SEARCH:
			v->run_search(feed->rssurl());
			break;
		case OP_TOGGLEITEMREAD: {
				std::string itemposname = f->get("itempos");
				GetLogger().log(LOG_INFO, "itemlist_formaction: toggling item read at pos `%s'", itemposname.c_str());
				if (itemposname.length() > 0) {
					std::istringstream posname(itemposname);
					unsigned int itempos = 0;
					posname >> itempos;
					v->set_status(_("Toggling read flag for article..."));
					visible_items[itempos].first->set_unread(!visible_items[itempos].first->unread());
					v->set_status("");
					do_redraw = true;
				}
			}
			break;
		case OP_SETFILTER: {
				char buf[256];
				snprintf(buf,sizeof(buf), "{hbox[lastline] .expand:0 {label .expand:0 text:\"%s\"}{input[filter] modal:1 .expand:h text[filtertext]:\"\"}}", _("Filter: "));
				f->modify("lastline", "replace", buf);
				f->set_focus("filter");
			}
			break;
		case OP_CLEARFILTER:
			apply_filter = false;
			do_redraw = true;
			break;
		default:
			break;
	}
	if (quit) {
		v->pop_current_formaction();
	}
}

void itemlist_formaction::do_update_visible_items() {
	std::vector<rss_item>& items = feed->items();

	if (visible_items.size() > 0)
		visible_items.erase(visible_items.begin(), visible_items.end());

	unsigned int i=0;
	for (std::vector<rss_item>::iterator it = items.begin(); it != items.end(); ++it, ++i) {
		if (!apply_filter || m.matches(&(*it))) {
			visible_items.push_back(std::pair<rss_item *, unsigned int>(&(*it), i));
		}
	}
}

void itemlist_formaction::prepare() {
	if (update_visible_items) {
		do_update_visible_items();
		update_visible_items = false;
	}

	if (do_redraw) {
		std::string code = "{list";

		for (std::vector<std::pair<rss_item *, unsigned int> >::iterator it = visible_items.begin(); it != visible_items.end(); ++it) {
			std::string line = "{listitem[";
			std::ostringstream x;
			x << it->second;
			line.append(x.str());
			line.append("] text:");
			std::string title;
			char buf[20];
			snprintf(buf,sizeof(buf),"%4u ",it->second + 1);
			title.append(buf);
			if (it->first->unread()) {
				title.append("N ");
			} else {
				title.append("  ");
			}
			char datebuf[64];
			time_t t = it->first->pubDate_timestamp();
			struct tm * stm = localtime(&t);
			strftime(datebuf,sizeof(datebuf), "%b %d   ", stm);
			title.append(datebuf);
			title.append(it->first->title());
			GetLogger().log(LOG_DEBUG, "itemlist_formaction: XXXTITLE it->first->title = `%s' title = `%s' quoted title = `%s'", 
				it->first->title().c_str(), title.c_str(), stfl::quote(title).c_str());
			line.append(stfl::quote(title));
			line.append("}");
			code.append(line);
		}

		code.append("}");

		f->modify("items","replace_inner",code);
		
		set_head(feed->title(),feed->unread_item_count(),feed->items().size(), feed->rssurl());

		do_redraw = false;
	}
}

void itemlist_formaction::init() {
	f->set("itempos","0");
	f->set("msg","");
	do_redraw = true;
	set_keymap_hints();
}

void itemlist_formaction::set_head(const std::string& s, unsigned int unread, unsigned int total, const std::string &url) {
	char buf[1024];
	snprintf(buf, sizeof(buf), _("%s %s - Articles in feed '%s' (%u unread, %u total) - %s"), PROGRAM_NAME, PROGRAM_VERSION, s.c_str(), unread, total, url.c_str());
	f->set("head", buf);
}

bool itemlist_formaction::jump_to_next_unread_item(bool start_with_first) {
	unsigned int itempos;
	std::istringstream is(f->get("itempos"));
	is >> itempos;
	for (unsigned int i=(start_with_first?itempos:(itempos+1));i<visible_items.size();++i) {
		GetLogger().log(LOG_DEBUG, "itemlist_formaction::jump_to_next_unread_item: visible_items[%u] unread = %s", i, visible_items[i].first->unread() ? "true" : "false");
		if (visible_items[i].first->unread()) {
			std::ostringstream os;
			os << i;
			f->set("itempos", os.str());
			return true;
		}
	}
	for (unsigned int i=0;i<=itempos;++i) {
		if (visible_items[i].first->unread()) {
			std::ostringstream os;
			os << i;
			f->set("itempos", os.str());
			return true;
		}
	}
	return false;
}

std::string itemlist_formaction::get_guid() {
	unsigned int itempos;
	std::istringstream is(f->get("itempos"));
	is >> itempos;
	return visible_items[itempos].first->guid();
}

keymap_hint_entry * itemlist_formaction::get_keymap_hint() {
	static keymap_hint_entry hints[] = {
		{ OP_QUIT, _("Quit") },
		{ OP_OPEN, _("Open") },
		{ OP_SAVE, _("Save") },
		{ OP_RELOAD, _("Reload") },
		{ OP_NEXTUNREAD, _("Next Unread") },
		{ OP_MARKFEEDREAD, _("Mark All Read") },
		{ OP_SEARCH, _("Search") },
		{ OP_HELP, _("Help") },
		{ OP_NIL, NULL }
	};
	return hints;
}

void itemlist_formaction::handle_cmdline(const std::string& cmd) {
	unsigned int idx = 0;
	if (1==sscanf(cmd.c_str(),"%u",&idx)) {
		if (idx > 0 && idx <= visible_items[visible_items.size()-1].second + 1) {
			int i = get_pos(idx - 1);
			if (i == -1) {
				v->show_error(_("Position not visible!"));
			} else {
				std::ostringstream idxstr;
				idxstr << i;
				f->set("itempos", idxstr.str());
			}
		} else {
			v->show_error(_("Invalid position!"));
		}
	} else {
		// hand over all other commands to formaction
		formaction::handle_cmdline(cmd);
	}
}

int itemlist_formaction::get_pos(unsigned int realidx) {
	for (unsigned int i=0;i<visible_items.size();++i) {
		if (visible_items[i].second == realidx)
			return i;
	}
	return -1;
}


}

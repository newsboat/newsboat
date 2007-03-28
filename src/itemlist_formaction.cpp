#include <itemlist_formaction.h>
#include <view.h>
#include <config.h>
#include <logger.h>

#include <sstream>

namespace newsbeuter {

itemlist_formaction::itemlist_formaction(view * vv, std::string formstr)
	: formaction(vv,formstr), feed(0), show_no_unread_error(false) { 
}

itemlist_formaction::~itemlist_formaction() { }

void itemlist_formaction::process_operation(operation op) {
	bool quit = false;
	std::vector<rss_item>& items = feed->items();
	switch (op) {
		case OP_OPEN: {
				bool open_next_item = false;
				do {
					std::string itemposname = f->get("itempos");
					GetLogger().log(LOG_INFO, "itemlist_formaction: opening item at pos `%s' open_next_item = %d", itemposname.c_str(), open_next_item);
					if (itemposname.length() > 0) {
						std::istringstream posname(itemposname);
						unsigned int pos = 0;
						posname >> pos;
						open_next_item = false; // v->get_ctrl()->open_item(*feed, items[pos].guid());
						v->push_itemview(feed, items[pos].guid());
						do_redraw = true;
					} else {
						v->show_error(_("No item selected!")); // should not happen
					}
					if (open_next_item) {
						/*
						if (!jump_to_next_unread_item(items, true)) {
							open_next_item = false;
							quit = true;
						}
						*/
					}
				} while (open_next_item);
			}
			break;
		case OP_SAVE: 
			{
				char buf[1024];
				std::string itemposname = f->get("itempos");
				GetLogger().log(LOG_INFO, "itemlist_formaction: saving item at pos `%s'", itemposname.c_str());
				if (itemposname.length() > 0) {
					std::istringstream posname(itemposname);
					unsigned int pos = 0;
					posname >> pos;
					
					std::string filename = v->run_filebrowser(FBT_SAVE,v->get_filename_suggestion(items[pos].title()));
					if (filename == "") {
						v->show_error(_("Aborted saving."));
					} else {
						try {
							v->write_item(items[pos], filename);
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
			do_redraw = true;
			break;
		case OP_QUIT:
			GetLogger().log(LOG_INFO, "itemlist_formaction: quitting");
			quit = true;
			break;
		case OP_NEXTUNREAD:
			GetLogger().log(LOG_INFO, "itemlist_formaction: jumping to next unread item");
			//if (!jump_to_next_unread_item(items, true))
			//	show_no_unread_error = true;
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
					unsigned int pos = 0;
					posname >> pos;
					v->set_status(_("Toggling read flag for article..."));
					items[pos].set_unread(!items[pos].unread());
					v->set_status("");
					do_redraw = true;
				}
			}
			break;
		default:
			break;
	}
	if (quit) {
		v->pop_current_formaction();
	}
}

void itemlist_formaction::prepare() {
	if (do_redraw) {
		std::vector<rss_item>& items = feed->items();

		std::string code = "{list";

		unsigned int i=0;
		for (std::vector<rss_item>::iterator it = items.begin(); it != items.end(); ++it, ++i) {
			std::string line = "{listitem[";
			std::ostringstream x;
			x << i;
			line.append(x.str());
			line.append("] text:");
			std::string title;
			char buf[20];
			snprintf(buf,sizeof(buf),"%4u ",i+1);
			title.append(buf);
			if (it->unread()) {
				title.append("N ");
			} else {
				title.append("  ");
			}
			char datebuf[64];
			time_t t = it->pubDate_timestamp();
			struct tm * stm = localtime(&t);
			strftime(datebuf,sizeof(datebuf), "%b %d   ", stm);
			title.append(datebuf);
			title.append(it->title());
			GetLogger().log(LOG_DEBUG, "itemlist_formaction: XXXTITLE it->title = `%s' title = `%s' quoted title = `%s'", 
				it->title().c_str(), title.c_str(), stfl::quote(title).c_str());
			line.append(stfl::quote(title));
			line.append("}");
			code.append(line);
		}

		code.append("}");

		f->modify("items","replace_inner",code);
		
		set_head(feed->title(),feed->unread_item_count(),feed->items().size(), feed->rssurl());

		do_redraw = false;

		if (show_no_unread_error) {
			v->show_error(_("No unread items."));
			show_no_unread_error = false;
		}
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
	snprintf(buf, sizeof(buf), _("Articles in feed '%s' (%u unread, %u total) - %s"), s.c_str(), unread, total, url.c_str());
	f->set("head", buf);
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


}

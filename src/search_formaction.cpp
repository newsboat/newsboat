#include <search_formaction.h>
#include <view.h>
#include <config.h>
#include <logger.h>

#include <sstream>

namespace newsbeuter {

search_formaction::search_formaction(view * vv, std::string formstr)
	: formaction(vv, formstr), quit(false), set_listfocus(false) { }

search_formaction::~search_formaction() { }

void search_formaction::process_operation(operation op) {
	switch (op) {
		case OP_OPEN: {
				std::string querytext = f->get("querytext");
				std::string focus = f->get_focus();
				if (focus == "query") {
					if (querytext.length() > 0) {
						items = v->get_ctrl()->search_for_items(querytext, feedurl);
						if (items.size() > 0) {
							char buf[1024];
							f->set("listpos", "0");
							snprintf(buf, sizeof(buf), _("Search Articles - %u results"), items.size());
							f->set("head", buf);
							set_listfocus = true;
							do_redraw = true;
						} else {
							v->show_error(_("No results."));
						}
					} else {
						quit = true;
					}
				} else {
					std::string itemposname = f->get("listpos");
					GetLogger().log(LOG_INFO, "view::run_search: opening item at pos `%s'", itemposname.c_str());
					if (itemposname.length() > 0) {
						std::istringstream posname(itemposname);
						unsigned int pos = 0;
						posname >> pos;
						rss_feed * tmpfeed = v->get_ctrl()->get_feed_by_url(items[pos].feedurl());
						if (tmpfeed) {
							v->push_itemview(tmpfeed, items[pos].guid());
						} else {
							v->show_error(_("Weird. I just found an item that belongs to a nonexistent feed. That's a bug."));
						}
						do_redraw = true;
					} else {
						v->show_error(_("No item selected!")); // should not happen
					}
				}
			}
			break;
		case OP_SEARCH:
			f->set_focus("query");
			break;
		case OP_QUIT:
			quit = true;
			break;
		case OP_HELP:
			v->push_help();
			v->set_status("");
			break;
		default:
			break;
	}
	if (quit) {
		v->pop_current_formaction();
	}
}

void search_formaction::prepare() {
	if (do_redraw) {
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
			line.append(stfl::quote(title));
			line.append("}");
			code.append(line);
		}

		code.append("}");

		f->modify("results","replace_inner",code);

		if (set_listfocus) {
			f->run(-1);
			f->set_focus("results");
			GetLogger().log(LOG_DEBUG, "view::run_search: setting focus to results");
			set_listfocus = false;
		}

		do_redraw = false;
	}
}

void search_formaction::init() {
	f->set("msg","");
	f->set("head",_("Search Articles"));
	f->set("searchprompt",_("Search for: "));
	f->modify("results","replace_inner","{list}");
	f->set_focus("query");
	do_redraw = true;
	quit = false;
	items.erase(items.begin(), items.end());
}

keymap_hint_entry * search_formaction::get_keymap_hint() {
	static keymap_hint_entry hints[] = {
		{ OP_QUIT, _("Quit") },
		{ OP_OPEN, _("Search/Open") },
		{ OP_SEARCH, _("New Search") },
		{ OP_HELP, _("Help") },
		{ OP_NIL, NULL }
	};
	return hints;
}


}

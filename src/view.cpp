#include <feedlist.h>
#include <itemlist.h>
#include <itemview.h>
#include <help.h>
#include <iostream>
#include <assert.h>

extern "C" {
#include <stfl.h>
}

#include <view.h>
#include <rss.h>
#include <htmlrenderer.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <sstream>

using namespace noos;

view::view(controller * c) : ctrl(c), cfg(0), keys(0) { 
	feedlist_form = stfl_create(feedlist_str);
	itemlist_form = stfl_create(itemlist_str);
	itemview_form = stfl_create(itemview_str);
	help_form = stfl_create(help_str);
}

view::~view() {
	stfl_reset();
	stfl_free(feedlist_form);
	stfl_free(itemlist_form);
	stfl_free(itemview_form);
}

void view::set_config_container(configcontainer * cfgcontainer) {
	cfg = cfgcontainer;	
}

void view::set_keymap(keymap * k) {
	keys = k;
}

void view::feedlist_status(const char * msg) {
	stfl_set(feedlist_form,"msg",msg);
	stfl_run(feedlist_form,-1);
}

void view::itemlist_status(const char * msg) {
	stfl_set(itemlist_form,"msg",msg);
	stfl_run(itemlist_form,-1);
}

void view::itemview_status(const char * msg) {
	stfl_set(itemview_form,"msg",msg);
	stfl_run(itemview_form,-1);
}

void view::feedlist_error(const char * msg) {
	feedlist_status(msg);
	::sleep(2);
	feedlist_status("");
}

void view::itemlist_error(const char * msg) {
	itemlist_status(msg);
	::sleep(2);
	itemlist_status("");
}

void view::itemview_error(const char * msg) {
	itemview_status(msg);
	::sleep(2);
	itemview_status("");
}

void view::run_feedlist() {
	bool quit = false;
	bool update = false;
	
	set_feedlist_keymap_hint();

	do {

		if (update) {
			update = false;
			ctrl->update_feedlist();
		}

		const char * event = stfl_run(feedlist_form,0);
		if (!event) continue;

		operation op = keys->get_operation(event);

		switch (op) {
			case OP_OPEN: {
					const char * feedposname = stfl_get(feedlist_form, "feedposname");
					if (feedposname) {
						std::istringstream posname(feedposname);
						unsigned int pos = 0;
						posname >> pos;
						ctrl->open_feed(pos);
					} else {
						feedlist_error("Error: no feed selected!"); // should not happen
					}
				}
				break;
			case OP_RELOAD: {
					const char * feedposname = stfl_get(feedlist_form, "feedposname");
					if (feedposname) {
						std::istringstream posname(feedposname);
						unsigned int pos = 0;
						posname >> pos;
						ctrl->reload(pos);
					} else {
						feedlist_error("Error: no feed selected!"); // should not happen
					}
				}
				break;
			case OP_RELOADALL:
				ctrl->reload_all();
				break;
			case OP_MARKFEEDREAD: {
					const char * feedposname = stfl_get(feedlist_form, "feedposname");
					if (feedposname) {
						std::istringstream posname(feedposname);
						unsigned int pos = 0;
						posname >> pos;
						ctrl->mark_all_read(pos);
						update = true;
					} else {
						feedlist_error("Error: no feed selected!"); // should not happen
					}
				}
				break;
			case OP_MARKALLFEEDSREAD:
				ctrl->catchup_all();
				update = true;
				break;
			case OP_QUIT:
				quit = true;
				break;
			case OP_HELP:
				run_help();
				break;
			default:
				break;
		}
	} while (!quit);

	stfl_reset();
}

void view::run_itemlist(rss_feed& feed) {
	bool quit = false;
	bool rebuild_list = true;
	bool show_no_unread_error = false;
	std::vector<rss_item>& items = feed.items();

	stfl_set(itemlist_form,"itempos","0");
	
	set_itemlist_keymap_hint();
	set_itemlist_head(feed.title());

	do {
		if (rebuild_list) {

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
				title.append(it->title());
				line.append(stfl_quote(title.c_str()));
				line.append("}");
				code.append(line);
			}

			code.append("}");

			stfl_modify(itemlist_form,"items","replace_inner",code.c_str());

			rebuild_list = false;
		}
		
		if (show_no_unread_error) {
			itemlist_error("No unread items.");
			show_no_unread_error = false;
		}

		const char * event = stfl_run(itemlist_form,0);
		if (!event) continue;

		operation op = keys->get_operation(event);

		switch (op) {
			case OP_OPEN: {
					bool open_next_item = false;
					do {
						const char * itemposname = stfl_get(itemlist_form, "itempos");
						if (itemposname) {
							std::istringstream posname(itemposname);
							unsigned int pos = 0;
							posname >> pos;
							open_next_item = ctrl->open_item(items[pos]);
							rebuild_list = true;
						} else {
							itemlist_error("Error: no item selected!"); // should not happen
						}
						if (open_next_item) {
							if (!jump_to_next_unread_item(items)) {
								open_next_item = false;
								show_no_unread_error = true;
							}
						}
					} while (open_next_item);
				}
				break;
			case OP_SAVE:
				// TODO: save currently selected article
				break;
			case OP_HELP:
				run_help();
				break;
			case OP_QUIT:
				quit = true;
				break;
			case OP_NEXTUNREAD:
				if (!jump_to_next_unread_item(items))
					show_no_unread_error = true;
				break;
			case OP_MARKFEEDREAD:
				mark_all_read(items);
				rebuild_list = true;
				break;
			default:
				break;
		}

	} while (!quit);
}

bool view::jump_to_next_unread_item(std::vector<rss_item>& items) {
	const char * itemposname = stfl_get(itemlist_form, "itemposname");

	if (itemposname) {
		std::istringstream posname(itemposname);
		unsigned int pos = 0;
		posname >> pos;
		for (unsigned int i=pos;i<items.size();++i) {
			if (items[i].unread()) {
				std::ostringstream posname;
				posname << i;
				stfl_set(itemlist_form,"itempos",posname.str().c_str());
				// std::cerr << "setting itemposname to " << posname.str().c_str() << std::endl;
				return true;
			}
		}
		for (unsigned int i=0;i<pos;++i) {
			if (items[i].unread()) {
				std::ostringstream posname;
				posname << i;
				stfl_set(itemlist_form,"itempos",posname.str().c_str());
				// std::cerr << "setting itemposname to " << posname.str().c_str() << std::endl;
				return true;
			}
		}
	} else {
		itemlist_error("Error: no item selected!"); // shouldn't happen
	}
	return false;
}

bool view::run_itemview(rss_item& item) {
	bool quit = false;
	bool retval = false;
	
	set_itemview_keymap_hint();

	std::string code = "{list";

	code.append("{listitem text:");
	std::ostringstream title;
	title << "Title: ";
	title << item.title();
	code.append(stfl_quote(title.str().c_str()));
	code.append("}");

	code.append("{listitem text:");
	std::ostringstream author;
	author << "Author: ";
	author << item.author();
	code.append(stfl_quote(author.str().c_str()));
	code.append("}");

	code.append("{listitem text:");
	std::ostringstream link;
	link << "Link: ";
	link << item.link();
	code.append(stfl_quote(link.str().c_str()));
	code.append("}");
	
	code.append("{listitem text:");
	std::ostringstream date;
	date << "Date: ";
	date << item.pubDate();
	code.append(stfl_quote(date.str().c_str()));
	code.append("}");

	code.append("{listitem text:\"\"}");
	
	set_itemview_head(item.title());
	
	stfl_run(itemview_form,-1); // XXX HACK: render once so that we get a proper widget width
	const char * widthstr = stfl_get(itemview_form,"article:w");
	unsigned int render_width = 80;
	if (widthstr) {
		std::istringstream is(widthstr);
		is >> render_width;
		if (render_width - 5 > 0)
	  		render_width -= 5; 	
	}

	htmlrenderer rnd(render_width);

	// const char * desc = item.description().c_str();
	// std::cerr << desc << std::endl;
	std::vector<std::string> lines;
	// std::cerr << "`" << item.description() << "'" << std::endl;
	rnd.render(item.description(), lines);

	for (std::vector<std::string>::iterator it = lines.begin(); it != lines.end(); ++it) {
		std::string line = std::string("{listitem text:") + std::string(stfl_quote(it->c_str())) + std::string("}");
		code.append(line);
	}

	code.append("}");

	stfl_modify(itemview_form,"article","replace_inner",code.c_str());

	stfl_set(itemview_form,"articleoffset","0");

	do {
		const char * event = stfl_run(itemview_form,0);
		if (!event) continue;

		operation op = keys->get_operation(event);

		switch (op) {
			case OP_OPEN:
				// nothing
				break;
			case OP_SAVE:
				// TODO: save currently selected article
				break;
			case OP_OPENINBROWSER:
				itemview_status("Starting browser...");
				open_in_browser(item.link());
				itemview_status("");
				break;
			case OP_NEXTUNREAD:
				retval = true;
			case OP_QUIT:
				quit = true;
				break;
			case OP_HELP:
				run_help();
				break;
			default:
				break;
		}

	} while (!quit);

	return retval;
}

void view::open_in_browser(const std::string& url) {
	std::string cmdline;
	std::string browser = cfg->get_configvalue("browser");
	if (browser != "")
		cmdline.append(browser);
	else
		cmdline.append("lynx");
	cmdline.append(" '");
	cmdline.append(url);
	cmdline.append("'");
	stfl_reset();
	::system(cmdline.c_str());
}

void view::run_help() {
	set_itemlist_keymap_hint();
	
	std::vector<std::pair<std::string,std::string> > descs;
	keys->get_keymap_descriptions(descs);
	
	std::string code = "{list";
	
	for (std::vector<std::pair<std::string,std::string> >::iterator it=descs.begin();it!=descs.end();++it) {
		std::string line = "{listitem text:";
		std::string descline = it->first + std::string("\t") + it->second;
		line.append(stfl_quote(descline.c_str()));
		line.append("}");
		
		code.append(line);
	}
	
	code.append("}");
	
	stfl_modify(help_form,"helptext","replace_inner",code.c_str());
	
	bool quit = false;
	
	do {
		const char * event = stfl_run(help_form,0);
		if (!event) continue;

		operation op = keys->get_operation(event);

		switch (op) {
			case OP_QUIT:
				quit = true;
				break;
			default:
				break;
		}
	} while (!quit);
}

void view::set_feedlist(std::vector<rss_feed>& feeds) {
	std::string code = "{list";
	
	assert(cfg != NULL); // must not happen
	
	bool show_read_feeds = cfg->get_configvalue_as_bool("show-read-feeds");
	
	// std::cerr << "show-read-feeds" << (show_read_feeds?"true":"false") << std::endl;

	unsigned int i = 0;
	unsigned short feedlist_number = 1;
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
			for (std::vector<rss_item>::iterator rit = it->items().begin(); rit != it->items().end(); ++rit) {
				if (rit->unread())
					++unread_count;
			}
		}

		if (show_read_feeds || unread_count > 0) {
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
			line.append(stfl_quote(title.c_str()));
			line.append("}");

			code.append(line);
		}
	}

	code.append("}");

	stfl_modify(feedlist_form,"feeds","replace_inner",code.c_str());
}

void view::mark_all_read(std::vector<rss_item>& items) {
	for (std::vector<rss_item>::iterator it = items.begin(); it != items.end(); ++it) {
		it->set_unread(false);
	}
}

struct keymap_hint_entry {
	operation op; 
	char * text;
};

std::string view::prepare_keymaphint(keymap_hint_entry * hints) {
	std::string keymap_hint;
	for (int i=0;hints[i].op != OP_NIL; ++i) {
		keymap_hint.append(keys->getkey(hints[i].op));
		keymap_hint.append(":");
		keymap_hint.append(hints[i].text);
		keymap_hint.append(" ");
	}
	return keymap_hint;	
}

void view::set_itemview_keymap_hint() {
	keymap_hint_entry hints[] = {
		{ OP_QUIT, "Quit" },
		{ OP_OPEN, "Open" },
		{ OP_SAVE, "Save" },
		{ OP_NEXTUNREAD, "Next Unread" },
		{ OP_MARKFEEDREAD, "Mark All Read" },
		{ OP_HELP, "Help" },
		{ OP_NIL, NULL }
	};
	std::string keymap_hint = prepare_keymaphint(hints);
	stfl_set(itemview_form,"help", keymap_hint.c_str());
}

void view::set_feedlist_keymap_hint() {
	keymap_hint_entry hints[] = {
		{ OP_QUIT, "Quit" },
		{ OP_OPEN, "Open" },
		{ OP_RELOAD, "Reload" },
		{ OP_RELOADALL, "Reload All" },
		{ OP_MARKFEEDREAD, "Mark Read" },
		{ OP_MARKALLFEEDSREAD, "Catchup All" },
		{ OP_HELP, "Help" },
		{ OP_NIL, NULL }
	};
	std::string keymap_hint = prepare_keymaphint(hints);
	stfl_set(feedlist_form,"help", keymap_hint.c_str());
}

void view::set_itemlist_keymap_hint() {
	keymap_hint_entry hints[] = {
		{ OP_QUIT, "Quit" },
		{ OP_OPEN, "Open" },
		{ OP_SAVE, "Save" },
		{ OP_NEXTUNREAD, "Next Unread" },
		{ OP_OPENINBROWSER, "Open in Browser" },
		{ OP_HELP, "Help" },
		{ OP_NIL, NULL }
	};
	std::string keymap_hint = prepare_keymaphint(hints);
	stfl_set(itemlist_form,"help", keymap_hint.c_str());
}

void view::set_help_keymap_hint() {
	keymap_hint_entry hints[] = {
		{ OP_QUIT, "Quit" },
		{ OP_NIL, NULL }
	};
	std::string keymap_hint = prepare_keymaphint(hints);
	stfl_set(help_form,"help", keymap_hint.c_str());	
}

void view::set_itemlist_head(const std::string& s) {
	std::string caption = "Articles in feed '";
	caption.append(s);
	caption.append("'");
	stfl_set(itemlist_form,"head",caption.c_str());	
}

void view::set_itemview_head(const std::string& s) {
	std::string caption = "Article '";
	caption.append(s);
	caption.append("'");
	stfl_set(itemview_form,"head",caption.c_str());		
}

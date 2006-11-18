#include <feedlist.h>
#include <itemlist.h>
#include <itemview.h>

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

view::view(controller * c) : ctrl(c) { 
	feedlist_form = stfl_create(feedlist_str);
	itemlist_form = stfl_create(itemlist_str);
	itemview_form = stfl_create(itemview_str);
}

view::~view() {
	stfl_reset();
	stfl_free(feedlist_form);
	stfl_free(itemlist_form);
	stfl_free(itemview_form);
}

void view::feedlist_status(const char * msg) {
	stfl_set(feedlist_form,"msg",msg);
	stfl_run(feedlist_form,-1);
}

void view::itemlist_status(const char * msg) {
	stfl_set(itemlist_form,"msg",msg);
	stfl_run(itemlist_form,-1);
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

void view::run_feedlist() {
	bool quit = false;
	do {
		const char * event = stfl_run(feedlist_form,0);
		if (!event) continue;

		if (strcmp(event,"ENTER")==0) {
			const char * feedposname = stfl_get(feedlist_form, "feedposname");
			if (feedposname) {
				std::istringstream posname(feedposname);
				unsigned int pos = 0;
				posname >> pos;
				ctrl->open_feed(pos);
			} else {
				feedlist_error("Error: no feed selected!"); // should not happen
			}
		} else if (strncmp(event,"CHAR(",5)==0) {

			unsigned int x;
			char c;
			sscanf(event,"CHAR(%d)",&x); // XXX: refactor

			c = static_cast<char>(x);

			switch (c) {
				case 'r': {
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
				case 'R':
					ctrl->reload_all();
					break;
				case 'q':
					quit = true;
					break;
				default:
					break;
			}

		}
	} while (!quit);
}

void view::run_itemlist(rss_feed& feed) {
	bool quit = false;
	bool rebuild_list = true;
	std::vector<rss_item>& items = feed.items();

	stfl_set(itemlist_form,"itempos","0");

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
				std::string title = " ";
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

		const char * event = stfl_run(itemlist_form,0);
		if (!event) continue;

		if (strcmp(event,"ENTER")==0) {
			const char * itemposname = stfl_get(itemlist_form, "itemposname");
			if (itemposname) {
				std::istringstream posname(itemposname);
				unsigned int pos = 0;
				posname >> pos;
				ctrl->open_item(items[pos]);
				rebuild_list = true;
			} else {
				itemlist_error("Error: no item selected!"); // should not happen
			}
		} else if (strncmp(event,"CHAR(",5)==0) {

			unsigned int x;
			char c;
			sscanf(event,"CHAR(%d)",&x); // XXX: refactor

			c = static_cast<char>(x);

			switch (c) {
				case 's':
					// TODO: save currently selected article
					break;
				case 'q':
					quit = true;
					break;
			}
		}

	} while (!quit);
}

void view::run_itemview(rss_item& item) {
	bool quit = false;

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

	code.append("{listitem text:\"\"}");

	htmlrenderer rnd;

	std::vector<std::string> lines = rnd.render(item.description());

	for (std::vector<std::string>::iterator it = lines.begin(); it != lines.end(); ++it) {
		std::string line = std::string("{listitem text:") + std::string(stfl_quote(it->c_str())) + std::string("}");
		code.append(line);
	}

	code.append("}");

	stfl_modify(itemview_form,"article","replace_inner",code.c_str());

	do {
		const char * event = stfl_run(itemview_form,0);
		if (!event) continue;

		if (strcmp(event,"ENTER")==0) {
			// nothing
		} else if (strncmp(event,"CHAR(",5)==0) {

			unsigned int x;
			char c;
			sscanf(event,"CHAR(%d)",&x); // XXX: refactor

			c = static_cast<char>(x);

			switch (c) {
				case 's':
					// TODO: save currently selected article
					break;
				case 'q':
					quit = true;
					break;
			}
		}


	} while (!quit);
}

void view::set_feedlist(std::vector<rss_feed>& feeds) {
	std::string code = "{list";

	unsigned int i = 0;
	for (std::vector<rss_feed>::iterator it = feeds.begin(); it != feeds.end(); ++it, ++i) {
		std::string title = it->title();
		if (title == "") {
			title = it->rssurl(); // rssurl must always be present.
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
		snprintf(buf,sizeof(buf),"(%u/%u) ",unread_count,it->items().size());
		snprintf(buf2,sizeof(buf2),"%14s",buf);
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

	code.append("}");

	// std::cerr << code << std::endl;

	stfl_modify(feedlist_form,"feeds","replace_inner",code.c_str());
}

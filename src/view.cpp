#include "feedlist.h"
#include "itemlist.h"
#include "itemview.h"

extern "C" {
#include <stfl.h>
}

#include <view.h>
#include <rss.h>
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

void view::feedlist_status(char * msg) {
	stfl_set(feedlist_form,"msg",msg);
	stfl_run(feedlist_form,-1);
}

void view::feedlist_error(char * msg) {
	feedlist_status(msg);
	::sleep(2);
	feedlist_status("");
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
				case 'r':
					// ctrl->reload();
					break;
				case 'R':
					// ctrl->reload_all();
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

	std::vector<rss_item>& items = feed.items();

	std::string code = "{list";

	unsigned int i=0;
	for (std::vector<rss_item>::iterator it = items.begin(); it != items.end(); ++it, ++i) {
		std::string line = "{listitem[";
		std::ostringstream x;
		x << i;
		line.append(x.str());
		line.append("] text:");
		std::string title = it->title();
		line.append(stfl_quote(title.c_str()));
		line.append("}");
		code.append(line);
	}

	code.append("}");

	stfl_modify(itemlist_form,"items","replace_inner",code.c_str());

	do {
		const char * event = stfl_run(itemlist_form,0);
		if (!event) continue;

		if (strcmp(event,"ENTER")==0) {
			// c->open_item();
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

void view::run_itemview() {

}

void view::set_feedlist(const std::vector<std::string>& feeds) {
	std::string code = "{list";

	unsigned int i = 0;
	for (std::vector<std::string>::const_iterator it = feeds.begin(); it != feeds.end(); ++it, ++i) {
		std::string line = "{listitem[";
		std::ostringstream num;
		num << i;
		line.append(num.str());
		line.append("] text:");
		line.append(stfl_quote(it->c_str()));
		line.append("}");

		code.append(line);
	}

	code.append("}");

	// std::cerr << code << std::endl;

	stfl_modify(feedlist_form,"feeds","replace_inner",code.c_str());
}

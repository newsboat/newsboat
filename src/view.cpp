#include "feedlist.h"
#include "itemlist.h"
#include "itemview.h"

extern "C" {
#include <stfl.h>
}

#include <view.h>
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

void view::run_feedlist() {
	bool quit = false;
	do {
		const char * event = stfl_run(feedlist_form,0);
		if (!event) continue;

		if (strcmp(event,"ENTER")==0) {
			// ctrl->open_feed();
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

void view::run_itemlist() {

}

void view::run_itemview() {

}

void view::set_feedlist(const std::vector<std::string>& feeds) {
	std::string code = "{list";

	unsigned int i = 0;
	for (std::vector<std::string>::const_iterator it = feeds.begin(); it != feeds.end(); ++it) {
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

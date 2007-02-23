#ifndef PODBEUTER_VIEW__H
#define PODBEUTER_VIEW__H

#include <view.h>
#include <stflpp.h>

#include <keymap.h>
#include <stflpp.h>

namespace podbeuter {

class pb_controller;

class pb_view {
	public:
		pb_view(pb_controller * c = 0);
		~pb_view();
		void run();
		void set_keymap(newsbeuter::keymap * k) { keys = k; }
	private:
		pb_controller * ctrl;
		newsbeuter::stfl::form dllist_form;
		newsbeuter::keymap * keys;
};

}

#endif

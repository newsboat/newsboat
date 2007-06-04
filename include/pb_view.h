#ifndef PODBEUTER_VIEW__H
#define PODBEUTER_VIEW__H

#include <keymap.h>
#include <stflpp.h>
#include <colormanager.h>

using namespace newsbeuter;

namespace podbeuter {

class pb_controller;

class newsbeuter::colormanager;

struct keymap_hint_entry;

class pb_view {
	public:
		pb_view(pb_controller * c = 0);
		~pb_view();
		void run();
		void set_keymap(newsbeuter::keymap * k) { 
			keys = k; 
			set_bindings();
		}

	private:

		friend class newsbeuter::colormanager;

		struct keymap_hint_entry {
			operation op; 
			char * text;
		};

		void run_help();
		void set_dllist_keymap_hint();
		void set_help_keymap_hint();
		void set_bindings();

		std::string prepare_keymaphint(keymap_hint_entry * hints);
		pb_controller * ctrl;
		newsbeuter::stfl::form dllist_form;
		newsbeuter::stfl::form help_form;
		newsbeuter::keymap * keys;
};

}

#endif

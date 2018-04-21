#ifndef PODBOAT_VIEW_H_
#define PODBOAT_VIEW_H_

#include "keymap.h"
#include "stflpp.h"
#include "colormanager.h"

using namespace newsboat;

namespace podboat {

class pb_controller;

struct keymap_hint_entry;

class pb_view {
	public:
		explicit pb_view(pb_controller * c = 0);
		~pb_view();
		void run(bool auto_download);
		void set_keymap(newsboat::keymap * k) {
			keys = k;
			set_bindings();
		}

	private:

		friend class newsboat::colormanager;

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
		newsboat::stfl::form dllist_form;
		newsboat::stfl::form help_form;
		newsboat::keymap * keys;
};

}

#endif /* PODBOAT_VIEW_H_ */

#ifndef PODBOAT_VIEW_H_
#define PODBOAT_VIEW_H_

#include "colormanager.h"
#include "keymap.h"
#include "stflpp.h"

using namespace newsboat;

namespace podboat {

class PbController;
class Download;

struct keymap_hint_entry;

class PbView {
public:
	explicit PbView(PbController* c = 0);
	~PbView();
	void run(bool auto_Download);
	void set_keymap(newsboat::Keymap* k)
	{
		keys = k;
		set_bindings();
	}

private:
	friend class newsboat::ColorManager;

	struct keymap_hint_entry {
		operation op;
		char* text;
	};

	void run_help();
	void set_dllist_keymap_hint();
	void set_help_keymap_hint();
	void set_bindings();

	std::string prepare_keymaphint(keymap_hint_entry* hints);
	std::string Format_line(const std::string& podlist_format,
			const Download& dl,
			unsigned int pos,
			unsigned int width);
	PbController* ctrl;
	newsboat::Stfl::Form dllist_form;
	newsboat::Stfl::Form help_form;
	newsboat::Keymap* keys;
};

} // namespace podboat

#endif /* PODBOAT_VIEW_H_ */

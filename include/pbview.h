#ifndef PODBOAT_VIEW_H_
#define PODBOAT_VIEW_H_

#include "colormanager.h"
#include "keymap.h"
#include "stflpp.h"

using namespace newsboat;

namespace podboat {

class PbController;
class Download;

struct KeyMapHintEntry;

class PbView {
public:
	explicit PbView(PbController* c = 0);
	~PbView();
	void run(bool auto_download);
	void set_keymap(newsboat::KeyMap* k)
	{
		keys = k;
		set_bindings();
	}

private:
	friend class newsboat::ColorManager;

	struct KeyMapHintEntry {
		Operation op;
		char* text;
	};

	void run_help();
	void set_dllist_keymap_hint();
	void set_help_keymap_hint();
	void set_bindings();

	std::string prepare_keymaphint(KeyMapHintEntry* hints);
	std::string format_line(const std::string& podlist_format,
			const Download& dl,
			unsigned int pos,
			unsigned int width);
	PbController* ctrl;
	newsboat::Stfl::Form dllist_form;
	newsboat::Stfl::Form help_form;
	newsboat::KeyMap* keys;
};

} // namespace podboat

#endif /* PODBOAT_VIEW_H_ */

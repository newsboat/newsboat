#ifndef PODBOAT_VIEW_H_
#define PODBOAT_VIEW_H_

#include "colormanager.h"
#include "keymap.h"
#include "listwidget.h"
#include "textviewwidget.h"
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
	void run(bool auto_download, bool wrap_scroll);
	void set_keymap(newsboat::KeyMap* k)
	{
		keys = k;
	}
	void apply_colors_to_all_forms();

private:
	struct KeyMapHintEntry {
		Operation op;
		char* text;
	};

	void run_help();
	void set_dllist_keymap_hint();
	void set_help_keymap_hint();
	std::pair<double, std::string> get_speed_human_readable(double kbps);

	std::string prepare_keymaphint(KeyMapHintEntry* hints);
	std::string format_line(const std::string& podlist_format,
		const Download& dl,
		unsigned int pos,
		unsigned int width);
	PbController* ctrl;
	newsboat::Stfl::Form dllist_form;
	newsboat::Stfl::Form help_form;
	newsboat::KeyMap* keys;
	const newsboat::ColorManager& colorman;

	ListWidget downloads_list;
	TextviewWidget help_textview;
};

} // namespace podboat

#endif /* PODBOAT_VIEW_H_ */

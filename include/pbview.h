#ifndef PODBOAT_VIEW_H_
#define PODBOAT_VIEW_H_

#include "colormanager.h"
#include "lineview.h"
#include "listwidget.h"
#include "textviewwidget.h"
#include "stflpp.h"
#include "stflrichtext.h"

namespace newsboat {
class KeyMap;
}

using namespace newsboat;

namespace podboat {

class PbController;
class Download;

class PbView {
public:
	explicit PbView(PbController& c);
	~PbView();
	void run(bool auto_download, bool wrap_scroll);
	void apply_colors_to_all_forms();
	void set_view_update_necessary()
	{
		update_view = true;
	}

private:
	void run_help();
	void set_dllist_keymap_hint();
	void set_help_keymap_hint();
	std::pair<double, std::string> get_speed_human_readable(double kbps);
	void handle_resize();

	StflRichText format_line(const std::string& podlist_format,
		const Download& dl,
		unsigned int pos,
		unsigned int width);

	bool update_view;
	PbController& ctrl;
	newsboat::Stfl::Form dllist_form;
	newsboat::Stfl::Form help_form;
	LineView title_line_dllist_form;
	LineView title_line_help_form;
	LineView msg_line_dllist_form;
	LineView msg_line_help_form;
	newsboat::KeyMap& keys;
	const newsboat::ColorManager& colorman;

	ListWidget downloads_list;
	TextviewWidget help_textview;
};

} // namespace podboat

#endif /* PODBOAT_VIEW_H_ */

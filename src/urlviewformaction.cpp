#include "urlviewformaction.h"

#include <sstream>
#include <string>

#include "config.h"
#include "fmtstrformatter.h"
#include "listformatter.h"
#include "rssfeed.h"
#include "strprintf.h"
#include "utils.h"
#include "view.h"

namespace newsboat {

/*
 * The UrlViewFormAction is probably the simplest dialog of all. It
 * displays a list of URLs, and makes it possible to open the URLs
 * in a browser or to bookmark them.
 */

UrlViewFormAction::UrlViewFormAction(View* vv,
	std::shared_ptr<RssFeed>& feed,
	Utf8String formstr,
	ConfigContainer* cfg)
	: FormAction(vv, formstr, cfg)
	, quit(false)
	, feed(feed)
	, urls_list("urls", FormAction::f, cfg->get_configvalue_as_int("scrolloff"))
{
}

UrlViewFormAction::~UrlViewFormAction() {}

bool UrlViewFormAction::process_operation(Operation op,
	bool /* automatic */,
	std::vector<Utf8String>* /* args */)
{
	bool hardquit = false;
	switch (op) {
	case OP_PREV:
		urls_list.move_up(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_NEXT:
		urls_list.move_down(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_OPENINBROWSER:
	case OP_OPENBROWSER_AND_MARK:
	case OP_OPEN: {
		const bool interactive = true;
		open_current_position_in_browser(interactive);
	}
	break;
	case OP_OPENINBROWSER_NONINTERACTIVE: {
		const bool interactive = false;
		open_current_position_in_browser(interactive);
	}
	break;
	case OP_BOOKMARK: {
		if (!links.empty()) {
			const unsigned int pos = urls_list.get_position();
			this->start_bookmark_qna("", links[pos].first, feed->title());
		} else {
			v->get_statusline().show_error(_("No links available!"));
		}
	}
	break;
	case OP_OPEN_URL_1:
	case OP_OPEN_URL_2:
	case OP_OPEN_URL_3:
	case OP_OPEN_URL_4:
	case OP_OPEN_URL_5:
	case OP_OPEN_URL_6:
	case OP_OPEN_URL_7:
	case OP_OPEN_URL_8:
	case OP_OPEN_URL_9:
	case OP_OPEN_URL_10: {
		unsigned int idx = op - OP_OPEN_URL_1;

		if (idx < links.size()) {
			const auto feedurl = (feed != nullptr ?  feed->rssurl() : "");
			const bool interactive = true;
			v->open_in_browser(links[idx].first, feedurl, utils::link_type_str(links[idx].second),
				interactive);
		}
	}
	break;
	case OP_HELP:
		v->push_help();
		break;
	case OP_QUIT:
		quit = true;
		break;
	case OP_HARDQUIT:
		hardquit = true;
		break;
	default:
		if (handle_list_operations(urls_list, op)) {
			break;
		}
		break;
	}
	if (hardquit) {
		while (v->formaction_stack_size() > 0) {
			v->pop_current_formaction();
		}
	} else if (quit) {
		v->pop_current_formaction();
	}
	return true;
}

void UrlViewFormAction::open_current_position_in_browser(bool interactive)
{
	if (!links.empty()) {
		const unsigned int pos = urls_list.get_position();
		const auto feedurl = (feed != nullptr ?  feed->rssurl() : "");
		v->open_in_browser(links[pos].first, feedurl, utils::link_type_str(links[pos].second),
			interactive);
	} else {
		v->get_statusline().show_error(_("No links available!"));
	}
}

void UrlViewFormAction::prepare()
{
	if (do_redraw) {
		update_heading();

		ListFormatter listfmt;
		unsigned int i = 0;
		for (const auto& link : links) {
			listfmt.add_line(utils::quote_for_stfl(strprintf::fmt("%2u  %s", i + 1,
						link.first)));
			i++;
		}
		urls_list.stfl_replace_lines(listfmt);
	}
}

void UrlViewFormAction::init()
{
	recalculate_widget_dimensions();

	do_redraw = true;
	quit = false;
	set_keymap_hints();
}

void UrlViewFormAction::update_heading()
{
	const unsigned int width = urls_list.get_width();

	FmtStrFormatter fmt;
	fmt.register_fmt('N', PROGRAM_NAME);
	fmt.register_fmt('V', utils::program_version());

	set_value("head",
		fmt.do_format(
			cfg->get_configvalue("urlview-title-format"), width));
}

const std::vector<KeyMapHintEntry>& UrlViewFormAction::get_keymap_hint() const
{
	static const std::vector<KeyMapHintEntry> hints = {{OP_QUIT, _s("Quit")},
		{OP_OPEN, _s("Open in Browser")},
		{OP_BOOKMARK, _s("Save Bookmark")},
		{OP_HELP, _s("Help")}
	};
	return hints;
}

void UrlViewFormAction::handle_cmdline(const Utf8String& cmd)
{
	unsigned int idx = 0;
	if (1 == sscanf(cmd.c_str(), "%u", &idx)) {
		if (idx < 1 || idx > links.size()) {
			v->get_statusline().show_error(_("Invalid position!"));
		} else {
			urls_list.set_position(idx - 1);
		}
	} else {
		FormAction::handle_cmdline(cmd);
	}
}

Utf8String UrlViewFormAction::title()
{
	return _s("URLs");
}

} // namespace newsboat

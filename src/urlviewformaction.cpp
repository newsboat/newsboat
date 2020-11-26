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
	std::string formstr,
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
	std::vector<std::string>* /* args */)
{
	bool hardquit = false;
	switch (op) {
	case OP_PREV:
	case OP_SK_UP:
		urls_list.move_up(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_NEXT:
	case OP_SK_DOWN:
		urls_list.move_down(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_SK_HOME:
		urls_list.move_to_first();
		break;
	case OP_SK_END:
		urls_list.move_to_last();
		break;
	case OP_SK_PGUP:
		urls_list.move_page_up(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_SK_PGDOWN:
		urls_list.move_page_down(cfg->get_configvalue_as_bool("wrap-scroll"));
		break;
	case OP_OPENINBROWSER:
	case OP_OPENBROWSER_AND_MARK:
	case OP_OPEN: {
		if (!links.empty()) {
			const unsigned int pos = urls_list.get_position();
			v->set_status(_("Starting browser..."));
			const std::string feedurl = (feed != nullptr ?
					feed->rssurl() :
					"");
			v->open_in_browser(links[pos].first, feedurl);
			v->set_status("");
		} else {
			v->show_error(_("No links available!"));
		}
	}
	break;
	case OP_BOOKMARK: {
		if (!links.empty()) {
			const unsigned int pos = urls_list.get_position();
			this->start_bookmark_qna("", links[pos].first, "", feed->title());
		} else {
			v->show_error(_("No links available!"));
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
			v->set_status(_("Starting browser..."));
			const std::string feedurl = (feed != nullptr ?
					feed->rssurl() :
					"");
			v->open_in_browser(links[idx].first, feedurl);
			v->set_status("");
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
	default: // nothing
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
	v->set_status("");

	f.run(-3); // compute all widget dimensions

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

	f.set("head",
		fmt.do_format(
			cfg->get_configvalue("urlview-title-format"), width));
}

KeyMapHintEntry* UrlViewFormAction::get_keymap_hint()
{
	static KeyMapHintEntry hints[] = {{OP_QUIT, _("Quit")},
		{OP_OPEN, _("Open in Browser")},
		{OP_BOOKMARK, _("Save Bookmark")},
		{OP_HELP, _("Help")},
		{OP_NIL, nullptr}
	};
	return hints;
}

void UrlViewFormAction::handle_cmdline(const std::string& cmd)
{
	unsigned int idx = 0;
	if (1 == sscanf(cmd.c_str(), "%u", &idx)) {
		if (idx < 1 || idx > links.size()) {
			v->show_error(_("Invalid position!"));
		} else {
			urls_list.set_position(idx - 1);
		}
	} else {
		FormAction::handle_cmdline(cmd);
	}
}

std::string UrlViewFormAction::title()
{
	return _("URLs");
}

} // namespace newsboat

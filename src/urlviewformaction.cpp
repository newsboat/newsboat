#include "urlviewformaction.h"

#include <sstream>

#include "config.h"
#include "formatstring.h"
#include "listformatter.h"
#include "strprintf.h"
#include "utils.h"
#include "view.h"

namespace newsboat {

/*
 * The UrlViewFormaction is probably the simplest dialog of all. It
 * displays a list of URLs, and makes it possible to open the URLs
 * in a browser or to bookmark them.
 */

UrlViewFormaction::UrlViewFormaction(View* vv,
	std::shared_ptr<RssFeed>& feed,
	std::string formstr)
	: Formaction(vv, formstr)
	, quit(false)
	, feed(feed)
{
}

UrlViewFormaction::~UrlViewFormaction() {}

void UrlViewFormaction::process_operation(operation op,
	bool /* automatic */,
	std::vector<std::string>* /* args */)
{
	bool hardquit = false;
	switch (op) {
	case OP_OPENINBROWSER:
	case OP_OPEN: {
		std::string posstr = f->get("feedpos");
		if (posstr.length() > 0) {
			unsigned int idx = Utils::to_u(posstr, 0);
			v->set_status(_("Starting browser..."));
			v->open_in_browser(links[idx].first);
			v->set_status("");
		} else {
			v->show_error(_("No link selected!"));
		}
	} break;
	case OP_BOOKMARK: {
		std::string posstr = f->get("feedpos");
		if (posstr.length() > 0) {
			unsigned int idx = Utils::to_u(posstr, 0);

			this->start_bookmark_qna(
				"", links[idx].first, "", feed->title());

		} else {
			v->show_error(_("No link selected!"));
		}
	} break;
	case OP_1:
	case OP_2:
	case OP_3:
	case OP_4:
	case OP_5:
	case OP_6:
	case OP_7:
	case OP_8:
	case OP_9:
	case OP_0: {
		unsigned int idx = op - OP_1;

		if (idx < links.size()) {
			v->set_status(_("Starting browser..."));
			v->open_in_browser(links[idx].first);
			v->set_status("");
		}
	} break;
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
		while (v->Formaction_stack_size() > 0) {
			v->pop_current_formaction();
		}
	} else if (quit) {
		v->pop_current_formaction();
	}
}

void UrlViewFormaction::prepare()
{
	if (do_redraw) {
		ListFormatter listfmt;
		unsigned int i = 0;
		for (const auto& link : links) {
			listfmt.add_line(
				StrPrintf::fmt("%2u  %s", i + 1, link.first),
				i);
			i++;
		}
		f->modify("urls", "replace_inner", listfmt.format_list());
	}
}

void UrlViewFormaction::init()
{
	v->set_status("");

	std::string Viewwidth = f->get("urls:w");
	unsigned int width = Utils::to_u(Viewwidth, 80);

	FmtStrFormatter fmt;
	fmt.register_fmt('N', PROGRAM_NAME);
	fmt.register_fmt('V', PROGRAM_VERSION);

	f->set("head",
		fmt.do_format(
			v->get_cfg()->get_configvalue("urlView-title-Format"),
			width));
	do_redraw = true;
	quit = false;
	set_keymap_hints();
}

keymap_hint_entry* UrlViewFormaction::get_keymap_hint()
{
	static keymap_hint_entry hints[] = {{OP_QUIT, _("Quit")},
		{OP_OPEN, _("Open in Browser")},
		{OP_BOOKMARK, _("Save Bookmark")},
		{OP_NIL, nullptr}};
	return hints;
}

void UrlViewFormaction::handle_cmdline(const std::string& cmd)
{
	unsigned int idx = 0;
	if (1 == sscanf(cmd.c_str(), "%u", &idx)) {
		if (idx < 1 || idx > links.size()) {
			v->show_error(_("Invalid position!"));
		} else {
			f->set("feedpos", std::to_string(idx - 1));
		}
	} else {
		Formaction::handle_cmdline(cmd);
	}
}

std::string UrlViewFormaction::title()
{
	return _("URLs");
}

} // namespace newsboat

#include "pbview.h"

#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <curses.h>
#include <iostream>
#include <sstream>

#include "config.h"
#include "configcontainer.h"
#include "dllist.h"
#include "download.h"
#include "fmtstrformatter.h"
#include "help.h"
#include "listformatter.h"
#include "logger.h"
#include "pbcontroller.h"
#include "poddlthread.h"
#include "strprintf.h"
#include "utils.h"

using namespace newsboat;

namespace podboat {

PbView::PbView(PbController* c)
	: ctrl(c)
	, dllist_form(dllist_str)
	, help_form(help_str)
	, keys(0)
	, downloads_list("dls", dllist_form)
	, help_textview("helptext", help_form)
{
	if (getenv("ESCDELAY") == nullptr) {
		set_escdelay(25);
	}
}

PbView::~PbView()
{
	Stfl::reset();
}

void PbView::run(bool auto_download, bool wrap_scroll)
{
	bool quit = false;

	// Make sure curses is initialized
	dllist_form.run(-3);
	// Hide cursor using curses
	curs_set(0);

	set_dllist_keymap_hint();

	do {
		if (ctrl->view_update_necessary()) {
			const double total_kbps = ctrl->get_total_kbps();
			const auto speed = get_speed_human_readable(total_kbps);

			char parbuf[128] = "";
			if (ctrl->get_maxdownloads() > 1) {
				snprintf(parbuf,
					sizeof(parbuf),
					_(" - %u parallel downloads"),
					ctrl->get_maxdownloads());
			}

			char buf[1024];
			snprintf(buf,
				sizeof(buf),
				_("Queue (%u downloads in progress, %u total) "
					"- %.2f %s total%s"),
				static_cast<unsigned int>(
					ctrl->downloads_in_progress()),
				static_cast<unsigned int>(
					ctrl->downloads().size()),
				speed.first,
				speed.second.c_str(),
				parbuf);

			dllist_form.set("head", buf);

			LOG(Level::DEBUG,
				"PbView::run: updating view... "
				"downloads().size() "
				"= %" PRIu64,
				static_cast<uint64_t>(ctrl->downloads().size()));

			ListFormatter listfmt;
			std::string formatstring = ctrl->get_formatstr();

			dllist_form.run(-3); // compute all widget dimensions
			unsigned int width = utils::to_u(dllist_form.get("dls:w"));

			unsigned int i = 0;
			for (const auto& dl : ctrl->downloads()) {
				auto lbuf = format_line(formatstring, dl, i, width);
				listfmt.add_line(lbuf, std::to_string(i));
				i++;
			}

			downloads_list.stfl_replace_lines(listfmt);

			ctrl->set_view_update_necessary(false);
		}

		const char* event = dllist_form.run(500);

		if (auto_download) {
			if (ctrl->get_maxdownloads() >
				ctrl->downloads_in_progress()) {
				ctrl->start_downloads();
			}
		}

		if (!event || strcmp(event, "TIMEOUT") == 0) {
			continue;
		}

		Operation op = keys->get_operation(event, "podboat");

		if (dllist_form.get("msg").length() > 0) {
			dllist_form.set("msg", "");
			ctrl->set_view_update_necessary(true);
		}

		switch (op) {
		case OP_REDRAW:
			Stfl::reset();
			break;
		case OP_SK_UP:
			downloads_list.move_up(wrap_scroll);
			break;
		case OP_SK_DOWN:
			downloads_list.move_down(wrap_scroll);
			break;
		case OP_SK_HOME:
			downloads_list.move_to_first();
			break;
		case OP_SK_END:
			downloads_list.move_to_last();
			break;
		case OP_SK_PGUP:
			downloads_list.move_page_up(wrap_scroll);
			break;
		case OP_SK_PGDOWN:
			downloads_list.move_page_down(wrap_scroll);
			break;
		case OP_PB_TOGGLE_DLALL:
			auto_download = !auto_download;
			break;
		case OP_HARDQUIT:
		case OP_QUIT:
			if (ctrl->downloads_in_progress() > 0) {
				dllist_form.set("msg",
					_("Error: can't quit: download(s) in "
						"progress."));
				ctrl->set_view_update_necessary(true);
			} else {
				quit = true;
			}
			break;
		case OP_PB_MOREDL:
			ctrl->increase_parallel_downloads();
			break;
		case OP_PB_LESSDL:
			ctrl->decrease_parallel_downloads();
			break;
		case OP_PB_DOWNLOAD: {
			std::istringstream os(dllist_form.get("dlposname"));
			int idx = -1;
			os >> idx;
			if (idx != -1) {
				if (ctrl->downloads()[idx].status() !=
					DlStatus::DOWNLOADING) {
					std::thread t{PodDlThread(
							&ctrl->downloads()[idx],
							ctrl->get_cfgcont())};
					t.detach();
				}
			}
		}
		break;
		case OP_PB_PLAY: {
			std::istringstream os(dllist_form.get("dlposname"));
			int idx = -1;
			os >> idx;
			if (idx != -1) {
				DlStatus status =
					ctrl->downloads()[idx].status();
				if (status == DlStatus::FINISHED ||
					status == DlStatus::PLAYED ||
					status == DlStatus::READY) {
					ctrl->play_file(ctrl->downloads()[idx]
						.filename());
					ctrl->downloads()[idx].set_status(
						DlStatus::PLAYED);
				} else {
					dllist_form.set("msg",
						_("Error: download needs to be "
							"finished before the file "
							"can be played."));
				}
			}
		}
		break;
		case OP_PB_MARK_FINISHED: {
			std::istringstream os(dllist_form.get("dlposname"));
			int idx = -1;
			os >> idx;
			if (idx != -1) {
				DlStatus status =
					ctrl->downloads()[idx].status();
				if (status == DlStatus::PLAYED) {
					ctrl->downloads()[idx].set_status(
						DlStatus::FINISHED);
				}
			}
		}
		break;
		case OP_PB_CANCEL: {
			std::istringstream os(dllist_form.get("dlposname"));
			int idx = -1;
			os >> idx;
			if (idx != -1) {
				if (ctrl->downloads()[idx].status() ==
					DlStatus::DOWNLOADING) {
					ctrl->downloads()[idx].set_status(
						DlStatus::CANCELLED);
				}
			}
		}
		break;
		case OP_PB_DELETE: {
			std::istringstream os(dllist_form.get("dlposname"));
			int idx = -1;
			os >> idx;
			if (idx != -1) {
				if (ctrl->downloads()[idx].status() !=
					DlStatus::DOWNLOADING) {
					ctrl->downloads()[idx].set_status(
						DlStatus::DELETED);
				}
			}
		}
		break;
		case OP_PB_PURGE:
			if (ctrl->downloads_in_progress() > 0) {
				dllist_form.set("msg",
					_("Error: unable to perform operation: "
						"download(s) in progress."));
			} else {
				ctrl->purge_queue();
			}
			ctrl->set_view_update_necessary(true);
			break;
		case OP_HELP:
			run_help();
			break;
		default:
			break;
		}

	} while (!quit);
}

std::pair<double, std::string> PbView::get_speed_human_readable(double kbps)
{
	if (kbps < 1024) {
		return std::make_pair(kbps, _("KB/s"));
	} else if (kbps < 1024 * 1024) {
		return std::make_pair(kbps / 1024, _("MB/s"));
	} else {
		return std::make_pair(kbps / 1024 / 1024, _("GB/s"));
	}
}

void PbView::run_help()
{
	set_help_keymap_hint();

	help_form.set("head", _("Help"));

	const auto descs = keys->get_keymap_descriptions("podboat");

	ListFormatter listfmt;

	for (const auto& desc : descs) {
		std::string descline;
		descline.append(desc.key);
		descline.append(8 - desc.key.length(), ' ');
		descline.append(desc.cmd);
		descline.append(24 - desc.cmd.length(), ' ');
		descline.append(desc.desc);

		listfmt.add_line(descline);
	}

	help_textview.stfl_replace_lines(listfmt.get_lines_count(),
		listfmt.format_list());

	bool quit = false;

	do {
		const char* event = help_form.run(0);
		if (!event) {
			continue;
		}

		Operation op = keys->get_operation(event, "help");

		switch (op) {
		case OP_SK_UP:
			help_textview.scroll_up();
			break;
		case OP_SK_DOWN:
			help_textview.scroll_down();
			break;
		case OP_SK_HOME:
			help_textview.scroll_to_top();
			break;
		case OP_SK_END:
			help_textview.scroll_to_bottom();
			break;
		case OP_SK_PGUP:
			help_textview.scroll_page_up();
			break;
		case OP_SK_PGDOWN:
			help_textview.scroll_page_down();
			break;
		case OP_HARDQUIT:
		case OP_QUIT:
			quit = true;
			break;
		default:
			break;
		}
	} while (!quit);
}

std::string PbView::prepare_keymaphint(KeyMapHintEntry* hints)
{
	std::string keymap_hint;
	for (int i = 0; hints[i].op != OP_NIL; ++i) {
		std::string bound_keys = utils::join(keys->get_keys(hints[i].op, "podboat"),
				",");
		if (bound_keys.empty()) {
			bound_keys = "<none>";
		}
		keymap_hint.append(bound_keys);
		keymap_hint.append(":");
		keymap_hint.append(hints[i].text);
		keymap_hint.append(" ");
	}
	return keymap_hint;
}

void PbView::set_help_keymap_hint()
{
	KeyMapHintEntry hints[] = {{OP_QUIT, _("Quit")}, {OP_NIL, nullptr}};
	std::string keymap_hint = prepare_keymaphint(hints);
	help_form.set("help", keymap_hint);
}

void PbView::set_dllist_keymap_hint()
{
	KeyMapHintEntry hints[] = {{OP_QUIT, _("Quit")},
		{OP_PB_DOWNLOAD, _("Download")},
		{OP_PB_CANCEL, _("Cancel")},
		{OP_PB_DELETE, _("Delete")},
		{OP_PB_PURGE, _("Purge Finished")},
		{OP_PB_TOGGLE_DLALL, _("Toggle Automatic Download")},
		{OP_PB_PLAY, _("Play")},
		{OP_PB_MARK_FINISHED, _("Mark as Finished")},
		{OP_HELP, _("Help")},
		{OP_NIL, nullptr}
	};

	std::string keymap_hint = prepare_keymaphint(hints);
	dllist_form.set("help", keymap_hint);
}

std::string PbView::format_line(const std::string& podlist_format,
	const Download& dl,
	unsigned int pos,
	unsigned int width)
{
	FmtStrFormatter fmt;

	const double speed_kbps = dl.kbps();
	const auto speed = get_speed_human_readable(speed_kbps);

	fmt.register_fmt('i', strprintf::fmt("%u", pos + 1));
	fmt.register_fmt('d',
		strprintf::fmt("%.1f", dl.current_size() / (1024 * 1024)));
	fmt.register_fmt(
		't', strprintf::fmt("%.1f", dl.total_size() / (1024 * 1024)));
	fmt.register_fmt('p', strprintf::fmt("%.1f", dl.percents_finished()));
	fmt.register_fmt('k', strprintf::fmt("%.2f", speed_kbps));
	fmt.register_fmt('K', strprintf::fmt("%.2f %s", speed.first, speed.second));
	fmt.register_fmt('S', strprintf::fmt("%s", dl.status_text()));
	fmt.register_fmt('u', strprintf::fmt("%s", dl.url()));
	fmt.register_fmt('F', strprintf::fmt("%s", dl.filename()));
	fmt.register_fmt('b', strprintf::fmt("%s", dl.basename()));

	auto formattedLine = fmt.do_format(podlist_format, width);
	formattedLine = utils::quote_for_stfl(formattedLine);
	return formattedLine;
}

} // namespace podboat

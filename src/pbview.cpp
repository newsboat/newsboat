#include "pbview.h"

#include <cinttypes>
#include <cstring>
#include <ncurses.h>

#include "config.h"
#include "configcontainer.h"
#include "dllist.h"
#include "download.h"
#include "fmtstrformatter.h"
#include "help.h"
#include "listformatter.h"
#include "logger.h"
#include "pbcontroller.h"
#include "strprintf.h"

using namespace Newsboat;

namespace Podboat {

PbView::PbView(PbController& c)
	: update_view(true)
	, ctrl(c)
	, dllist_form(dllist_str)
	, help_form(help_str)
	, title_line_dllist_form(dllist_form, "head")
	, title_line_help_form(help_form, "head")
	, msg_line_dllist_form(dllist_form, "msg")
	, msg_line_help_form(help_form, "msg")
	, keys(ctrl.get_keymap())
	, colorman(ctrl.get_colormanager())
	, downloads_list("dls", dllist_form,
		  ctrl.get_cfgcont()->get_configvalue_as_int("scrolloff"))
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

	std::vector<KeyCombination> key_sequence;
	do {
		if (update_view) {
			const double total_kbps = ctrl.get_total_kbps();
			const auto speed = get_speed_human_readable(total_kbps);

			auto title = strprintf::fmt(
					_("Queue (%u downloads in progress, %u total) - %.2f %s total"),
					static_cast<unsigned int>(ctrl.downloads_in_progress()),
					static_cast<unsigned int>(ctrl.downloads().size()),
					speed.first,
					speed.second);

			if (ctrl.get_maxdownloads() > 1) {
				title += strprintf::fmt(_(" - %u parallel downloads"), ctrl.get_maxdownloads());
			}

			title_line_dllist_form.set_text(title);

			LOG(Level::DEBUG,
				"PbView::run: updating view... "
				"downloads().size() "
				"= %" PRIu64,
				static_cast<uint64_t>(ctrl.downloads().size()));

			ListFormatter listfmt;
			const std::string line_format =
				ctrl.get_cfgcont()->get_configvalue("podlist-format");

			dllist_form.run(-3); // compute all widget dimensions

			auto render_line = [this, line_format](std::uint32_t line,
			std::uint32_t width) -> StflRichText {
				const auto& downloads = ctrl.downloads();
				const auto& dl = downloads.at(line);
				return format_line(line_format, dl, line, width);
			};
			downloads_list.invalidate_list_content(ctrl.downloads().size(), render_line);

			update_view = false;
		}

		// If there's no status message, we know there's no error to show
		// Thus, it's safe to replace with the download's status
		if (msg_line_dllist_form.get_text().empty() && ctrl.downloads().size() > 0) {
			const auto idx = downloads_list.get_position();
			msg_line_dllist_form.set_text(ctrl.downloads()[idx].status_msg());
		}

		const auto event = dllist_form.run(500);

		if (auto_download) {
			if (ctrl.get_maxdownloads() >
				ctrl.downloads_in_progress()) {
				ctrl.start_downloads();
			}
		}

		if (event.empty() || event == "TIMEOUT") {
			continue;
		}

		if (event == "RESIZE") {
			handle_resize();
			continue;
		}

		const auto key_combination = KeyCombination::from_bindkey(event);
		if (key_combination == KeyCombination("ESC") && !key_sequence.empty()) {
			key_sequence.clear();
		} else {
			key_sequence.push_back(key_combination);
		}
		auto binding_state = MultiKeyBindingState::NotFound;
		BindingType type = BindingType::Bind;
		auto cmds = keys.get_operation(key_sequence, "Podboat", binding_state, type);

		if (binding_state == MultiKeyBindingState::MoreInputNeeded) {
			continue;
		}

		key_sequence.clear();

		if (cmds.size() != 1) {
			// TODO: Add support to Podboat for running a list of commands
			continue;
		}
		if (cmds.size() != 1) {
			// TODO: Add support to Podboat for running commands with arguments
			continue;
		}
		Operation op = cmds.front().op;

		if (msg_line_dllist_form.get_text().length() > 0) {
			msg_line_dllist_form.set_text("");
			update_view = true;
		}

		switch (op) {
		case OP_REDRAW:
			Stfl::reset();
			break;
		case OP_PREV:
		case OP_SK_UP:
			downloads_list.move_up(wrap_scroll);
			break;
		case OP_NEXT:
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
		case OP_SK_HALF_PAGE_UP:
			downloads_list.scroll_halfpage_up(wrap_scroll);
			break;
		case OP_SK_HALF_PAGE_DOWN:
			downloads_list.scroll_halfpage_down(wrap_scroll);
			break;
		case OP_PB_TOGGLE_DLALL:
			auto_download = !auto_download;
			break;
		case OP_HARDQUIT:
		case OP_QUIT:
			if (ctrl.downloads_in_progress() > 0) {
				msg_line_dllist_form.set_text(_("Error: can't quit: download(s) in progress."));
				update_view = true;
			} else {
				quit = true;
			}
			break;
		case OP_PB_MOREDL:
			ctrl.increase_parallel_downloads();
			break;
		case OP_PB_LESSDL:
			ctrl.decrease_parallel_downloads();
			break;
		case OP_PB_DOWNLOAD: {
			if (ctrl.downloads().size() >= 1) {
				const auto idx = downloads_list.get_position();
				auto& item = ctrl.downloads()[idx];
				if (item.status() != DlStatus::DOWNLOADING) {
					ctrl.start_download(item);
				}
			}
		}
		break;
		case OP_PB_PLAY: {
			if (ctrl.downloads().size() >= 1) {
				const auto idx = downloads_list.get_position();
				DlStatus status =
					ctrl.downloads()[idx].status();
				if (status == DlStatus::FINISHED ||
					status == DlStatus::PLAYED ||
					status == DlStatus::READY) {
					ctrl.play_file(ctrl.downloads()[idx]
						.filename());
					ctrl.downloads()[idx].set_status(
						DlStatus::PLAYED);
				} else {
					msg_line_dllist_form.set_text(_("Error: download needs to be "
							"finished before the file can be played."));
				}
			}
		}
		break;
		case OP_PB_MARK_FINISHED: {
			auto& downloads = ctrl.downloads();
			if (downloads.size() >= 1) {
				const auto idx = downloads_list.get_position();
				DlStatus status =
					downloads[idx].status();
				if (status == DlStatus::PLAYED) {
					downloads[idx].set_status(DlStatus::FINISHED);
					if (idx + 1 < downloads.size()) {
						downloads_list.set_position(idx + 1);
					}
				}
			}
		}
		break;
		case OP_PB_CANCEL: {
			if (ctrl.downloads().size() >= 1) {
				const auto idx = downloads_list.get_position();
				if (ctrl.downloads()[idx].status() ==
					DlStatus::DOWNLOADING) {
					ctrl.downloads()[idx].set_status(
						DlStatus::CANCELLED);
				}
			}
		}
		break;
		case OP_PB_DELETE: {
			auto& downloads = ctrl.downloads();
			if (downloads.size() >= 1) {
				const auto idx = downloads_list.get_position();
				if (downloads[idx].status() !=
					DlStatus::DOWNLOADING) {
					downloads[idx].set_status(DlStatus::DELETED);
					if (idx + 1 < downloads.size()) {
						downloads_list.set_position(idx + 1);
					}
				}
			}
		}
		break;
		case OP_PB_PURGE:
			if (ctrl.downloads_in_progress() > 0) {
				msg_line_dllist_form.set_text(_("Error: unable to perform operation: "
						"download(s) in progress."));
			} else {
				ctrl.purge_queue();
			}
			update_view = true;
			break;
		case OP_HELP:
			run_help();
			break;
		default:
			break;
		}

	} while (!quit);
}

void PbView::handle_resize()
{
	std::vector<std::reference_wrapper<Newsboat::Stfl::Form>> forms = {dllist_form, help_form};
	for (const auto& form : forms) {
		form.get().run(-3);
	}
	update_view = true;
}

void PbView::apply_colors_to_all_forms()
{
	using namespace std::placeholders;
	colorman.apply_colors(std::bind(&Newsboat::Stfl::Form::set, &dllist_form, _1,
			_2));
	colorman.apply_colors(std::bind(&Newsboat::Stfl::Form::set, &help_form, _1,
			_2));
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

	title_line_help_form.set_text(_("Help"));

	const auto descs = keys.get_keymap_descriptions("Podboat");

	ListFormatter listfmt;

	for (const auto& desc : descs) {
		const std::string descline = strprintf::fmt("%-7s %-23s %s",
				desc.key.to_bindkey_string(),
				desc.cmd,
				desc.desc);

		listfmt.add_line(StflRichText::from_plaintext(descline));
	}

	help_textview.stfl_replace_lines(listfmt.get_lines_count(),
		listfmt.format_list());

	bool quit = false;

	std::vector<KeyCombination> key_sequence;
	do {
		const auto event = help_form.run(0);
		if (event.empty()) {
			continue;
		}

		if (event == "RESIZE") {
			handle_resize();
			continue;
		}

		const auto key_combination = KeyCombination::from_bindkey(event);
		if (key_combination == KeyCombination("ESC") && !key_sequence.empty()) {
			key_sequence.clear();
		} else {
			key_sequence.push_back(key_combination);
		}
		auto binding_state = MultiKeyBindingState::NotFound;
		BindingType type = BindingType::Bind;
		auto cmds = keys.get_operation(key_sequence, "help", binding_state, type);

		if (binding_state == MultiKeyBindingState::MoreInputNeeded) {
			continue;
		}

		key_sequence.clear();

		if (cmds.size() != 1) {
			// TODO: Add support to Podboat for running a list of commands
			continue;
		}
		if (cmds.size() != 1) {
			// TODO: Add support to Podboat for running commands with arguments
			continue;
		}
		Operation op = cmds.front().op;

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
		case OP_SK_HALF_PAGE_UP:
			help_textview.scroll_halfpage_up();
			break;
		case OP_SK_HALF_PAGE_DOWN:
			help_textview.scroll_halfpage_down();
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

void PbView::set_help_keymap_hint()
{
	static const std::vector<KeyMapHintEntry> hints = {{OP_QUIT, _("Quit")}};
	const auto keymap_hint = keys.prepare_keymap_hint(hints, "Podboat");
	help_form.set("help", keymap_hint);
}

void PbView::set_dllist_keymap_hint()
{
	static const std::vector<KeyMapHintEntry> hints = {{OP_QUIT, _("Quit")},
		{OP_PB_DOWNLOAD, _("Download")},
		{OP_PB_CANCEL, _("Cancel")},
		{OP_PB_DELETE, _("Delete")},
		{OP_PB_PURGE, _("Purge Finished")},
		{OP_PB_TOGGLE_DLALL, _("Toggle Automatic Download")},
		{OP_PB_PLAY, _("Play")},
		{OP_PB_MARK_FINISHED, _("Mark as Finished")},
		{OP_HELP, _("Help")}
	};

	const auto keymap_hint = keys.prepare_keymap_hint(hints, "Podboat");
	dllist_form.set("help", keymap_hint);
}

StflRichText PbView::format_line(const std::string& podlist_format,
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
	return StflRichText::from_plaintext(formattedLine);
}

} // namespace Podboat

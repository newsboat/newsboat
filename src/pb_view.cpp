#include <pb_view.h>
#include <pb_controller.h>
#include <poddlthread.h>
#include <download.h>
#include <config.h>
#include <logger.h>

#include <dllist.h>
#include <help.h>
#include <utils.h>

#include <sstream>
#include <iostream>
#include <cstring>

using namespace newsbeuter;

namespace podbeuter {

pb_view::pb_view(pb_controller * c) : ctrl(c), dllist_form(dllist_str), help_form(help_str), keys(0) { 
}

pb_view::~pb_view() { 
	stfl::reset();
}

void pb_view::run(bool auto_download) {
	bool quit = false;

	set_dllist_keymap_hint();

	do {

		if (ctrl->view_update_necessary()) {

			double total_kbps = ctrl->get_total_kbps();

			char parbuf[128] = "";
			if (ctrl->get_maxdownloads() > 1) {
				snprintf(parbuf, sizeof(parbuf), _(" - %u parallel downloads"), ctrl->get_maxdownloads());
			}

			char buf[1024];
			snprintf(buf, sizeof(buf), _("Queue (%u downloads in progress, %u total) - %.2f kb/s total%s"), 
				static_cast<unsigned int>(ctrl->downloads_in_progress()), 
				static_cast<unsigned int>(ctrl->downloads().size()), total_kbps, parbuf);

			dllist_form.set("head", buf);

			LOG(LOG_DEBUG, "pb_view::run: updating view... downloads().size() = %u", ctrl->downloads().size());

			if (ctrl->downloads().size() > 0) {
				std::string code = "{list";
				
				unsigned int i = 0;
				for (auto dl : ctrl->downloads()) {
					char lbuf[1024];
					snprintf(lbuf, sizeof(lbuf), " %4u [%6.1fMB/%6.1fMB] [%5.1f %%] [%7.2f kb/s] %-20s %s -> %s", i+1, dl.current_size()/(1024*1024), dl.total_size()/(1024*1024), dl.percents_finished(), dl.kbps(), dl.status_text(), dl.url(), dl.filename());
					code.append(utils::strprintf("{listitem[%u] text:%s}", i, stfl::quote(lbuf).c_str()));
					i++;
				}

				code.append("}");

				dllist_form.modify("dls", "replace_inner", code);
			}

			ctrl->set_view_update_necessary(false);
		}

		const char * event = dllist_form.run(500);

		if (auto_download) {
			if (ctrl->get_maxdownloads() > ctrl->downloads_in_progress()) {
				ctrl->start_downloads();
			}
		}

		if (!event || strcmp(event,"TIMEOUT")==0) continue;

		operation op = keys->get_operation(event, "podbeuter");

		if (dllist_form.get("msg").length() > 0) {
			dllist_form.set("msg", "");
			ctrl->set_view_update_necessary(true);
		}

		switch (op) {
			case OP_PB_TOGGLE_DLALL:
				auto_download = !auto_download;
				break;
			case OP_HARDQUIT:
			case OP_QUIT:
				if (ctrl->downloads_in_progress() > 0) {
					dllist_form.set("msg", _("Error: can't quit: download(s) in progress."));
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
						if (ctrl->downloads()[idx].status() != DL_DOWNLOADING) {
							std::thread t{poddlthread(&ctrl->downloads()[idx], ctrl->get_cfgcont())};
						}
					}
				}
				break;
			case OP_PB_PLAY: {
					std::istringstream os(dllist_form.get("dlposname"));
					int idx = -1;
					os >> idx;
					if (idx != -1) {
						dlstatus_t status = ctrl->downloads()[idx].status();
						if (status == DL_FINISHED || status == DL_PLAYED || status == DL_READY) {
							ctrl->play_file(ctrl->downloads()[idx].filename());
							ctrl->downloads()[idx].set_status(DL_PLAYED);
						} else {
							dllist_form.set("msg", _("Error: download needs to be finished before the file can be played."));
						}
					}
				}
				break;
			case OP_PB_MARK_FINISHED: {
					std::istringstream os(dllist_form.get("dlposname"));
					int idx = -1;
					os >> idx;
					if (idx != -1) {
						dlstatus_t status = ctrl->downloads()[idx].status();
						if ( status == DL_PLAYED ) {
							ctrl->downloads()[idx].set_status(DL_FINISHED);
						}
					}
				}
				break;
			case OP_PB_CANCEL: {
					std::istringstream os(dllist_form.get("dlposname"));
					int idx = -1;
					os >> idx;
					if (idx != -1) {
						if (ctrl->downloads()[idx].status() == DL_DOWNLOADING) {
							ctrl->downloads()[idx].set_status(DL_CANCELLED);
						}
					}
				}
				break;
			case OP_PB_DELETE: {
					std::istringstream os(dllist_form.get("dlposname"));
					int idx = -1;
					os >> idx;
					if (idx != -1) {
						if (ctrl->downloads()[idx].status() != DL_DOWNLOADING) {
							ctrl->downloads()[idx].set_status(DL_DELETED);
						}
					}
				}
				break;
			case OP_PB_PURGE:
				if (ctrl->downloads_in_progress() > 0) {
					dllist_form.set("msg", _("Error: unable to perform operation: download(s) in progress."));
				} else {
					ctrl->reload_queue(true);
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



void pb_view::set_bindings() {
	if (keys) {
		std::string upkey("** "); upkey.append(keys->getkey(OP_SK_UP, "podbeuter"));
		std::string downkey("** "); downkey.append(keys->getkey(OP_SK_DOWN, "podbeuter"));
		std::string pgupkey("** "); pgupkey.append(keys->getkey(OP_SK_PGUP, "podbeuter"));
		std::string pgdownkey("** "); pgdownkey.append(keys->getkey(OP_SK_PGDOWN, "podbeuter"));
		std::string homekey("** "); homekey.append(keys->getkey(OP_SK_HOME, "podbeuter"));
		std::string endkey("** "); endkey.append(keys->getkey(OP_SK_END, "podbeuter"));

		dllist_form.set("bind_up", upkey);
		dllist_form.set("bind_down", downkey);
		dllist_form.set("bind_page_up", pgupkey);
		dllist_form.set("bind_page_down", pgdownkey);
		dllist_form.set("bind_home", homekey);
		dllist_form.set("bind_end", endkey);

		help_form.set("bind_up", upkey);
		help_form.set("bind_down", downkey);
		help_form.set("bind_page_up", pgupkey);
		help_form.set("bind_page_down", pgdownkey);
		help_form.set("bind_home", homekey);
		help_form.set("bind_end", endkey);

	}
}




void pb_view::run_help() {
	set_help_keymap_hint();

	help_form.set("head",_("Help"));
	
	std::vector<keymap_desc> descs;
	keys->get_keymap_descriptions(descs, KM_PODBEUTER);
	
	std::string code = "{list";
	
	for (auto desc : descs) {
		std::string line = "{listitem text:";

		std::string descline;
		descline.append(desc.key);
		descline.append(8-desc.key.length(),' ');
		descline.append(desc.cmd);
		descline.append(24-desc.cmd.length(),' ');
		descline.append(desc.desc);
		line.append(stfl::quote(descline));

		line.append("}");
		
		code.append(line);
	}
	
	code.append("}");
	
	help_form.modify("helptext","replace_inner",code);
	
	bool quit = false;
	
	do {
		const char * event = help_form.run(0);
		if (!event) continue;

		operation op = keys->get_operation(event, "help");

		switch (op) {
			case OP_HARDQUIT:
			case OP_QUIT:
				quit = true;
				break;
			default:
				break;
		}
	} while (!quit);
}

std::string pb_view::prepare_keymaphint(keymap_hint_entry * hints) {
	std::string keymap_hint;
	for (int i=0;hints[i].op != OP_NIL; ++i) {
		keymap_hint.append(keys->getkey(hints[i].op, "podbeuter"));
		keymap_hint.append(":");
		keymap_hint.append(hints[i].text);
		keymap_hint.append(" ");
	}
	return keymap_hint;	
}

void pb_view::set_help_keymap_hint() {
	keymap_hint_entry hints[] = {
		{ OP_QUIT, _("Quit") },
		{ OP_NIL, NULL }
	};
	std::string keymap_hint = prepare_keymaphint(hints);
	help_form.set("help", keymap_hint);
}

void pb_view::set_dllist_keymap_hint() {
	keymap_hint_entry hints[] = {
		{ OP_QUIT, _("Quit") },
		{ OP_PB_DOWNLOAD, _("Download") },
		{ OP_PB_CANCEL, _("Cancel") },
		{ OP_PB_DELETE, _("Delete") },
		{ OP_PB_PURGE, _("Purge Finished") },
		{ OP_PB_TOGGLE_DLALL, _("Toggle Automatic Download") },
		{ OP_PB_PLAY, _("Play") },
		{ OP_PB_MARK_FINISHED, _("Mark as Finished") },
		{ OP_HELP, _("Help") },
		{ OP_NIL, NULL }
	};

	std::string keymap_hint = prepare_keymaphint(hints);
	dllist_form.set("help", keymap_hint);
}

}

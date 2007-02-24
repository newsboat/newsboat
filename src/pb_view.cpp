#include <pb_view.h>
#include <pb_controller.h>
#include <poddlthread.h>
#include <dllist.h>
#include <download.h>
#include <config.h>
#include <sstream>
#include <iostream>

using namespace podbeuter;
using namespace newsbeuter;

pb_view::pb_view(pb_controller * c) : ctrl(c), dllist_form(dllist_str), keys(0) { 
}

pb_view::~pb_view() { 
	stfl::reset();
}

void pb_view::run() {
	bool quit = false;

	dllist_form.set("head", _("Queue"));

	do {

		if (ctrl->view_update_necessary()) {

			if (ctrl->downloads().size() > 0) {

				std::string code = "{list";
				
				unsigned int i = 0;
				for (std::vector<download>::iterator it=ctrl->downloads().begin();it!=ctrl->downloads().end();++it,++i) {
					char buf[1024];
					std::ostringstream os;
					snprintf(buf, sizeof(buf), " %4u [%5.1f %%] %-20s %s -> %s", i+1, it->percents_finished(), it->status_text(), it->url(), it->filename());
					os << "{listitem[" << i << "] text:" << stfl::quote(buf) << "}";
					code.append(os.str());
				}

				code.append("}");

				dllist_form.modify("dls", "replace_inner", code);
			}

			ctrl->set_view_update_necessary(false);
		}

		const char * event = dllist_form.run(1000);
		if (!event || strcmp(event,"TIMEOUT")==0) continue;

		operation op = keys->get_operation(event);

		switch (op) {
			case OP_QUIT:
				quit = true;
				break;
			case OP_PB_DOWNLOAD: {
					std::istringstream os(dllist_form.get("dlposname"));
					int idx = -1;
					os >> idx;
					if (idx != -1) {
						if (ctrl->downloads()[idx].status() != DL_DOWNLOADING) {
							poddlthread * thread = new poddlthread(&ctrl->downloads()[idx]);
							thread->start();
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
						if (ctrl->downloads()[idx].status() == DL_CANCELLED) {
							ctrl->downloads()[idx].set_status(DL_DELETED);
						}
					}
				}
				break;
			case OP_PB_PURGE:
				// TODO: delete all cancelled and finished downloads
				break;
			default:
				break;
		}

	} while (!quit);
}

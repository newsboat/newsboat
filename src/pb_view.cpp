#include <pb_view.h>
#include <pb_controller.h>
#include <dllist.h>
#include <download.h>
#include <config.h>
#include <sstream>

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

			std::string code = "{list";
			
			unsigned int i = 0;
			for (std::vector<download>::iterator it=ctrl->downloads().begin();it!=ctrl->downloads().end();++it,++i) {
				char buf[1024];
				std::ostringstream os;
				snprintf(buf, sizeof(buf), " %4u %3.1f %20s %s", i+1, it->percents_finished(), it->status_text(), it->filename());
				os << "{item[" << i << "] text:" << stfl::quote(buf) << "}";
				code.append(os.str());
			}

			code.append("}");

			dllist_form.modify("dls", "replace_inner", code);

			ctrl->set_view_update_necessary(false);
		}

		const char * event = dllist_form.run(1);
		if (!event || strcmp(event,"TIMEOUT")==0) continue;

		operation op = keys->get_operation(event);

		switch (op) {
			case OP_QUIT:
				quit = true;
				break;
			case OP_PB_DOWNLOAD:
				// TODO: start downloading current selection
				break;
			case OP_PB_CANCEL:
				// TODO: cancel currently selected download
				break;
			case OP_PB_DELETE:
				// TODO: delete currently selected download
				break;
			case OP_PB_PURGE:
				// TODO: delete all cancelled and finished downloads
				break;
			default:
				break;
		}

	} while (!quit);
}

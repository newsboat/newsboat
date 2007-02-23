#include <pb_view.h>
#include <dllist.h>

using namespace podbeuter;
using namespace newsbeuter;

pb_view::pb_view(pb_controller * c) : ctrl(c), dllist_form(dllist_str), keys(0) { 
}

pb_view::~pb_view() { 
	stfl::reset();
}

void pb_view::run() {
	bool quit = false;

	do {

		// TODO: set list

		const char * event = dllist_form.run(1);
		if (!event || strcmp(event,"TIMEOUT")==0) continue;

		operation op = keys->get_operation(event);

		switch (op) {
			case OP_QUIT:
				quit = true;
				break;
			default:
				break;
		}

	} while (!quit);
}

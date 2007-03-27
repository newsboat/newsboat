#include <formaction.h>

namespace newsbeuter {

formaction::formaction(view * vv, std::string formstr) : v(vv), f(0), do_redraw(true) { 
	f = new stfl::form(formstr);
	// TODO: set keyboard hint
}

formaction::~formaction() { 
	delete f;
}

stfl::form& formaction::get_form() {
	return *f;
}

}

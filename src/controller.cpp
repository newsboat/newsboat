#include <view.h>
#include <controller.h>

using namespace noos;

controller::controller() : v(0) { }

void controller::set_view(view * vv) {
	v = vv;
}

void controller::run() {
	// TODO
}

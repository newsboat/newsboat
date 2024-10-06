#include "tui.h"

#include <cassert>

#include "libnewsboat-ffi/src/tui.rs.h"

namespace {
	newsboat::Tui* tui_instance = nullptr;
}

namespace newsboat {

Tui::Tui()
	: rs_object(tui::bridged::create())
{
	tui_instance = this;
}

Tui::~Tui()
{
	tui_instance = nullptr;
}

Tui& Tui::get_instance()
{
	assert(tui_instance != nullptr);
	return *tui_instance;
}

void Tui::run()
{
	tui::bridged::run(*rs_object);
}

} // namespace newsboat

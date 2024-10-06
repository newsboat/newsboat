#include "tui.h"
#include "libnewsboat-ffi/src/tui.rs.h"

namespace newsboat {

Tui::Tui()
	: rs_object(tui::bridged::create())
{
}

void Tui::run()
{
	tui::bridged::run(*rs_object);
}

} // namespace newsboat

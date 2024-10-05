#include "tui.h"

namespace newsboat {

Tui::Tui()
	: rs_object(tui::bridged::create())
{
}

} // namespace newsboat

#ifndef NEWSBOAT_TUI_H_
#define NEWSBOAT_TUI_H_

#include "libnewsboat-ffi/src/tui.rs.h"

namespace newsboat {

class Tui {
public:
	Tui();
	~Tui() = default;

private:
	rust::Box<tui::bridged::Tui> rs_object;
};

} // namespace newsboat

#endif /* NEWSBOAT_TUI_H_ */

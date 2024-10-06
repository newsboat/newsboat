#ifndef NEWSBOAT_TUI_H_
#define NEWSBOAT_TUI_H_

#include <cstdint>
#include <string>

#include "libnewsboat-ffi/src/tui.rs.h"

namespace newsboat {

class Tui {
public:
	Tui();
	~Tui();

	static Tui& get_instance();

	std::string run(std::int32_t timeout);

private:
	rust::Box<tui::bridged::Tui> rs_object;
};

} // namespace newsboat

#endif /* NEWSBOAT_TUI_H_ */

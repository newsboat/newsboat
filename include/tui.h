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
	std::string get_variable(std::string key);
	void set_variable(std::string key, std::string value);
	void modify_form(std::string name, std::string mode, std::string value);

private:
	rust::Box<tui::bridged::Tui> rs_object;
};

} // namespace newsboat

#endif /* NEWSBOAT_TUI_H_ */

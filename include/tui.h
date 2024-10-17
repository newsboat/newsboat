#ifndef NEWSBOAT_TUI_H_
#define NEWSBOAT_TUI_H_

#include <cstdint>
#include <string>

#include "libnewsboat-ffi/src/tui.rs.h"

namespace newsboat {

class RustForm {
public:
	RustForm();

	std::string get_variable(std::string key);
	void set_variable(std::string key, std::string value);
	void modify_form(std::string name, std::string mode, std::string value);

// TODO: Avoid leaking implementation detail?
public:
	rust::Box<tui::bridged::Form> rs_object;
};

class Tui {
public:
	Tui();
	~Tui();

	static Tui& get_instance(); // TODO: Remove
	void reset();
	std::string run(RustForm& form, std::int32_t timeout);

private:
	rust::Box<tui::bridged::Tui> rs_object;
};

} // namespace newsboat

#endif /* NEWSBOAT_TUI_H_ */

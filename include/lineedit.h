#ifndef NEWSBOAT_LINEEDIT_H_
#define NEWSBOAT_LINEEDIT_H_

#include "libnewsboat-ffi/src/lineedit.rs.h" // IWYU pragma: export

#include "event.h"
#include "stflpp.h"

#include <cstdint>

namespace newsboat {

class LineEdit {
public:
	LineEdit(Stfl::Form& form, const std::string& name);

	void set_value(const std::string& value);
	std::string get_value();

	void show();
	void hide();

	void set_position(std::uint32_t pos);
	std::uint32_t get_position();

	void handle_event(const Event& event);

private:
	void sync_to_stfl();

	rust::Box<lineedit::bridged::LineEdit> rs_object;
	Stfl::Form& f;
	const std::string name;
};

} // namespace newsboat

#endif /* NEWSBOAT_LINEEDIT_H_ */

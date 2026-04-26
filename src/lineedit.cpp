#include "lineedit.h"

#include "libnewsboat-ffi/src/lineedit.rs.h"

namespace newsboat {

LineEdit::LineEdit(Stfl::Form& form, const std::string& name)
	: rs_object(lineedit::bridged::create())
	, f(form)
	, name(name)
{
}

void LineEdit::set_value(const std::string& value)
{
	lineedit::bridged::set_text(*rs_object, value);
	sync_to_stfl();
}

std::string LineEdit::get_value()
{
	return std::string(lineedit::bridged::get_text(*rs_object));
}

void LineEdit::show()
{
	f.set("show_" + name + "_input", "1");
}

void LineEdit::hide()
{
	f.set("show_" + name + "_input", "0");
}

void LineEdit::set_position(std::uint32_t pos)
{
	lineedit::bridged::set_cursor_location(*rs_object, pos);
	sync_to_stfl();
}

std::uint32_t LineEdit::get_position()
{
	return lineedit::bridged::get_cursor_location(*rs_object);
}

void LineEdit::handle_event(const Event& event)
{
	if (event.printableCharacter.has_value()) {
		lineedit::bridged::insert_at_cursor(*rs_object, event.printableCharacter.value());
	} else {
		lineedit::bridged::handle_event(*rs_object, event.name);
	}
	sync_to_stfl();
}

void LineEdit::sync_to_stfl()
{
	const auto text = std::string(lineedit::bridged::get_text(*rs_object));
	const auto location = lineedit::bridged::get_cursor_location(*rs_object);

	f.set(name + "_value", text);
	f.set(name + "_value_pos", std::to_string(location));
}

} // namespace newsboat

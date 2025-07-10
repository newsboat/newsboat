#include "lineedit.h"

#include "utils.h"

namespace Newsboat {

LineEdit::LineEdit(Stfl::Form& form, const std::string& name)
	: f(form)
	, name(name)
{
}

void LineEdit::set_value(const std::string& value)
{
	f.set(name + "_value", value);
}

std::string LineEdit::get_value()
{
	return f.get(name + "_value");
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
	f.set(name + "_value_pos", std::to_string(pos));
}

std::uint32_t LineEdit::get_position()
{
	std::string pos = f.get(name + "_value_pos");
	return utils::to_u(pos, 0);
}

} // namespace Newsboat

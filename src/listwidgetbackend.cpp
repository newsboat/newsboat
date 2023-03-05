#include "listwidgetbackend.h"

#include "utils.h"

namespace newsboat {

ListWidgetBackend::ListWidgetBackend(const std::string& list_name, Stfl::Form& form)
	: list_name(list_name)
	, form(form)
	, num_lines(0)
{
}

void ListWidgetBackend::stfl_replace_list(std::string stfl)
{
	num_lines = 0;
	form.modify(list_name, "replace", stfl);

	on_list_changed();
}

void ListWidgetBackend::stfl_replace_lines(const ListFormatter& listfmt)
{
	num_lines = listfmt.get_lines_count();
	form.modify(list_name, "replace_inner", listfmt.format_list());

	on_list_changed();
}

std::uint32_t ListWidgetBackend::get_width()
{
	return utils::to_u(form.get(list_name + ":w"));
}

std::uint32_t ListWidgetBackend::get_height()
{
	return utils::to_u(form.get(list_name + ":h"));
}

std::uint32_t ListWidgetBackend::get_num_lines()
{
	return num_lines;
}

void ListWidgetBackend::update_position(std::uint32_t pos, std::uint32_t scroll_offset)
{
	form.set(list_name + "_pos", std::to_string(pos));
	form.set(list_name + "_offset", std::to_string(scroll_offset));
}

} // namespace newsboat

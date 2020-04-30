#include "listwidget.h"

#include <algorithm>

#include "utils.h"

namespace newsboat {

ListWidget::ListWidget(const std::string& list_name,
	Stfl::Form& form)
	: list_name(list_name)
	, form(form)
	, num_lines(0)
{
}

void ListWidget::stfl_replace_list(std::uint32_t number_of_lines,
	std::string stfl)
{
	num_lines = number_of_lines;
	form.modify(list_name, "replace", stfl);
}

void ListWidget::stfl_replace_lines(std::uint32_t number_of_lines,
	std::string stfl)
{
	num_lines = number_of_lines;
	form.modify(list_name, "replace_inner", stfl);
}

bool ListWidget::move_up()
{
	std::uint32_t curpos = get_position();
	if (curpos > 0) {
		set_position(curpos - 1);
		return true;
	}
	return false;
}

bool ListWidget::move_down()
{
	if (num_lines == 0) {
		// Ignore if list is empty
		return false;
	}
	std::uint32_t maxpos = num_lines - 1;
	std::uint32_t curpos = get_position();
	if (curpos + 1 <= maxpos) {
		set_position(curpos + 1);
		return true;
	}
	return false;
}

void ListWidget::move_to_first()
{
	set_position(0);
}

void ListWidget::move_to_last()
{
	if (num_lines == 0) {
		// Ignore if list is empty
		return;
	}
	std::uint32_t maxpos = num_lines - 1;
	set_position(maxpos);
}

void ListWidget::move_page_up()
{
	std::uint32_t curpos = get_position();
	std::uint32_t list_height = get_height();
	if (curpos > list_height) {
		set_position(curpos - list_height);
	} else {
		set_position(0);
	}
}

void ListWidget::move_page_down()
{
	if (num_lines == 0) {
		// Ignore if list is empty
		return;
	}
	std::uint32_t maxpos = num_lines - 1;
	std::uint32_t curpos = get_position();
	std::uint32_t list_height = get_height();
	if (curpos + list_height < maxpos) {
		set_position(curpos + list_height);
	} else {
		set_position(maxpos);
	}
}

std::uint32_t ListWidget::get_position()
{
	std::string pos = form.get(list_name + "_pos");
	if (!pos.empty()) {
		return std::max(0, std::stoi(pos));
	}
	return 0;
}

void ListWidget::set_position(std::uint32_t pos)
{
	form.set(list_name + "_pos", std::to_string(pos));
}

std::uint32_t ListWidget::get_width()
{
	return utils::to_u(form.get(list_name + ":w"));
}

std::uint32_t ListWidget::get_height()
{
	return utils::to_u(form.get(list_name + ":h"));
}

} // namespace newsboat

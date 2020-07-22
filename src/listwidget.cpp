#include "listwidget.h"

#include <algorithm>

#include "utils.h"

namespace newsboat {

ListWidget::ListWidget(const std::string& list_name, Stfl::Form& form,
	std::uint32_t scrolloff)
	: list_name(list_name)
	, form(form)
	, num_lines(0)
	, num_context_lines(scrolloff)
{
}

void ListWidget::stfl_replace_list(std::uint32_t number_of_lines,
	std::string stfl)
{
	num_lines = number_of_lines;
	form.modify(list_name, "replace", stfl);
}

void ListWidget::stfl_replace_lines(const ListFormatter& listfmt)
{
	num_lines = listfmt.get_lines_count();
	form.modify(list_name, "replace_inner", listfmt.format_list());
}

bool ListWidget::move_up(bool wrap_scroll)
{
	const std::uint32_t curpos = get_position();
	if (curpos > 0) {
		set_position(curpos - 1);
		return true;
	} else if (wrap_scroll) {
		move_to_last();
		return true;
	}
	return false;
}

bool ListWidget::move_down(bool wrap_scroll)
{
	if (num_lines == 0) {
		// Ignore if list is empty
		return false;
	}
	const std::uint32_t maxpos = num_lines - 1;
	const std::uint32_t curpos = get_position();
	if (curpos + 1 <= maxpos) {
		set_position(curpos + 1);
		return true;
	} else if (wrap_scroll) {
		move_to_first();
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
	const std::uint32_t maxpos = num_lines - 1;
	set_position(maxpos);
}

void ListWidget::move_page_up(bool wrap_scroll)
{
	const std::uint32_t curpos = get_position();
	const std::uint32_t list_height = get_height();
	if (curpos > list_height) {
		set_position(curpos - list_height);
	} else if (wrap_scroll && curpos == 0) {
		move_to_last();
	} else {
		set_position(0);
	}
}

void ListWidget::move_page_down(bool wrap_scroll)
{
	if (num_lines == 0) {
		// Ignore if list is empty
		return;
	}
	const std::uint32_t maxpos = num_lines - 1;
	const std::uint32_t curpos = get_position();
	const std::uint32_t list_height = get_height();
	if (curpos + list_height < maxpos) {
		set_position(curpos + list_height);
	} else if (wrap_scroll && curpos == maxpos) {
		move_to_first();
	} else {
		set_position(maxpos);
	}
}

std::uint32_t ListWidget::get_position()
{
	const std::string pos = form.get(list_name + "_pos");
	if (!pos.empty()) {
		return std::max(0, std::stoi(pos));
	}
	return 0;
}

void ListWidget::set_position(std::uint32_t pos)
{
	form.set(list_name + "_pos", std::to_string(pos));
	update_scroll_offset(pos);
}

std::uint32_t ListWidget::get_width()
{
	return utils::to_u(form.get(list_name + ":w"));
}

std::uint32_t ListWidget::get_height()
{
	return utils::to_u(form.get(list_name + ":h"));
}

std::uint32_t ListWidget::get_scroll_offset()
{
	const std::string offset = form.get(list_name + "_offset");
	if (!offset.empty()) {
		return std::max(0, std::stoi(offset));
	}
	return 0;
}

void ListWidget::set_scroll_offset(std::uint32_t offset)
{
	form.set(list_name + "_offset", std::to_string(offset));
}

void ListWidget::update_scroll_offset(std::uint32_t pos)
{
	const auto h = get_height();
	const auto cur_scroll_offset = get_scroll_offset();
	const std::uint32_t max_offset = (num_lines >= h ? num_lines - h : 0);

	if (2 * num_context_lines < h) {
		if (pos + num_context_lines >= cur_scroll_offset + h) { // Need to scroll down
			if (pos + num_context_lines >= h) {
				const std::uint32_t target_offset = pos + num_context_lines - h + 1;
				set_scroll_offset(std::min(target_offset, max_offset));
			} else {
				set_scroll_offset(0);
			}
		}
		if (pos < cur_scroll_offset + num_context_lines) { // Need to scroll up
			if (pos >= num_context_lines) {
				set_scroll_offset(pos - num_context_lines);
			} else {
				set_scroll_offset(0);
			}
		}
	} else { // Keep selected item in the middle
		if (pos > h / 2) {
			const std::uint32_t target_offset = pos - h / 2;
			set_scroll_offset(std::min(target_offset, max_offset));
		} else {
			set_scroll_offset(0);
		}
	}
}

} // namespace newsboat

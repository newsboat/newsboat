#include "textviewwidget.h"

#include <algorithm>

#include "utils.h"

namespace newsboat {

TextviewWidget::TextviewWidget(const std::string& textview_name,
	Stfl::Form& form)
	: textview_name(Utf8String::from_utf8(textview_name))
	, form(form)
	, num_lines(0)
{
}

void TextviewWidget::stfl_replace_textview(std::uint32_t number_of_lines,
	std::string stfl)
{
	num_lines = number_of_lines;
	form.modify(textview_name.to_utf8(), "replace", stfl);
}

void TextviewWidget::stfl_replace_lines(std::uint32_t number_of_lines,
	std::string stfl)
{
	num_lines = number_of_lines;
	form.modify(textview_name.to_utf8(), "replace_inner", stfl);
}

void TextviewWidget::scroll_up()
{
	const std::uint32_t offset = get_scroll_offset();
	if (offset > 0) {
		set_scroll_offset(offset - 1);
	}
}

void TextviewWidget::scroll_down()
{
	if (num_lines == 0) {
		// Ignore if list is empty
	}
	const std::uint32_t maxoffset = num_lines - 1;
	const std::uint32_t offset = get_scroll_offset();
	if (offset < maxoffset) {
		set_scroll_offset(offset + 1);
	}
}
void TextviewWidget::scroll_to_top()
{
	set_scroll_offset(0);
}

void TextviewWidget::scroll_to_bottom()
{
	if (num_lines == 0) {
		// Ignore if list is empty
	}
	const std::uint32_t maxoffset = num_lines - 1;
	const std::uint32_t widget_height = get_height();
	if (maxoffset + 2 < widget_height) {
		set_scroll_offset(0);
	} else {
		set_scroll_offset((maxoffset + 2) - widget_height);
	}
}

void TextviewWidget::scroll_page_up()
{
	const std::uint32_t offset = get_scroll_offset();
	const std::uint32_t widget_height = get_height();
	if (offset + 1 > widget_height) {
		set_scroll_offset((offset + 1) - widget_height);
	} else {
		set_scroll_offset(0);
	}
}

void TextviewWidget::scroll_page_down()
{
	if (num_lines == 0) {
		// Ignore if list is empty
	}
	const std::uint32_t maxoffset = num_lines - 1;
	const std::uint32_t offset = get_scroll_offset();
	const std::uint32_t widget_height = get_height();
	if (offset + widget_height - 1 < maxoffset) {
		set_scroll_offset(offset + widget_height - 1);
	} else {
		set_scroll_offset(maxoffset);
	}
}

std::uint32_t TextviewWidget::get_scroll_offset()
{
	const std::string offset = form.get((textview_name + "_offset").to_utf8());
	if (!offset.empty()) {
		return std::max(0, std::stoi(offset));
	}
	return 0;
}

void TextviewWidget::set_scroll_offset(std::uint32_t offset)
{
	form.set((textview_name + "_offset").to_utf8(), std::to_string(offset));
}

std::uint32_t TextviewWidget::get_width()
{
	return utils::to_u(form.get((textview_name + ":w").to_utf8()));
}

std::uint32_t TextviewWidget::get_height()
{
	return utils::to_u(form.get((textview_name + ":h").to_utf8()));
}

} // namespace newsboat

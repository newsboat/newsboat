#include "newlistwidgetbackend.h"

#include "listformatter.h"
#include "utils.h"

namespace newsboat {

NewListWidgetBackend::NewListWidgetBackend(const std::string& list_name,
	const std::string& context, Stfl::Form& form, RegexManager& rxman)
	: list_name(list_name)
	, form(form)
	, listfmt(&rxman, context)
	, num_lines(0)
	, scroll_offset(0)
	, get_formatted_line({})
{
}

NewListWidgetBackend::NewListWidgetBackend(const std::string& list_name, Stfl::Form& form)
	: list_name(list_name)
	, form(form)
	, listfmt()
	, num_lines(0)
	, scroll_offset(0)
	, get_formatted_line({})
{
}

void NewListWidgetBackend::stfl_replace_list(std::string stfl)
{
	num_lines = 0;
	scroll_offset = 0;
	get_formatted_line = {};

	form.modify(list_name, "replace", stfl);

	invalidate_list_content(0, {});
	on_list_changed();
}

std::uint32_t NewListWidgetBackend::get_width()
{
	return utils::to_u(form.get(list_name + ":w"));
}

std::uint32_t NewListWidgetBackend::get_height()
{
	return utils::to_u(form.get(list_name + ":h"));
}

std::uint32_t NewListWidgetBackend::get_num_lines()
{
	return num_lines;
}

void NewListWidgetBackend::invalidate_list_content(std::uint32_t line_count,
	std::function<std::string(std::uint32_t, std::uint32_t)> get_line_method)
{
	get_formatted_line = get_line_method;
	num_lines = line_count;

	on_list_changed();

	render();
}

void NewListWidgetBackend::update_position(std::uint32_t pos,
	std::uint32_t new_scroll_offset)
{
	scroll_offset = new_scroll_offset;
	form.set(list_name + "_pos", std::to_string(pos - scroll_offset));
	form.set(list_name + "_offset", "0");

	render();
}

void NewListWidgetBackend::render()
{
	const auto viewport_width = get_width();
	const auto viewport_height = get_height();
	const auto visible_content_lines = std::min(viewport_height, num_lines - scroll_offset);

	listfmt.clear();
	for (std::uint32_t i = 0; i < visible_content_lines; ++i) {
		std::string formatted_line = "NO FORMATTER DEFINED";
		if (get_formatted_line) {
			formatted_line = get_formatted_line(scroll_offset + i, viewport_width);
		}
		listfmt.add_line(formatted_line);
	}

	form.modify(list_name, "replace_inner", listfmt.format_list());
}

} // namespace newsboat
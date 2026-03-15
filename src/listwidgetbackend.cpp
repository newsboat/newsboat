#include "listwidgetbackend.h"

#include "listformatter.h"
#include "utils.h"

namespace newsboat {

ListWidgetBackend::ListWidgetBackend(const std::string& list_name,
	Dialog context, Stfl::Form& form, RegexManager& rxman)
	: list_name(list_name)
	, form(form)
	, listfmt(&rxman, context)
	, num_lines(0)
	, scroll_offset(0)
	, get_formatted_line({})
{
}

ListWidgetBackend::ListWidgetBackend(const std::string& list_name, Stfl::Form& form)
	: list_name(list_name)
	, form(form)
	, listfmt()
	, num_lines(0)
	, scroll_offset(0)
	, get_formatted_line({})
{
}

void ListWidgetBackend::stfl_replace_list(std::string stfl)
{
	num_lines = 0;
	scroll_offset = 0;
	{
		std::lock_guard<std::mutex> guard(mutex);
		line_cache.clear();
	}
	get_formatted_line = {};

	form.modify(list_name, "replace", stfl);

	invalidate_list_content(0, {});
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

void ListWidgetBackend::invalidate_list_content(std::uint32_t line_count,
	std::function<StflRichText(std::uint32_t, std::uint32_t)> get_line_method)
{
	{
		std::lock_guard<std::mutex> guard(mutex);
		line_cache.clear();
	}
	get_formatted_line = get_line_method;
	num_lines = line_count;

	on_list_changed();

	render();
}

void ListWidgetBackend::update_position(std::uint32_t pos,
	std::uint32_t new_scroll_offset)
{
	scroll_offset = new_scroll_offset;
	form.set(list_name + "_pos", std::to_string(pos - scroll_offset));
	form.set(list_name + "_offset", "0");

	render();
}

void ListWidgetBackend::render()
{
	const auto viewport_width = get_width();
	const auto viewport_height = get_height();
	const auto visible_content_lines = std::min(viewport_height, num_lines - scroll_offset);

	listfmt.clear();
	std::lock_guard<std::mutex> guard(mutex);
	for (std::uint32_t i = 0; i < visible_content_lines; ++i) {
		const std::uint32_t line = scroll_offset + i;
		auto formatted_line = StflRichText::from_plaintext("NO FORMATTER DEFINED");
		if (line_cache.count(line) >= 1) {
			formatted_line = line_cache.at(line);
		} else if (get_formatted_line) {
			formatted_line = get_formatted_line(line, viewport_width);
			line_cache.insert({line, formatted_line});
		}
		listfmt.add_line(formatted_line);
	}

	form.modify(list_name, "replace_inner", listfmt.format_list());
}

} // namespace newsboat

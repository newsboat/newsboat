#include "listformatter.h"

#include <limits.h>

#include "logger.h"
#include "stflpp.h"
#include "stflrichtext.h"
#include "strprintf.h"
#include "utils.h"

namespace newsboat {

ListFormatter::ListFormatter(RegexManager* r, std::optional<Dialog> loc)
	: rxman(r)
	, location(loc)
{}

void ListFormatter::add_line(const StflRichText& text)
{
	LOG(Level::DEBUG, "ListFormatter::add_line: `%s'", text.stfl_quoted());
	set_line(UINT_MAX, text);
}

void ListFormatter::set_line(const unsigned int itempos,
	const StflRichText& text)
{
	const std::wstring wide = utils::str2wstr(text.stfl_quoted());
	const std::wstring cleaned = utils::clean_nonprintable_characters(wide);
	const std::string formatted_text = utils::wstr2str(cleaned);
	const StflRichText stflRichText = StflRichText::from_quoted(formatted_text);

	if (itempos == UINT_MAX) {
		lines.push_back(stflRichText);
	} else {
		lines[itempos] = stflRichText;
	}
}

std::string ListFormatter::format_list() const
{
	std::string format_cache = "{list";
	for (auto str : lines) {
		if (rxman && location.has_value()) {
			rxman->quote_and_highlight(str, location.value());
		}
		format_cache.append(strprintf::fmt(
				"{listitem text:%s}", Stfl::quote(str.stfl_quoted())));
	}
	format_cache.push_back('}');
	return format_cache;
}

} // namespace newsboat

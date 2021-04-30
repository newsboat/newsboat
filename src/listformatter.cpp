#include "listformatter.h"

#include <assert.h>
#include <limits.h>

#include "stflpp.h"
#include "strprintf.h"
#include "utils.h"

namespace newsboat {

ListFormatter::ListFormatter(RegexManager* r, const std::string& loc)
	: rxman(r)
	, location(Utf8String::from_utf8(loc))
{}

ListFormatter::~ListFormatter() {}

void ListFormatter::add_line(const std::string& text)
{
	set_line(UINT_MAX, text);
	LOG(Level::DEBUG, "ListFormatter::add_line: `%s'", text);
}

void ListFormatter::set_line(const unsigned int itempos,
	const std::string& text)
{
	std::vector<Utf8String> formatted_text;

	formatted_text.push_back(Utf8String::from_utf8(utils::wstr2str(
				utils::clean_nonprintable_characters(utils::str2wstr(text)))));

	if (itempos == UINT_MAX) {
		lines.insert(lines.cend(),
			formatted_text.cbegin(),
			formatted_text.cend());
	} else {
		lines[itempos] = formatted_text[0];
	}
}

std::string ListFormatter::format_list() const
{
	std::string format_cache = "{list";
	for (auto str : lines) {
		std::string utf8_str = str.to_utf8();
		if (rxman) {
			rxman->quote_and_highlight(utf8_str, location.to_utf8());
		}
		format_cache.append(strprintf::fmt("{listitem text:%s}", Stfl::quote(utf8_str)));
	}
	format_cache.push_back('}');
	return format_cache;
}

} // namespace newsboat

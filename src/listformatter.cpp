#include "listformatter.h"

#include <assert.h>
#include <limits.h>

#include "stflpp.h"
#include "stflrichtext.h"
#include "strprintf.h"
#include "utils.h"

namespace newsboat {

ListFormatter::ListFormatter(RegexManager* r, const std::string& loc)
	: rxman(r)
	, location(loc)
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
	const std::string formatted_text = utils::wstr2str(utils::clean_nonprintable_characters(
				utils::str2wstr(text)));

	// TODO: Propagate usage of StflRichText
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
		if (rxman) {
			rxman->quote_and_highlight(str, location);
		}
		format_cache.append(strprintf::fmt(
				"{listitem text:%s}", Stfl::quote(str.stfl_quoted_string())));
	}
	format_cache.push_back('}');
	return format_cache;
}

} // namespace newsboat

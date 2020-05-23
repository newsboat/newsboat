#include "listformatter.h"

#include <assert.h>
#include <limits.h>

#include "stflpp.h"
#include "strprintf.h"
#include "utils.h"

namespace newsboat {

ListFormatter::ListFormatter(RegexManager* r, const std::string& loc)
	: rxman(r)
	, location(loc)
{}

ListFormatter::~ListFormatter() {}

void ListFormatter::add_line(const std::string& text,
	const std::string& id,
	unsigned int width)
{
	set_line(UINT_MAX, text, id, width);
	LOG(Level::DEBUG, "ListFormatter::add_line: `%s'", text);
}

void ListFormatter::set_line(const unsigned int itempos,
	const std::string& text,
	const std::string& id,
	unsigned int width)
{
	std::vector<LineIdPair> formatted_text;

	if (width > 0 && text.length() > 0) {
		std::wstring mytext = utils::clean_nonprintable_characters(
				utils::str2wstr(text));

		while (mytext.length() > 0) {
			size_t size = mytext.length();
			size_t w = utils::wcswidth_stfl(mytext, size);
			if (w > width) {
				while (size &&
					(w = utils::wcswidth_stfl(
								mytext, size)) > width) {
					size--;
				}
			}
			formatted_text.push_back(LineIdPair(
					utils::wstr2str(mytext.substr(0, size)), id));
			mytext.erase(0, size);
		}
	} else {
		formatted_text.push_back(LineIdPair(
				utils::wstr2str(utils::clean_nonprintable_characters(
						utils::str2wstr(text))),
				id));
	}

	if (itempos == UINT_MAX) {
		lines.insert(lines.cend(),
			formatted_text.cbegin(),
			formatted_text.cend());
	} else {
		lines[itempos] = formatted_text[0];
	}
}

void ListFormatter::add_lines(const std::vector<std::string>& thelines,
	unsigned int width)
{
	for (const auto& line : thelines) {
		add_line(utils::replace_all(line, "\t", "        "),
			"",
			width);
	}
}

std::string ListFormatter::format_list()
{
	format_cache = "{list";
	for (const auto& line : lines) {
		std::string str = line.first;
		if (rxman) {
			rxman->quote_and_highlight(str, location);
		}
		if (line.second.empty()) {
			format_cache.append(strprintf::fmt(
					"{listitem text:%s}", Stfl::quote(str)));
		} else {
			format_cache.append(
				strprintf::fmt("{listitem[%s] text:%s}",
					line.second,
					Stfl::quote(str)));
		}
	}
	format_cache.push_back('}');
	return format_cache;
}

} // namespace newsboat

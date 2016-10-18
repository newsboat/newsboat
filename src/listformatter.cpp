#include <listformatter.h>
#include <utils.h>
#include <stflpp.h>
#include <assert.h>
#include <limits.h>

namespace newsbeuter {

listformatter::listformatter() { }

listformatter::~listformatter() { }

void listformatter::add_line(const std::string& text, unsigned int id, unsigned int width) {
	set_line(UINT_MAX, text, id, width);
	LOG(level::DEBUG, "listformatter::add_line: `%s'", text.c_str());
}

void listformatter::set_line(const unsigned int itempos,
    const std::string& text, unsigned int id, unsigned int width)
{
	std::vector<line_id_pair> formatted_text;

	if (width > 0 && text.length() > 0) {
		std::wstring mytext = utils::clean_nonprintable_characters(utils::str2wstr(text));

		while (mytext.length() > 0) {
			size_t size = mytext.length();
			size_t w = utils::wcswidth_stfl(mytext, size);
			if (w > width) {
				while (size && (w = utils::wcswidth_stfl(mytext, size)) > width) {
					size--;
				}
			}
			formatted_text.push_back(
					line_id_pair(utils::wstr2str(mytext.substr(0, size)), id));
			mytext.erase(0, size);
		}
	} else {
		formatted_text.push_back(
				line_id_pair(
					utils::wstr2str(
						utils::clean_nonprintable_characters(
							utils::str2wstr(text))),
					id));
	}

	if (itempos == UINT_MAX) {
		lines.insert(
				lines.cend(),
				formatted_text.cbegin(),
				formatted_text.cend());
	} else {
		lines[itempos] = formatted_text[0];
	}
}

void listformatter::add_lines(const std::vector<std::string>& thelines, unsigned int width) {
	for (auto line : thelines) {
		add_line(utils::replace_all(line, "\t", "        "), UINT_MAX, width);
	}
}

std::string listformatter::format_list(regexmanager * rxman, const std::string& location) {
	format_cache = "{list";
	for (auto line : lines) {
		std::string str = line.first;
		if (rxman)
			rxman->quote_and_highlight(str, location);
		if (line.second == UINT_MAX) {
			format_cache.append(utils::strprintf("{listitem text:%s}", stfl::quote(str).c_str()));
		} else {
			format_cache.append(utils::strprintf("{listitem[%u] text:%s}", line.second, stfl::quote(str).c_str()));
		}
	}
	format_cache.append(1, '}');
	return format_cache;
}

}

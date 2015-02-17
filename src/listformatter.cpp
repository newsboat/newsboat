#include <listformatter.h>
#include <utils.h>
#include <stflpp.h>

namespace newsbeuter {

listformatter::listformatter() : refresh_cache(true) { }

listformatter::~listformatter() { }

void listformatter::add_line(const std::string& text, unsigned int id, unsigned int width) {
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
			lines.push_back(line_id_pair(utils::wstr2str(mytext.substr(0, size)), id));
			mytext.erase(0, size);
		}
	} else {
		lines.push_back(line_id_pair(utils::wstr2str(utils::clean_nonprintable_characters(utils::str2wstr(text))), id));
	}
	LOG(LOG_DEBUG, "listformatter::add_line: `%s'", text.c_str());
	refresh_cache = true;
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
	refresh_cache = false;
	return format_cache;
}

}

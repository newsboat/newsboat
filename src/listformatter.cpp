#include <listformatter.h>
#include <utils.h>
#include <stflpp.h>

namespace newsbeuter {

listformatter::listformatter() : refresh_cache(true) { }

listformatter::~listformatter() { }

void listformatter::add_line(const std::string& text, unsigned int id) {
	lines.push_back(line_id_pair(text, id));
	refresh_cache = true;
}

void listformatter::add_lines(const std::vector<std::string>& lines) {
	for (std::vector<std::string>::const_iterator it=lines.begin();it!=lines.end();++it) {
		add_line(*it);
	}
}

std::string listformatter::format_list() {
	format_cache = "{list";
	for (std::vector<line_id_pair>::iterator it=lines.begin();it!=lines.end();++it) {
		if (it->second == UINT_MAX) {
			format_cache.append(utils::strprintf("{listitem text:%s}", stfl::quote(it->first).c_str()));
		} else {
			format_cache.append(utils::strprintf("{listitem[%u] text:%s}", it->second, stfl::quote(it->first).c_str()));
		}
	}
	format_cache.append(1, '}');
	refresh_cache = false;
	return format_cache;
}

}

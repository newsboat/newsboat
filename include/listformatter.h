#ifndef LISTFORMATTER__H
#define LISTFORMATTER__H

#include <climits>
#include <vector>
#include <string>
#include <utility>
#include <regexmanager.h>

namespace newsbeuter {


class listformatter {

	typedef std::pair<std::string, unsigned int> line_id_pair;

	public:
		listformatter();
		~listformatter();
		void add_line(const std::string& text, unsigned int id = UINT_MAX, unsigned int width = 0);
		void add_lines(const std::vector<std::string>& lines, unsigned int width = 0);
		std::string format_list(regexmanager * r = NULL, const std::string& location = "");
		inline unsigned int get_lines_count() { return lines.size(); }
	private:
		std::vector<line_id_pair> lines;
		std::string format_cache;
		bool refresh_cache;
};

}

#endif

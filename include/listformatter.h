#ifndef LISTFORMATTER__H
#define LISTFORMATTER__H

#include <climits>
#include <vector>
#include <string>
#include <utility>

namespace newsbeuter {


class listformatter {

	typedef std::pair<std::string, unsigned int> line_id_pair;

	public:
		listformatter();
		~listformatter();
		void add_line(const std::string& text, unsigned int id = UINT_MAX);
		void add_lines(const std::vector<std::string>& lines);
		std::string format_list();
	private:
		std::vector<line_id_pair> lines;
		std::string format_cache;
		bool refresh_cache;
};

}

#endif

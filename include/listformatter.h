#ifndef NEWSBOAT_LISTFORMATTER_H_
#define NEWSBOAT_LISTFORMATTER_H_

#include <climits>
#include <string>
#include <utility>
#include <vector>

#include "regexmanager.h"

namespace newsboat {

class listformatter {
	typedef std::pair<std::string, unsigned int> line_id_pair;

public:
	listformatter();
	~listformatter();
	void add_line(const std::string& text,
		unsigned int id = UINT_MAX,
		unsigned int width = 0);
	void add_lines(const std::vector<std::string>& lines,
		unsigned int width = 0);
	void set_line(const unsigned int itempos,
		const std::string& text,
		unsigned int id = UINT_MAX,
		unsigned int width = 0);
	void clear()
	{
		lines.clear();
	}
	std::string format_list(regexmanager* r = nullptr,
		const std::string& location = "");
	unsigned int get_lines_count()
	{
		return lines.size();
	}

private:
	std::vector<line_id_pair> lines;
	std::string format_cache;
};

} // namespace newsboat

#endif /* NEWSBOAT_LISTFORMATTER_H_ */

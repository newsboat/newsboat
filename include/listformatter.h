#ifndef NEWSBOAT_LISTFORMATTER_H_
#define NEWSBOAT_LISTFORMATTER_H_

#include <climits>
#include <string>
#include <utility>
#include <vector>

#include "regexmanager.h"

namespace newsboat {

class ListFormatter {
	typedef std::pair<std::string, std::string> LineIdPair;

public:
	ListFormatter(RegexManager* r = nullptr, const std::string& loc = "");
	~ListFormatter();
	void add_line(const std::string& text,
		const std::string& id = "",
		unsigned int width = 0);
	void add_lines(const std::vector<std::string>& lines,
		unsigned int width = 0);
	void set_line(const unsigned int itempos,
		const std::string& text,
		const std::string& id = "",
		unsigned int width = 0);
	void clear()
	{
		lines.clear();
	}
	std::string format_list() const;
	unsigned int get_lines_count() const
	{
		return lines.size();
	}

private:
	std::vector<LineIdPair> lines;
	RegexManager* rxman;
	std::string location;
};

} // namespace newsboat

#endif /* NEWSBOAT_LISTFORMATTER_H_ */

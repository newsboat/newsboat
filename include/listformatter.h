#ifndef NEWSBOAT_LISTFORMATTER_H_
#define NEWSBOAT_LISTFORMATTER_H_

#include <climits>
#include <string>
#include <utility>
#include <vector>

#include "regexmanager.h"
#include "utf8string.h"

namespace newsboat {

class ListFormatter {
public:
	ListFormatter(RegexManager* r = nullptr, const std::string& loc = "");
	~ListFormatter();
	void add_line(const std::string& text);
	void set_line(const unsigned int itempos,
		const std::string& text);
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
	std::vector<Utf8String> lines;
	RegexManager* rxman;
	Utf8String location;
};

} // namespace newsboat

#endif /* NEWSBOAT_LISTFORMATTER_H_ */

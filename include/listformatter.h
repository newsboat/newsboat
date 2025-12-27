#ifndef NEWSBOAT_LISTFORMATTER_H_
#define NEWSBOAT_LISTFORMATTER_H_

#include <optional>
#include <string>
#include <vector>

#include "dialog.h"
#include "regexmanager.h"
#include "stflrichtext.h"

namespace newsboat {

class ListFormatter {
public:
	explicit ListFormatter(RegexManager* r = nullptr, std::optional<Dialog> loc = {});
	void add_line(const StflRichText& text);
	void set_line(const unsigned int itempos, const StflRichText& text);
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
	std::vector<StflRichText> lines;
	RegexManager* rxman;
	std::optional<Dialog> location;
};

} // namespace newsboat

#endif /* NEWSBOAT_LISTFORMATTER_H_ */

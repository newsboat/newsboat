#ifndef NEWSBOAT_TEXTFORMATTER_H_
#define NEWSBOAT_TEXTFORMATTER_H_

#include <climits>
#include <string>
#include <utility>
#include <vector>

#include "regexmanager.h"

namespace newsboat {

/*
 * LineType specifies the way wrapping should be handled.
 *
 * wrappable: Wrap lines at the user-specified text-width setting, if not set
 * wrap at the window border.
 *
 * softwrappable: Wrap at the window border
 *
 * nonwrappable: Don't wrap lines, characters that cannot be drawn due to
 *               insufficient window width will be ignored.
 */

enum class LineType { wrappable = 1, softwrappable, nonwrappable, hr };

class textformatter {
public:
	textformatter();
	~textformatter();
	void add_line(LineType type, std::string line);
	void add_lines(
		const std::vector<std::pair<LineType, std::string>>& lines);
	std::pair<std::string, std::size_t> format_text_to_list(
		regexmanager* r = nullptr,
		const std::string& location = "",
		const size_t wrap_width = 80,
		const size_t total_width = 0);
	std::string format_text_plain(const size_t width = 80,
		const size_t total_width = 0);

	void clear()
	{
		lines.clear();
	}

private:
	std::vector<std::pair<LineType, std::string>> lines;
};

} // namespace newsboat

#endif /* NEWSBOAT_TEXTFORMATTER_H_ */

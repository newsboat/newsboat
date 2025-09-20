#ifndef NEWSBOAT_TEXTFORMATTER_H_
#define NEWSBOAT_TEXTFORMATTER_H_

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "dialog.h"

namespace newsboat {

class RegexManager;

/// This type dictates how the line should be wrapped and/or rendered.
enum class LineType {
	/// Wrap the line at the user-specified text-width setting. If the setting
	/// is not set, wrap at the window border.
	wrappable = 1,

	/// Wrap the line at the window border.
	softwrappable,

	/// Do not wrap. The part of the line that doesn't fit into the window will
	/// be ignored.
	nonwrappable,

	/// Render as a horizontal line `text-width` characters long. If the
	/// setting is not set, the line will span the entire width of the window.
	hr
};

class TextFormatter {
public:
	TextFormatter() = default;
	~TextFormatter() = default;
	void add_line(LineType type, std::string line);
	void add_lines(
		const std::vector<std::pair<LineType, std::string>>& lines);
	std::pair<std::string, std::size_t> format_text_to_list(
		RegexManager* r = nullptr,
		std::optional<Dialog> location = {},
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

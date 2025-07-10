#ifndef NEWSBOAT_TEXTVIEWWIDGET_H_
#define NEWSBOAT_TEXTVIEWWIDGET_H_

#include <cstdint>
#include <string>

#include "stflpp.h"

namespace Newsboat {

class TextviewWidget {
public:
	TextviewWidget(const std::string& textview_name,
		Stfl::Form& form);
	void stfl_replace_textview(std::uint32_t number_of_lines, std::string stfl);
	void stfl_replace_lines(std::uint32_t number_of_lines, std::string stfl);

	void scroll_up();
	void scroll_down();
	void scroll_to_top();
	void scroll_to_bottom();
	void scroll_page_up();
	void scroll_page_down();
	void scroll_halfpage_up();
	void scroll_halfpage_down();

	std::uint32_t get_scroll_offset();
	void set_scroll_offset(std::uint32_t offset);

	std::uint32_t get_width();
	std::uint32_t get_height();
private:
	const std::string textview_name;
	Stfl::Form& form;
	std::uint32_t num_lines;
};

} // namespace Newsboat

#endif /* NEWSBOAT_TEXTVIEWWIDGET_H_ */

#ifndef NEWSBOAT_TEXTVIEWWIDGET_H_
#define NEWSBOAT_TEXTVIEWWIDGET_H_

#include <memory>
#include <string>

#include "stflpp.h"

namespace newsboat {

class TextviewWidget {
public:
	TextviewWidget(const std::string& textview_name,
		std::shared_ptr<Stfl::Form> form);
	void set_lines(uint32_t number_of_lines);

	void scroll_up();
	void scroll_down();
	void scroll_to_top();
	void scroll_to_bottom();
	void scroll_page_up();
	void scroll_page_down();

	uint32_t get_scroll_offset();
	void set_scroll_offset(uint32_t offset);

	uint32_t get_width();
	uint32_t get_height();
private:
	const std::string textview_name;
	std::shared_ptr<Stfl::Form> form;
	uint32_t num_lines;
};

} // namespace newsboat

#endif /* NEWSBOAT_TEXTVIEWWIDGET_H_ */

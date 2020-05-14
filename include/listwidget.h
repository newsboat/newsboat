#ifndef NEWSBOAT_LISTWIDGET_H_
#define NEWSBOAT_LISTWIDGET_H_

#include <cstdint>
#include <memory>
#include <string>

#include "stflpp.h"

namespace newsboat {

class ListWidget {
public:
	ListWidget(const std::string& list_name, Stfl::Form& form);
	void stfl_replace_list(std::uint32_t number_of_lines, std::string stfl);
	void stfl_replace_lines(std::uint32_t number_of_lines, std::string stfl);

	bool move_up(bool wrap_scroll);
	bool move_down(bool wrap_scroll);
	void move_to_first();
	void move_to_last();
	void move_page_up(bool wrap_scroll);
	void move_page_down(bool wrap_scroll);

	std::uint32_t get_position();
	void set_position(std::uint32_t pos);

	std::uint32_t get_width();
	std::uint32_t get_height();
private:
	const std::string list_name;
	Stfl::Form& form;
	std::uint32_t num_lines;
};

} // namespace newsboat

#endif /* NEWSBOAT_LISTWIDGET_H_ */

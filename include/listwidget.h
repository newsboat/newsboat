#ifndef NEWSBOAT_LISTWIDGET_H_
#define NEWSBOAT_LISTWIDGET_H_

#include <cstdint>
#include <memory>
#include <string>

#include "listformatter.h"
#include "stflpp.h"

namespace newsboat {

class ListWidget {
public:
	ListWidget(const std::string& list_name, Stfl::Form& form,
		std::uint32_t scrolloff);
	void stfl_replace_list(std::uint32_t number_of_lines, std::string stfl);
	void stfl_replace_lines(const ListFormatter& listfmt);

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
	std::uint32_t get_scroll_offset();
	void set_scroll_offset(std::uint32_t pos);

	void update_scroll_offset(std::uint32_t pos);

	const std::string list_name;
	Stfl::Form& form;
	std::uint32_t num_lines;
	std::uint32_t num_context_lines;
};

} // namespace newsboat

#endif /* NEWSBOAT_LISTWIDGET_H_ */

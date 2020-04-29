#ifndef NEWSBOAT_LISTWIDGET_H_
#define NEWSBOAT_LISTWIDGET_H_

#include <memory>
#include <string>

#include "stflpp.h"

namespace newsboat {

class ListWidget {
public:
	ListWidget(const std::string& list_name, std::shared_ptr<Stfl::Form> form);
	void set_lines(uint32_t number_of_lines);

	bool move_up();
	bool move_down();
	void move_to_first();
	void move_to_last();
	void move_page_up();
	void move_page_down();

	uint32_t get_position();
	void set_position(uint32_t pos);

	uint32_t get_width();
	uint32_t get_height();
private:
	const std::string list_name;
	std::shared_ptr<Stfl::Form> form;
	uint32_t num_lines;
};

} // namespace newsboat

#endif /* NEWSBOAT_LISTWIDGET_H_ */

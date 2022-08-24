#ifndef NEWSBOAT_LISTWIDGETBACKEND_H_
#define NEWSBOAT_LISTWIDGETBACKEND_H_

#include <string>

#include "listformatter.h"
#include "stflpp.h"

namespace newsboat {

class ListWidgetBackend {
public:
	ListWidgetBackend(const std::string& list_name, Stfl::Form& form);
	virtual ~ListWidgetBackend() = default;

	void stfl_replace_list(std::uint32_t number_of_lines, std::string stfl);
	void stfl_replace_lines(const ListFormatter& listfmt);

	std::uint32_t get_width();
	std::uint32_t get_height();
	std::uint32_t get_num_lines();

	void update_position(std::uint32_t pos, std::uint32_t scroll_offset);

protected:
	virtual void on_list_changed() = 0;

private:
	const std::string list_name;
	Stfl::Form& form;
	std::uint32_t num_lines;
};

} // namespace newsboat

#endif /* NEWSBOAT_LISTWIDGETBACKEND_H_ */

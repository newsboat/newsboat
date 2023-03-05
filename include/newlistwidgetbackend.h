#ifndef NEWSBOAT_NEWLISTWIDGETBACKEND_H_
#define NEWSBOAT_NEWLISTWIDGETBACKEND_H_

#include <cstdint>
#include <functional>
#include <map>
#include <string>

#include "regexmanager.h"
#include "stflpp.h"

namespace newsboat {

class NewListWidgetBackend {
public:
	NewListWidgetBackend(const std::string& list_name, const std::string& context,
		Stfl::Form& form, RegexManager& rxman);
	virtual ~NewListWidgetBackend() = default;

	void stfl_replace_list(std::string stfl);

	std::uint32_t get_width();
	std::uint32_t get_height();
	std::uint32_t get_num_lines();

	void invalidate_list_content(std::uint32_t num_lines,
		std::function<std::string(std::uint32_t, std::uint32_t)> get_line_method);

protected:
	virtual void on_list_changed() = 0;
	void update_position(std::uint32_t pos, std::uint32_t scroll_offset);

private:
	void render();

	const std::string list_name;
	std::string context;
	Stfl::Form& form;
	RegexManager& rxman;
	std::uint32_t num_lines;
	std::uint32_t scroll_offset;
	std::function<std::string(std::uint32_t, std::uint32_t)> get_formatted_line;
};

} // namespace newsboat

#endif /* NEWSBOAT_NEWLISTWIDGETBACKEND_H_ */

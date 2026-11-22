#ifndef NEWSBOAT_LISTWIDGETBACKEND_H_
#define NEWSBOAT_LISTWIDGETBACKEND_H_

#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <string>

#include "dialog.h"
#include "listformatter.h"
#include "regexmanager.h"
#include "stflpp.h"
#include "stflrichtext.h"

namespace newsboat {

class ListWidgetBackend {
public:
	ListWidgetBackend(const std::string& list_name, Dialog context,
		Stfl::Form& form, RegexManager& rxman);
	ListWidgetBackend(const std::string& list_name, Stfl::Form& form);
	virtual ~ListWidgetBackend() = default;

	void stfl_replace_list(std::string stfl);

	std::uint32_t get_width();
	std::uint32_t get_height();
	std::uint32_t get_num_lines();

	void invalidate_list_content(std::uint32_t num_lines,
		std::function<StflRichText(std::uint32_t, std::uint32_t)> get_line_method);

protected:
	virtual void on_list_changed() = 0;
	void update_position(std::uint32_t pos, std::uint32_t scroll_offset);

private:
	void render();

	const std::string list_name;
	Stfl::Form& form;
	ListFormatter listfmt;
	std::uint32_t num_lines;
	std::uint32_t scroll_offset;
	std::map<std::uint32_t, StflRichText> line_cache;
	std::function<StflRichText(std::uint32_t, std::uint32_t)> get_formatted_line;

	// Reload threads call invalidate_list_content() (through
	// View::set_feedlist()) while the UI thread moves the cursor, which calls
	// update_position() and render(). Every method takes this lock, so the
	// members above are never read and written at the same time. It is
	// recursive because invalidate_list_content() calls on_list_changed(),
	// which calls update_position() again.
	std::recursive_mutex mtx;
};

} // namespace newsboat

#endif /* NEWSBOAT_LISTWIDGETBACKEND_H_ */

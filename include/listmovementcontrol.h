#ifndef NEWSBOAT_LISTMOVEMENTCONTROL_H_
#define NEWSBOAT_LISTMOVEMENTCONTROL_H_

#include <cstdint>
#include <string>

#include "stflpp.h"

namespace Newsboat {

class RegexManager;

// Backend is required to implement:
// std::uint32_t get_height()
// std::uint32_t get_num_lines()
// void update_position(std::uint32_t pos, std::uint32_t scroll_offset)
// virtual void on_list_changed()

template<class Backend>
class ListMovementControl : public Backend {
public:
	ListMovementControl(const std::string& list_name, Stfl::Form& form,
		std::uint32_t scrolloff)
		: Backend(list_name, form)
	{
		num_context_lines = scrolloff;
		set_position(current_position);
	}

	ListMovementControl(const std::string& list_name, const std::string& context,
		Stfl::Form& form, RegexManager& rxman, std::uint32_t scrolloff)
		: Backend(list_name, context, form, rxman)
	{
		num_context_lines = scrolloff;
		set_position(current_position);
	}

	// Make sure the backend's destructor is marked virtual
	~ListMovementControl() override = default;

	bool move_up(bool wrap_scroll)
	{
		if (current_position > 0) {
			set_position(current_position - 1);
			return true;
		} else if (wrap_scroll) {
			move_to_last();
			return true;
		}

		return false;
	}

	bool move_down(bool wrap_scroll)
	{
		const auto num_lines = Backend::get_num_lines();
		if (num_lines == 0) {
			// Ignore if list is empty
			return false;
		}

		const std::uint32_t maxpos = num_lines - 1;
		if (current_position + 1 <= maxpos) {
			set_position(current_position + 1);
			return true;
		} else if (wrap_scroll) {
			move_to_first();
			return true;
		}

		return false;
	}

	void move_to_first()
	{
		set_position(0);
	}

	void move_to_last()
	{
		const auto num_lines = Backend::get_num_lines();
		if (num_lines == 0) {
			// Ignore if list is empty
			return;
		}

		const std::uint32_t maxpos = num_lines - 1;
		set_position(maxpos);
	}

	void move_page_up(bool wrap_scroll)
	{
		const std::uint32_t list_height = Backend::get_height();
		if (current_position > list_height) {
			set_position(current_position - list_height);
		} else if (wrap_scroll && current_position == 0) {
			move_to_last();
		} else {
			set_position(0);
		}
	}

	void move_page_down(bool wrap_scroll)
	{
		const auto num_lines = Backend::get_num_lines();
		if (num_lines == 0) {
			// Ignore if list is empty
			return;
		}

		const std::uint32_t maxpos = num_lines - 1;
		const std::uint32_t list_height = Backend::get_height();

		if (current_position + list_height < maxpos) {
			set_position(current_position + list_height);
		} else if (wrap_scroll && current_position == maxpos) {
			move_to_first();
		} else {
			set_position(maxpos);
		}
	}

	void scroll_halfpage_up(bool wrap_scroll)
	{
		const std::uint32_t list_height = Backend::get_height();
		const std::uint32_t scroll_amount = (list_height + 1) / 2;

		if (current_scroll_offset >= scroll_amount) {
			current_scroll_offset -= scroll_amount;
		} else {
			current_scroll_offset = 0;
		}

		if (wrap_scroll && current_position == 0) {
			move_to_last();
		} else if (current_position >= scroll_amount) {
			set_position(current_position - scroll_amount);
		} else {
			set_position(0);
		}
	}

	void scroll_halfpage_down(bool wrap_scroll)
	{
		const auto num_lines = Backend::get_num_lines();
		if (num_lines == 0) {
			// Ignore if list is empty
			return;
		}

		const std::uint32_t maxpos = num_lines - 1;
		const std::uint32_t list_height = Backend::get_height();
		const std::uint32_t scroll_amount = (list_height + 1) / 2;

		if (current_scroll_offset + scroll_amount <= max_offset()) {
			current_scroll_offset += scroll_amount;
		} else {
			current_scroll_offset = max_offset();
		}

		if (wrap_scroll && current_position == maxpos) {
			move_to_first();
		} else if (current_position + scroll_amount <= maxpos) {
			set_position(current_position + scroll_amount);
		} else {
			set_position(maxpos);
		}
	}

	std::uint32_t get_position()
	{
		return current_position;
	}

	void set_position(std::uint32_t pos)
	{
		// TODO: Check if in valid range?
		current_position = pos;
		current_scroll_offset = std::min(get_new_scroll_offset(pos), max_offset());
		Backend::update_position(current_position, current_scroll_offset);
	}

private:
	void on_list_changed() override
	{
		const auto num_lines = Backend::get_num_lines();
		if (current_position >= num_lines) {
			if (num_lines > 0) {
				set_position(num_lines - 1);
			} else {
				set_position(0);
			}
		} else {
			// Make sure position and scroll_offset are both correct after list changes/resizes
			set_position(current_position);
		}
	}

	std::uint32_t max_offset()
	{
		const auto h = Backend::get_height();
		const auto num_lines = Backend::get_num_lines();
		// An offset at which the last item of the list is at the bottom of the
		// widget. We shouldn't set "offset" to more than this value, otherwise
		// we'll have an empty "gap" at the bottom of the list. That's only
		// acceptable if the list is shorter than the widget's height.
		if (num_lines >= h) {
			return num_lines - h;
		} else {
			return 0;
		}
	}

	std::uint32_t get_new_scroll_offset(std::uint32_t pos)
	{
		// In STFL, "offset" is how many items at the beginning of the list are
		// hidden off-screen. That's how scrolling is implemented: to scroll down,
		// you increase "offset", hiding items at the top and showing more at the
		// bottom. By manipulating "offset" here, we can keep the cursor within the
		// bounds we set.
		//
		// All the lines that are visible because of "scrolloff" setting are called
		// "context" here. They include the current line under cursor (which has
		// position "pos"), "num_context_lines" lines above it, and
		// "num_context_lines" lines below it.

		const auto h = Backend::get_height();

		if (2 * num_context_lines < h) {
			// Check if items at the bottom of the "context" are visible. If not,
			// we'll have to scroll down.
			if (pos + num_context_lines >= current_scroll_offset + h) {
				if (pos + num_context_lines >= h) {
					const std::uint32_t target_offset = pos + num_context_lines - h + 1;
					return target_offset;
				} else { // "pos" is towards the beginning of the list; don't scroll
					return 0;
				}
			}

			// Check if items at the top of the "context" are visible. If not,
			// we'll have to scroll up.
			if (pos < current_scroll_offset + num_context_lines) {
				if (pos >= num_context_lines) {
					const std::uint32_t target_offset = pos - num_context_lines;
					return target_offset;
				} else { // "pos" is towards the beginning of the list; don't scroll
					return 0;
				}
			}

			return current_scroll_offset;
		} else { // Keep selected item in the middle
			if (pos > h / 2) {
				const std::uint32_t target_offset = pos - h / 2;
				return target_offset;
			} else { // "pos" is towards the beginning of the list; don't scroll
				return 0;
			}
		}
	}

private:
	std::uint32_t num_context_lines {};

	std::uint32_t current_position {};
	std::uint32_t current_scroll_offset {};
};

} // namespace Newsboat

#endif /* NEWSBOAT_LISTWIDGETBACKEND_H_ */

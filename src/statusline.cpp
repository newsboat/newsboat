#include "statusline.h"

#include <algorithm>

namespace Newsboat {

StatusLine::StatusLine(IStatus& s)
	: next_message_id(0)
	, active_message(0)
	, iStatus(s)
{
}

void StatusLine::show_message(const std::string& message)
{
	std::lock_guard<std::mutex> guard(m);

	iStatus.set_status(message);
	active_message = 0;
}

void StatusLine::show_error(const std::string& message)
{
	std::lock_guard<std::mutex> guard(m);

	iStatus.show_error(message);
	active_message = 0;
}

std::shared_ptr<AutoDiscardMessage> StatusLine::show_message_until_finished(
	const std::string& message)
{
	std::lock_guard<std::mutex> guard(m);

	// Make sure we don't store an active message with ID 0.  This allows us to
	// only restore a message in `mark_finished()` if the last status was set
	// using `show_message_until_finished()`.
	if (next_message_id == 0) {
		next_message_id++;
	}
	active_messages.push_back(std::make_pair(next_message_id, message));
	iStatus.set_status(message);
	active_message = next_message_id;
	// It is highly unlikely for `next_message_id` (an std::uint32_t) to wrap
	// around as that would require over 4 billion updates to the status.
	// If it does wrap around, the worst that can happen is that an existing
	// status is removed from the `active_messages` list a bit too early.
	next_message_id++;
	return std::make_shared<AutoDiscardMessage>(*this, active_message);
}

void StatusLine::mark_finished(std::uint32_t message_id)
{
	std::lock_guard<std::mutex> guard(m);

	active_messages.erase(
		std::remove_if(active_messages.begin(),
	active_messages.end(), [&](std::pair<std::uint32_t, std::string> x) {
		return x.first == message_id;
	}));

	if (active_message == message_id) {
		if (active_messages.empty()) {
			iStatus.set_status("");
		} else {
			active_message = active_messages.back().first;
			iStatus.set_status(active_messages.back().second);
		}
	}
}

AutoDiscardMessage::AutoDiscardMessage(StatusLine& s, std::uint32_t m_id)
	: status_line(s)
	, message_id(m_id)
{
}

AutoDiscardMessage::~AutoDiscardMessage()
{
	status_line.mark_finished(message_id);
}

} // namespace Newsboat

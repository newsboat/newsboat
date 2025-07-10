#ifndef NEWSBOAT_STATUSLINE_H_
#define NEWSBOAT_STATUSLINE_H_

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace Newsboat {

// StatusLine manages the array of messages, IStatus abstracts how messages are
// actually displayed, and AutoDiscardMessage is a RAII helper that removes its
// message when it's deconstructed

class IStatus {
public:
	virtual void set_status(const std::string& message) = 0;
	virtual void show_error(const std::string& message) = 0;
};

class AutoDiscardMessage;

class StatusLine {
public:
	explicit StatusLine(IStatus& s);

	void show_message(const std::string& message);
	void show_error(const std::string& message);

	std::shared_ptr<AutoDiscardMessage> show_message_until_finished(
		const std::string& message);
	void mark_finished(std::uint32_t message_id);

private:
	std::vector<std::pair<std::uint32_t, std::string>> active_messages;
	std::uint32_t next_message_id;
	std::uint32_t active_message;

	IStatus& iStatus;

	mutable std::mutex m;
};

class AutoDiscardMessage {
public:
	AutoDiscardMessage(StatusLine& s, std::uint32_t m_id);
	~AutoDiscardMessage();

private:
	StatusLine& status_line;
	const std::uint32_t message_id;
};

} // namespace Newsboat

#endif /* NEWSBOAT_STATUSLINE_H_ */

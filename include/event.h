#ifndef NEWSBOAT_EVENT_H_
#define NEWSBOAT_EVENT_H_

#include <optional>
#include <string>

namespace newsboat {

struct Event {
	std::string name{};
	std::optional<std::string> printableCharacter;
};

} // namespace newsboat

#endif /* NEWSBOAT_EVENT_H_ */

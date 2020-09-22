#ifndef NEWSBOAT_MATCHABLE_H_
#define NEWSBOAT_MATCHABLE_H_

#include <string>

#include "3rd-party/optional.hpp"

namespace newsboat {

class Matchable {
public:
	Matchable() = default;
	virtual ~Matchable() = default;
	virtual nonstd::optional<std::string> attribute_value(const std::string& attr)
	const =
		0;
};

} // namespace newsboat

#endif /* NEWSBOAT_MATCHABLE_H_ */


#ifndef NEWSBOAT_MATCHABLE_H_
#define NEWSBOAT_MATCHABLE_H_

#include <optional>
#include <string>

namespace Newsboat {

class Matchable {
public:
	Matchable() = default;
	virtual ~Matchable() = default;
	virtual std::optional<std::string> attribute_value(const std::string& attr)
	const =
		0;
};

} // namespace Newsboat

#endif /* NEWSBOAT_MATCHABLE_H_ */


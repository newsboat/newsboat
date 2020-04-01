#ifndef NEWSBOAT_MATCHABLE_H_
#define NEWSBOAT_MATCHABLE_H_

#include <string>

namespace newsboat {

class Matchable {
public:
	Matchable() = default;
	virtual ~Matchable() = default;
	virtual bool has_attribute(const std::string& attribname) = 0;
	virtual std::string get_attribute(const std::string& attribname) = 0;
};

} // namespace newsboat

#endif /* NEWSBOAT_MATCHABLE_H_ */


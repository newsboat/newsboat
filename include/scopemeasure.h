#ifndef NEWSBOAT_SCOPEMEASURE_H_
#define NEWSBOAT_SCOPEMEASURE_H_

#include <chrono>
#include <string>

#include "logger.h"

namespace newsboat {

class ScopeMeasure {
public:
	ScopeMeasure(const std::string& func, Level ll = Level::DEBUG);
	~ScopeMeasure();
	void stopover(const std::string& son = "");

private:
	std::chrono::time_point<std::chrono::steady_clock> start_time;
	std::string funcname;
	Level lvl = Level::DEBUG;
};

} // namespace newsboat

#endif /* NEWSBOAT_SCOPEMEASURE_H_ */

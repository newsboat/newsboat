#ifndef NEWSBOAT_SCOPEMEASURE_H_
#define NEWSBOAT_SCOPEMEASURE_H_

#include <sys/time.h>
#include <string>

#include "logger.h"

namespace newsboat {

class ScopeMeasure {
public:
	ScopeMeasure(const std::string& func, Level ll = Level::DEBUG);
	~ScopeMeasure();
	void stopover(const std::string& son = "");

private:
	// Can't use zero initialization (= {}) here because it fails with GCC 4.9:
	// https://travis-ci.org/newsboat/newsboat/jobs/557484737#L760
	struct timeval tv1 = {0, 0}, tv2 = {0, 0};
	std::string funcname;
	Level lvl = Level::DEBUG;
};

} // namespace newsboat

#endif /* NEWSBOAT_SCOPEMEASURE_H_ */

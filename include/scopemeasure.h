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
	struct timeval tv1 = {}, tv2 = {};
	std::string funcname;
	Level lvl = Level::DEBUG;
};

} // namespace newsboat

#endif /* NEWSBOAT_SCOPEMEASURE_H_ */

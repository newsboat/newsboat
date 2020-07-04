#ifndef NEWSBOAT_SCOPEMEASURE_H_
#define NEWSBOAT_SCOPEMEASURE_H_

#include <string>

#include "logger.h"

namespace newsboat {

class ScopeMeasure {
public:
	ScopeMeasure(const std::string& func, Level ll = Level::DEBUG);
	~ScopeMeasure();
	void stopover(const std::string& son = "");

private:
	void* rs_object = nullptr;
};

} // namespace newsboat

#endif /* NEWSBOAT_SCOPEMEASURE_H_ */

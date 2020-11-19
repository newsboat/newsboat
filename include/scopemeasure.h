#ifndef NEWSBOAT_SCOPEMEASURE_H_
#define NEWSBOAT_SCOPEMEASURE_H_

#include <string>

#include "scopemeasure.rs.h"

namespace newsboat {

class ScopeMeasure {
public:
	ScopeMeasure(const std::string& func);
	~ScopeMeasure() = default;
	void stopover(const std::string& son = "");

private:
	rust::Box<scopemeasure::bridged::ScopeMeasure> rs_object;
};

} // namespace newsboat

#endif /* NEWSBOAT_SCOPEMEASURE_H_ */

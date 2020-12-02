#include "scopemeasure.h"

namespace newsboat {

ScopeMeasure::ScopeMeasure(const std::string& func)
	: rs_object(scopemeasure::bridged::create(func))
{
}

void ScopeMeasure::stopover(const std::string& son)
{
	rs_object->stopover(son);
}

} // namespace newsboat

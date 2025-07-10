#include "scopemeasure.h"

namespace Newsboat {

ScopeMeasure::ScopeMeasure(const std::string& func)
	: rs_object(scopemeasure::bridged::create(func))
{
}

void ScopeMeasure::stopover(const std::string& son)
{
	scopemeasure::bridged::stopover(*rs_object, son);
}

} // namespace Newsboat

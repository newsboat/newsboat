#include "scopemeasure.h"

extern "C" {
	void* create_rs_scopemeasure(const char* scope_name);
	void destroy_rs_scopemeasure(void* object);
	void rs_scopemeasure_stopover(void* object, const char* stopover_name);
}

namespace newsboat {

ScopeMeasure::ScopeMeasure(const std::string& func)
{
	rs_object = create_rs_scopemeasure(func.c_str());
}

void ScopeMeasure::stopover(const std::string& son)
{
	rs_scopemeasure_stopover(rs_object, son.c_str());
}

ScopeMeasure::~ScopeMeasure()
{
	destroy_rs_scopemeasure(rs_object);
}

} // namespace newsboat

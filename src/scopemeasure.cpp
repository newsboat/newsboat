#include "scopemeasure.h"

extern "C" {
	void* create_rs_scopemeasure(const char* scope_name, newsboat::Level log_level);
	void destroy_rs_scopemeasure(void* object);
	void rs_scopemeasure_stopover(void* object, const char* stopover_name);
}

namespace newsboat {

ScopeMeasure::ScopeMeasure(const std::string& func, Level ll)
{
	rs_object = create_rs_scopemeasure(func.c_str(), ll);
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

#include "scopemeasure.h"

#include <cinttypes>

using fpseconds = std::chrono::duration<double>;

namespace newsboat {

ScopeMeasure::ScopeMeasure(const std::string& func, Level ll)
	: funcname(func)
	, lvl(ll)
{
	start_time = std::chrono::steady_clock::now();
}

void ScopeMeasure::stopover(const std::string& son)
{
	using namespace std::chrono;

	const auto now = steady_clock::now();
	const auto diff = duration_cast<fpseconds>(now - start_time).count();
	LOG(lvl,
		"ScopeMeasure: function `%s' (stop over `%s') took %.6f s so far",
		funcname,
		son,
		diff);
}

ScopeMeasure::~ScopeMeasure()
{
	using namespace std::chrono;

	const auto now = steady_clock::now();
	const auto diff = duration_cast<fpseconds>(now - start_time).count();
	LOG(lvl,
		"ScopeMeasure: function `%s' took %.6f s",
		funcname,
		diff);
}

} // namespace newsboat

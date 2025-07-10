#ifndef NEWSBOAT_SCOPEMEASURE_H_
#define NEWSBOAT_SCOPEMEASURE_H_

#include <string>

#include "libNewsboat-ffi/src/scopemeasure.rs.h" // IWYU pragma: export

namespace Newsboat {

class ScopeMeasure {
public:
	explicit ScopeMeasure(const std::string& func);
	~ScopeMeasure() = default;
	void stopover(const std::string& son = "");

private:
	rust::Box<scopemeasure::bridged::ScopeMeasure> rs_object;
};

} // namespace Newsboat

#endif /* NEWSBOAT_SCOPEMEASURE_H_ */

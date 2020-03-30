#ifndef NEWSBOAT_TEST_HELPERS_CHMOD_H_
#define NEWSBOAT_TEST_HELPERS_CHMOD_H_

#include <string>
#include <sys/stat.h>

namespace TestHelpers {

/// Sets new permissions on a given path, and restores them back when the
/// object is destroyed.
class Chmod {
	std::string m_path;
	mode_t m_originalMode;

public:
	Chmod(const std::string& path, mode_t newMode);

	~Chmod();
};

} // namespace TestHelpers

#endif /* NEWSBOAT_TEST_HELPERS_CHMOD_H_ */

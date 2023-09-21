#ifndef NEWSBOAT_TEST_HELPERS_CHMOD_H_
#define NEWSBOAT_TEST_HELPERS_CHMOD_H_

#include "filepath.h"
#include <sys/stat.h>

namespace test_helpers {

/// Sets new permissions on a given path, and restores them back when the
/// object is destroyed.
class Chmod {
	newsboat::Filepath m_path;
	mode_t m_originalMode;

public:
	Chmod(const newsboat::Filepath& path, mode_t newMode);

	~Chmod();
};

} // namespace test_helpers

#endif /* NEWSBOAT_TEST_HELPERS_CHMOD_H_ */

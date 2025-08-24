#ifndef NEWSBOAT_TEST_HELPERS_CHDIR_H_
#define NEWSBOAT_TEST_HELPERS_CHDIR_H_

#include "filepath.h"

namespace test_helpers {

/// Changes current working directory and restores it back when the object is
/// destroyed.
class Chdir {
	newsboat::Filepath m_old_path;

public:
	Chdir(const newsboat::Filepath& path);

	~Chdir();
};

} // namespace test_helpers

#endif /* NEWSBOAT_TEST_HELPERS_CHDIR_H_ */

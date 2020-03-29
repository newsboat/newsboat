#ifndef NEWSBOAT_TEST_HELPERS_CHDIR_H_
#define NEWSBOAT_TEST_HELPERS_CHDIR_H_

#include <string>

namespace TestHelpers {

/// Changes current working directory and restores it back when the object is
/// destroyed.
class Chdir {
	std::string m_old_path;

public:
	Chdir(const std::string& path);

	~Chdir();
};

} // namespace TestHelpers

#endif /* NEWSBOAT_TEST_HELPERS_CHDIR_H_ */

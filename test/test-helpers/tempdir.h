#ifndef NEWSBOAT_TEST_HELPERS_TEMPDIR_H_
#define NEWSBOAT_TEST_HELPERS_TEMPDIR_H_

#include <string>

#include "maintempdir.h"

namespace TestHelpers {

/* Objects of TempDir class create a temporary directory and remove it (along
 * with everything it contains) when the object is destructed. */
class TempDir {
public:
	TempDir();

	~TempDir();

	const std::string get_path() const;

private:
	MainTempDir tempdir;
	std::string dirpath;
};

} // namespace TestHelpers

#endif /* NEWSBOAT_TEST_HELPERS_TEMPDIR_H_ */

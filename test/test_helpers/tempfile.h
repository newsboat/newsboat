#ifndef NEWSBOAT_TEST_HELPERS_TEMPFILE_H_
#define NEWSBOAT_TEST_HELPERS_TEMPFILE_H_

#include "filepath.h"
#include "maintempdir.h"

namespace test_helpers {

/* Objects of TempFile class generate a temporary filename and delete the
 * corresponding file when they are destructed.
 *
 * This is useful for teardown in tests, where we use RAII to clean up
 * temporary DB files and stuff. */
class TempFile {
public:
	TempFile();

	~TempFile();

	newsboat::Filepath get_path() const;

private:
	MainTempDir tempdir;
	newsboat::Filepath filepath;
};

} // namespace test_helpers

#endif /* NEWSBOAT_TEST_HELPERS_TEMPFILE_H_ */

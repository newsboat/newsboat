#ifndef TEST_HELPERS_H_
#define TEST_HELPERS_H_

#include <cstdio>
#include <unistd.h>

namespace TestHelpers {

	/* Objects of TempFile class generate a temporary filename and delete the
	 * corresponding file when they are destructed.
	 *
	 * This is useful for teardown in tests, where we use RAII to clean up
	 * temporary DB files and stuff. */
	class TempFile {
		public:
			class tempfileexception : public std::exception {
				public:
					virtual const char* what() const throw() {
						return "tmpnam() failed to create a temporary filename";
					}
			};

			TempFile() {
				char path[L_tmpnam];
				/* Generate a temporary filename. This line will cause
				 * a warning at the linking state. It's fine: we don't expect
				 * our tests to be run in adverse situations where someone will
				 * try to steal the file from under us; and our testing
				 * framework isn't threaded, so there's no risk of different
				 * TempFile objects racing for a filename or something. */
				if (tmpnam(path)) {
					filepath = path;
				} else {
					throw tempfileexception();
				}
			}

			~TempFile() {
				::unlink(filepath.c_str());
			}

			const std::string getPath() {
				return filepath;
			}

		private:
			std::string filepath;
	};
}

#endif /* TEST_HELPERS_H_ */

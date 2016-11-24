#ifndef TEST_HELPERS_H_
#define TEST_HELPERS_H_

#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <string>
#include <exception>

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
					explicit tempfileexception(const char* error) {
						msg  = "failed to create a tempdir: ";
						msg += error;
					};

					virtual const char* what() const throw() {
						return msg.c_str();
					}

				private:
					std::string msg;
			};

			TempFile() {
				init_tempdir();
				init_file();
			}

			~TempFile() {
				::unlink(filepath.c_str());
				::rmdir(tempdir.c_str());
			}

			const std::string getPath() {
				return filepath;
			}

		private:
			void init_tempdir() {
				char* tmpdir_p = ::getenv("TMPDIR");

				if (tmpdir_p) {
					tempdir = tmpdir_p;
				} else {
					tempdir = "/tmp/";
				}

				tempdir += "/newsbeuter-tests/";

				int status = mkdir(tempdir.c_str(), S_IRWXU);
				if (status != 0) {
					// The directory already exists. That's fine, though, but
					// only as long as it has all the properties we need.

					int saved_errno = errno;
					bool success = false;

					if (saved_errno == EEXIST) {
						struct stat buffer;
						if (lstat(tempdir.c_str(), &buffer) == 0) {
							if (   buffer.st_mode & S_IRUSR
								&& buffer.st_mode & S_IWUSR
								&& buffer.st_mode & S_IXUSR)
							{
								success = true;
							}
						}
					}

					if (!success) {
						throw tempfileexception(strerror(saved_errno));
					}
				}
			};

			void init_file() {
				bool success = false;
				unsigned int tries = 0;

				// Make 10 attempts at generating a filename that doesn't exist
				do {
					tries++;

					// This isn't thread-safe, but we don't care because Catch
					// doesn't let us run tests in multiple threads anyway.
					std::string filename = std::to_string(rand());
					filepath = tempdir + "/" + filename;

					struct stat buffer;
					if (lstat(filepath.c_str(), &buffer) != 0) {
						if (errno == ENOENT) {
							success = true;
						}
					}
				} while (!success && tries < 10);

				if (!success) {
					throw tempfileexception("failed to generate unique filename");
				}
			}

			std::string tempdir;
			std::string filepath;
	};
}

#endif /* TEST_HELPERS_H_ */

#include "maintempdir.h"

#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

test_helpers::MainTempDir::tempfileexception::tempfileexception(
	const newsboat::Filepath& filepath,
	const std::string& error)
	: msg("tempfileexception: " + filepath.display() + ": " + error)
{
};

const char* test_helpers::MainTempDir::tempfileexception::what() const throw()
{
	return msg.c_str();
}

test_helpers::MainTempDir::MainTempDir()
{
	char* tmpdir_p = ::getenv("TMPDIR");

	if (tmpdir_p) {
		tempdir = newsboat::Filepath::from_locale_string(tmpdir_p);
	} else {
		tempdir = newsboat::Filepath::from_locale_string("/tmp");
	}

	tempdir.push(newsboat::Filepath::from_locale_string("newsboat-tests"));

	const auto tempdir_str = tempdir.to_locale_string();
	int status = mkdir(tempdir_str.c_str(), S_IRWXU);
	if (status != 0) {
		// The directory already exists. That's fine, though,
		// but only as long as it has all the properties we
		// need.

		int saved_errno = errno;
		bool success = false;

		if (saved_errno == EEXIST) {
			struct stat buffer;
			if (lstat(tempdir_str.c_str(), &buffer) == 0) {
				if (buffer.st_mode & S_IRUSR &&
					buffer.st_mode & S_IWUSR &&
					buffer.st_mode & S_IXUSR) {
					success = true;
				}
			}
		}

		if (!success) {
			throw tempfileexception(tempdir, strerror(saved_errno));
		}
	}
}

test_helpers::MainTempDir::~MainTempDir()
{
	// Try to remove the tempdir, but don't try *too* hard: there might be
	// other objects still using it. The last one will hopefully delete it.
	const auto tempdir_str = tempdir.to_locale_string();
	::rmdir(tempdir_str.c_str());
}

const std::string test_helpers::MainTempDir::get_path() const
{
	auto result = tempdir.to_locale_string();
	result.push_back('/');
	return result;
}

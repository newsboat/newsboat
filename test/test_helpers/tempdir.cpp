#include "tempdir.h"

#include <cerrno>
#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

test_helpers::TempDir::TempDir()
{
	const auto tempdir_path = tempdir.get_path().to_locale_string();
	std::vector<char> dirpath_template(
		std::begin(tempdir_path), std::end(tempdir_path));
	dirpath_template.push_back('/');
	const std::string dirname_template("tmp.XXXXXX");
	std::copy(dirname_template.begin(), dirname_template.end(),
		std::back_inserter(dirpath_template));
	dirpath_template.push_back('\0');

	const auto result = ::mkdtemp(dirpath_template.data());
	if (result == nullptr) {
		const auto saved_errno = errno;
		std::string msg("TempDir: failed to generate unique directory: (");
		msg += std::to_string(saved_errno);
		msg += ") ";
		msg += ::strerror(saved_errno);
		throw MainTempDir::tempfileexception(newsboat::Filepath(), msg);
	}

	// cned()-1 so we don't copy terminating null byte - std::string
	// doesn't need it
	const std::string dirpath_str(
		dirpath_template.cbegin(), dirpath_template.cend() - 1);
	dirpath = newsboat::Filepath::from_locale_string(dirpath_str);
}

test_helpers::TempDir::~TempDir()
{
	const pid_t pid = ::fork();
	if (pid == -1) {
		// Failed to fork. Oh well, we're in a destructor, so can't throw
		// or do anything else of use. Just give up.
	} else if (pid > 0) {
		// In parent
		// Wait for the child to finish. We don't care about child's exit
		// status, thus nullptr.
		::waitpid(pid, nullptr, 0);
	} else {
		// In child
		// Ignore the return value, because even if the call failed, we
		// can't do anything useful.
		const auto dirpath_str = dirpath.to_locale_string();
		::execlp("rm", "rm", "-rf", dirpath_str.c_str(), (char*)nullptr);
	}
}

newsboat::Filepath test_helpers::TempDir::get_path() const
{
	return dirpath.clone();
}

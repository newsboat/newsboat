#include "tempdir.h"

#include <cerrno>
#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

test_helpers::TempDir::TempDir()
{
	const auto dirpath_template = tempdir.get_path() + "tmp.XXXXXX";
	std::vector<char> dirpath_template_c(
		dirpath_template.cbegin(), dirpath_template.cend());
	dirpath_template_c.push_back('\0');

	const auto result = ::mkdtemp(dirpath_template_c.data());
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
	dirpath = std::string(
			dirpath_template_c.cbegin(), dirpath_template_c.cend() - 1);
	dirpath.push_back('/');
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
		::execlp("rm", "rm", "-rf", dirpath.c_str(), (char*)nullptr);
	}
}

const std::string test_helpers::TempDir::get_path() const
{
	return dirpath;
}

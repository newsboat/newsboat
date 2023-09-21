#include "chdir.h"

#include <cstring>
#include <unistd.h>

#include "utils.h"

test_helpers::Chdir::Chdir(const newsboat::Filepath& path)
{
	m_old_path = newsboat::utils::getcwd();
	const int result = ::chdir(path.to_locale_string().c_str());
	if (result != 0) {
		const auto saved_errno = errno;
		auto msg = std::string("test_helpers::Chdir: ")
			+ "couldn't change current directory to `"
			+ path.display()
			+ "': ("
			+ std::to_string(saved_errno)
			+ ") "
			+ strerror(saved_errno);
		throw std::runtime_error(msg);
	}
}

test_helpers::Chdir::~Chdir()
{
	// Ignore the return value, because even if the call failed, we
	// can't do anything useful.
	const int result = ::chdir(m_old_path.to_locale_string().c_str());
	(void)result;
}

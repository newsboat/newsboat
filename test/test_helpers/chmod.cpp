#include "chmod.h"

#include <cstring>
#include <iostream>
#include <stdexcept>

test_helpers::Chmod::Chmod(const newsboat::Filepath& path, mode_t newMode)
{
	m_path = path.clone();
	const auto throw_error = [this](std::string msg) {
		const auto saved_errno = errno;
		const auto message = std::string("test_helpers::Chmod: ")
			+ msg
			+ " `"
			+ std::string(this->m_path)
			+ "': ("
			+ std::to_string(saved_errno)
			+ ") "
			+ strerror(saved_errno);
		throw std::runtime_error(msg);
	};

	struct stat sb;
	const int result = ::stat(std::string(m_path).c_str(), &sb);
	if (result != 0) {
		throw_error("couldn't obtain current mode for");
	}
	m_originalMode = sb.st_mode;

	if (0 != ::chmod(std::string(m_path).c_str(), newMode)) {
		throw_error("couldn't change the mode for");
	}
}

test_helpers::Chmod::~Chmod()
{
	if (0 != ::chmod(std::string(m_path).c_str(), m_originalMode)) {
		const auto saved_errno = errno;
		std::cerr
				<< "test_helpers::Chmod: couldn't change back the mode for `"
					<< m_path
					<< "': ("
					<< std::to_string(saved_errno)
					<< ") "
					<< strerror(saved_errno)
					<< std::endl;
		::abort();
	}
}

#include "chmod.h"

#include <cstring>
#include <iostream>
#include <stdexcept>

TestHelpers::Chmod::Chmod(const std::string& path, mode_t newMode)
	: m_path(path)
{
	const auto throw_error = [this](std::string msg) {
		const auto saved_errno = errno;
		const auto message = std::string("TestHelpers::Chmod: ")
			+ msg
			+ " `"
			+ this->m_path
			+ "': ("
			+ std::to_string(saved_errno)
			+ ") "
			+ strerror(saved_errno);
		throw std::runtime_error(msg);
	};

	struct stat sb;
	const int result = ::stat(m_path.c_str(), &sb);
	if (result != 0) {
		throw_error("couldn't obtain current mode for");
	}
	m_originalMode = sb.st_mode;

	if (0 != ::chmod(m_path.c_str(), newMode)) {
		throw_error("couldn't change the mode for");
	}
}

TestHelpers::Chmod::~Chmod()
{
	if (0 != ::chmod(m_path.c_str(), m_originalMode)) {
		const auto saved_errno = errno;
		std::cerr
				<< "TestHelpers::Chmod: couldn't change back the mode for `"
					<< m_path
					<< "': ("
					<< std::to_string(saved_errno)
					<< ") "
					<< strerror(saved_errno)
					<< std::endl;
		::abort();
	}
}

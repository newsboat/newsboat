#include "tempfile.h"

#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <vector>

test_helpers::TempFile::TempFile()
{
	const auto filepath_template = tempdir.get_path() + "tmp.XXXXXX";
	std::vector<char> filepath_template_c(
		filepath_template.cbegin(), filepath_template.cend());
	filepath_template_c.push_back('\0');

	const auto fd = ::mkstemp(filepath_template_c.data());
	if (fd == -1) {
		const auto saved_errno = errno;
		std::string msg("TempFile: failed to generate unique filename: (");
		msg += std::to_string(saved_errno);
		msg += ") ";
		msg += ::strerror(saved_errno);
		throw MainTempDir::tempfileexception(newsboat::Filepath(), msg);
	}

	// cend()-1 so we don't copy the terminating null byte - std::string
	// doesn't need it
	filepath = std::string(
			filepath_template_c.cbegin(), filepath_template_c.cend() - 1);

	::close(fd);
	// `TempFile` is supposed to only *generate* the name, not create the
	// file. Since mkstemp does create a file, we have to remove it.
	::unlink(filepath.c_str());
}

test_helpers::TempFile::~TempFile()
{
	::unlink(filepath.c_str());
}

const std::string test_helpers::TempFile::get_path() const
{
	return filepath;
}

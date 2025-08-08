#include "tempfile.h"

#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <vector>

test_helpers::TempFile::TempFile()
{
	const auto tempdir_path = tempdir.get_path().to_locale_string();
	std::vector<char> filepath_template(
		std::begin(tempdir_path), std::end(tempdir_path));
	filepath_template.push_back('/');
	const std::string filename_template("tmp.XXXXXX");
	std::copy(filename_template.begin(), filename_template.end(),
		std::back_inserter(filepath_template));
	filepath_template.push_back('\0');

	const auto fd = ::mkstemp(filepath_template.data());
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
	const std::string filepath_str(
		filepath_template.cbegin(), filepath_template.cend() - 1);
	filepath = newsboat::Filepath::from_locale_string(filepath_str);

	::close(fd);
	// `TempFile` is supposed to only *generate* the name, not create the
	// file. Since mkstemp does create a file, we have to remove it.
	::unlink(filepath_str.c_str());
}

test_helpers::TempFile::~TempFile()
{
	const auto filepath_str = filepath.to_locale_string();
	::unlink(filepath_str.c_str());
}

newsboat::Filepath test_helpers::TempFile::get_path() const
{
	return filepath;
}

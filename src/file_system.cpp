#include "file_system.h"

#include <grp.h>
#include <pwd.h>
#include <sys/types.h>

#include "strprintf.h"

namespace newsboat {

namespace file_system {

FileType mode_to_filetype(mode_t mode)
{
	static struct FlagChar {
		mode_t flag;
		FileType ftype;
	} flags[] = {{S_IFREG, FileType::RegularFile},
		{S_IFDIR, FileType::Directory},
		{S_IFBLK, FileType::BlockDevice},
		{S_IFCHR, FileType::CharDevice},
		{S_IFIFO, FileType::Fifo},
		{S_IFLNK, FileType::Symlink},
		{S_IFSOCK, FileType::Socket},
		{0, FileType::Unknown}
	};

	for (unsigned int i = 0; flags[i].flag != 0; i++) {
		if ((mode & S_IFMT) == flags[i].flag) {
			return flags[i].ftype;
		}
	}
	return FileType::Unknown;
}

char filetype_to_char(FileType type)
{
	const char unknown = '?';

	switch (type) {
	case FileType::Unknown:
		return unknown;
	case FileType::RegularFile:
		return '-';
	case FileType::Directory:
		return 'd';
	case FileType::BlockDevice:
		return 'b';
	case FileType::CharDevice:
		return 'c';
	case FileType::Fifo:
		return 'p';
	case FileType::Symlink:
		return 'l';
	case FileType::Socket:
		return 's';
	}

	return unknown;
}


nonstd::optional<char> mode_suffix(mode_t mode)
{
	const auto type = mode_to_filetype(mode);

	switch (type) {
	case FileType::Unknown:
		return nonstd::nullopt;
	case FileType::Directory:
		return '/';
	case FileType::Symlink:
		return '@';
	case FileType::Socket:
		return '=';
	case FileType::Fifo:
		return '|';

	case FileType::RegularFile:
	case FileType::BlockDevice:
	case FileType::CharDevice:
		if (mode & S_IXUSR) {
			return '*';
		}
	}

	return nonstd::nullopt;
}

Utf8String get_user_padded(uid_t uid)
{
	const struct passwd* spw = ::getpwuid(uid);
	if (spw != nullptr) {
		// Manpages give no indication regarding the encoding of the username.
		// However, according to https://serverfault.com/a/578264 POSIX
		// suggests to use alphanumerics, period, hyphen, and underscore, and
		// the hyphen can't be the first character. All of these fit into
		// ASCII, which is a subset of UTF-8, so we'll treat the username as
		// UTF-8.
		return Utf8String::from_utf8(strprintf::fmt("%-8s", spw->pw_name));
	}
	return "????????";
}

Utf8String get_group_padded(gid_t gid)
{
	const struct group* sgr = ::getgrgid(gid);
	if (sgr != nullptr) {
		// See the comment in get_user_padded() and
		// https://unix.stackexchange.com/questions/11477/what-are-the-allowed-group-names-for-groupadd
		return strprintf::fmt("%-8s", sgr->gr_name);
	}
	return "????????";
}

Utf8String permissions_string(mode_t mode)
{
	Utf8String bitstrs[] = {
		"---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx"
	};

	Utf8String result;
	result.append(bitstrs[(mode & 0700) >> 6]);
	result.append(bitstrs[(mode & 0070) >> 3]);
	result.append(bitstrs[(mode & 0007)]);
	return result;
}

} // namespace file_system

} // namespace newsboat

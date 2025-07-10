#include "file_system.h"

#include <grp.h>
#include <pwd.h>
#include <sys/types.h>

#include "strprintf.h"

namespace Newsboat {

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


std::optional<char> mode_suffix(mode_t mode)
{
	const auto type = mode_to_filetype(mode);

	switch (type) {
	case FileType::Unknown:
		return std::nullopt;
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

	return std::nullopt;
}

std::string get_user_padded(uid_t uid)
{
	const struct passwd* spw = ::getpwuid(uid);
	if (spw != nullptr) {
		return strprintf::fmt("%-8s", spw->pw_name);
	}
	return "????????";
}

std::string get_group_padded(gid_t gid)
{
	const struct group* sgr = ::getgrgid(gid);
	if (sgr != nullptr) {
		return strprintf::fmt("%-8s", sgr->gr_name);
	}
	return "????????";
}

std::string permissions_string(mode_t mode)
{
	unsigned int val = mode & 0777;

	std::string str;
	const char* bitstrs[] = {
		"---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx"
	};
	for (int i = 0; i < 3; ++i) {
		unsigned char bits = val % 8;
		val /= 8;
		str.insert(0, bitstrs[bits]);
	}
	return str;
}

} // namespace file_system

} // namespace Newsboat

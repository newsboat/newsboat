#include "filesystembrowser.h"

namespace newsboat {

namespace FileSystemBrowser {

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

} // namespace FileSystemBrowser

} // namespace newsboat
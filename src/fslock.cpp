#include "fslock.h"

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "logger.h"

namespace newsboat {

void remove_lock(const std::string& lock_filepath)
{
	::unlink(lock_filepath.c_str());
	LOG(Level::DEBUG, "FsLock: removed lockfile %s", lock_filepath);
}

FsLock::~FsLock()
{
	if (locked) {
		remove_lock(lock_filepath);
		::close(fd);
	}
}

bool FsLock::try_lock(const std::string& new_lock_filepath, pid_t& pid)
{
	if (locked && lock_filepath == new_lock_filepath) {
		return true;
	}

	// pid == 0 indicates that something went majorly wrong during locking
	pid = 0;

	LOG(Level::DEBUG, "FsLock: trying to lock `%s'", new_lock_filepath);

	// first, we open (and possibly create) the lock file
	fd = ::open(new_lock_filepath.c_str(), O_RDWR | O_CREAT, 0600);
	if (fd < 0) {
		return false;
	}

	// then we lock it (returns immediately if locking is not possible)
	if (lockf(fd, F_TLOCK, 0) == 0) {
		LOG(Level::DEBUG,
			"FsLock: locked `%s', writing PID...",
			new_lock_filepath);
		std::string pidtext = std::to_string(getpid());
		// locking successful -> truncate file and write own PID into it
		ssize_t written = 0;
		if (ftruncate(fd, 0) == 0) {
			written = write(fd, pidtext.c_str(), pidtext.length());
		}
		bool success = (written != -1) &&
			(static_cast<unsigned int>(written) ==
				pidtext.length());
		LOG(Level::DEBUG,
			"FsLock: PID written successfully: %i",
			success);
		if (success) {
			if (locked) {
				remove_lock(lock_filepath);
			}
			locked = success;
			lock_filepath = new_lock_filepath;
		} else {
			::close(fd);
		}
		return success;
	} else {
		LOG(Level::ERROR,
			"FsLock: something went wrong during locking: %s",
			strerror(errno));
	}

	// locking was not successful -> read PID of locking process from the
	// file
	if (fd >= 0) {
		char buf[32];
		int len = read(fd, buf, sizeof(buf) - 1);
		if (len > 0) {
			buf[len] = '\0';
			unsigned int upid = 0;
			sscanf(buf, "%u", &upid);
			pid = upid;
		}
		close(fd);
	}
	LOG(Level::DEBUG, "FsLock: locking failed, already locked by %u", pid);
	return false;
}

} // namespace newsboat

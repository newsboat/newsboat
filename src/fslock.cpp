#include "fslock.h"

#include <cerrno>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "logger.h"

namespace newsboat {

FsLock::FsLock()
{
	rs_fslock = rs_fslock_new();
}

FsLock::~FsLock()
{
	rs_fslock_free(rs_fslock);
}

bool FsLock::try_lock(const std::string& new_lock_filepath, pid_t& pid)
{
	return rs_fslock_try_lock(rs_fslock, new_lock_filepath.c_str(), &pid);
}

} // namespace newsboat

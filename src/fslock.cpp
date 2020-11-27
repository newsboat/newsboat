#include "fslock.h"

#include <cerrno>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "logger.h"

namespace newsboat {

FsLock::FsLock()
	: rs_object(fslock::bridged::create())
{
}

bool FsLock::try_lock(const std::string& new_lock_filepath, pid_t& pid)
{
	std::int64_t p;
	const bool result =  rs_object->try_lock_ffi(new_lock_filepath, p);

	// We use `libc::pid_t` on the rust side so we can guarantee this will fit
	pid = static_cast<std::int64_t>(p);
	return result;
}

} // namespace newsboat

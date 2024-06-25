#define ENABLE_IMPLICIT_FILEPATH_CONVERSIONS

#include "fslock.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace newsboat {

FsLock::FsLock()
	: rs_object(fslock::bridged::create())
{
}

bool FsLock::try_lock(const Filepath& new_lock_filepath, pid_t& pid,
	std::string& error_message)
{
	std::int64_t p;
	rust::String message;
	const bool result = newsboat::fslock::bridged::try_lock(*rs_object,
			new_lock_filepath, p, message);

	// We use `libc::pid_t` on the rust side so we can guarantee this will fit
	pid = static_cast<std::int64_t>(p);
	error_message = std::string(message);

	return result;
}

} // namespace newsboat

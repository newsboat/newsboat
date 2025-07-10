#ifndef NEWSBOAT_FSLOCK_H_
#define NEWSBOAT_FSLOCK_H_

#include "libNewsboat-ffi/src/fslock.rs.h" // IWYU pragma: export

#include <string>
#include <sys/types.h>

namespace Newsboat {

class FsLock {
public:
	FsLock();
	~FsLock() = default;

	bool try_lock(const std::string& lock_file, pid_t& pid,
		std::string& error_message);

private:
	rust::Box<fslock::bridged::FsLock> rs_object;
};

} // namespace Newsboat

#endif /* NEWSBOAT_FSLOCK_H_ */

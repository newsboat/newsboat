#ifndef NEWSBOAT_FSLOCK_H_
#define NEWSBOAT_FSLOCK_H_

#include "libnewsboat-ffi/src/fslock.rs.h" // IWYU pragma: export

#include <string>
#include <sys/types.h>

#include "filepath.h"

namespace newsboat {

class FsLock {
public:
	FsLock();
	~FsLock() = default;

	bool try_lock(const Filepath& lock_file, pid_t& pid,
		std::string& error_message);

private:
	rust::Box<fslock::bridged::FsLock> rs_object;
};

} // namespace newsboat

#endif /* NEWSBOAT_FSLOCK_H_ */

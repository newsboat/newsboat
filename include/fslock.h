#ifndef NEWSBOAT_FSLOCK_H_
#define NEWSBOAT_FSLOCK_H_

#include "fslock.rs.h"

#include <string>
#include <sys/types.h>

namespace newsboat {

class FsLock {
public:
	FsLock();
	~FsLock() = default;

	bool try_lock(const std::string& lock_file, pid_t& pid);

private:
	rust::Box<fslock::bridged::FsLock> rs_object;
};

} // namespace newsboat

#endif /* NEWSBOAT_FSLOCK_H_ */

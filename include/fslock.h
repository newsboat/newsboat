#ifndef NEWSBOAT_FSLOCK_H_
#define NEWSBOAT_FSLOCK_H_

#include <string>
#include <sys/types.h>

namespace newsboat {

class FSLock {
public:
	FSLock() = default;
	~FSLock();

	bool try_lock(const std::string& lock_file, pid_t& pid);

private:
	std::string lock_filepath;
	int fd = -1;
	bool locked = false;
};

} // namespace newsboat

#endif /* NEWSBOAT_FSLOCK_H_ */

#ifndef NEWSBOAT_FSLOCK_H_
#define NEWSBOAT_FSLOCK_H_

#include <string>
#include <sys/types.h>

extern "C" {

	void* rs_fslock_new();

	void rs_fslock_free(void* ptr);

	bool rs_fslock_try_lock(void* ptr, const char* new_lock_filepath, pid_t* pid);

} // extern "C"

namespace newsboat {

class FsLock {
public:
	FsLock();
	~FsLock();

	bool try_lock(const std::string& lock_file, pid_t& pid);

private:
	void* rs_fslock = nullptr;
};

} // namespace newsboat

#endif /* NEWSBOAT_FSLOCK_H_ */

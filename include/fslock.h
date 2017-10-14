#ifndef NEWSBOAT_FSLOCK_H_
#define NEWSBOAT_FSLOCK_H_

#include <string>
#include <sys/types.h>

namespace newsboat {

class FSLock {
	public:
		FSLock() = default;
		~FSLock();

		bool try_lock(const std::string& lock_file, pid_t & pid);

	private:
		bool locked = { false };
		std::string lock_filepath;
};

}

#endif /* NEWSBOAT_FSLOCK_H_ */

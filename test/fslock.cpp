#define ENABLE_IMPLICIT_FILEPATH_CONVERSIONS

#include "fslock.h"

#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "3rd-party/catch.hpp"
#include "test_helpers/tempdir.h"
#include "test_helpers/tempfile.h"

using namespace newsboat;

// Forks and calls FsLock::try_lock() in the child process.
class LockProcess {
public:
	explicit LockProcess(const std::string& lock_location)
	{
		sem_start = sem_open(sem_start_name, O_CREAT, 0644, 0);
		sem_stop = sem_open(sem_stop_name, O_CREAT, 0644, 0);

		pid = ::fork();
		if (pid == -1) {
			FAIL("LockProcess: fork() failed");
		} else if (pid > 0) {
			// Parent process: Wait until child process has finished calling try_lock
			sem_wait(sem_start);
		} else {
			// Child process: Call try_lock, signal parent to continue, wait for parent process signal to stop
			pid_t ignore_pid;
			FsLock lock;
			std::string error_message;
			lock.try_lock(lock_location, ignore_pid, error_message);
			sem_post(sem_start);

			sem_wait(sem_stop);
			// Exit directly without running destructors (making sure we don't interfere with the original process)
			_exit(0);
		}
	}

	pid_t get_child_pid()
	{
		return pid;
	}

	~LockProcess()
	{
		if (pid > 0) {
			// Parent process: Signal child process to exit and wait for it to finish
			sem_post(sem_stop);
			::waitpid(pid, nullptr, 0);

			REQUIRE(sem_unlink(sem_start_name) == 0);
			REQUIRE(sem_unlink(sem_stop_name) == 0);
			REQUIRE(sem_close(sem_start) == 0);
			REQUIRE(sem_close(sem_stop) == 0);
		}
	}

private:
	const char* sem_start_name = "/newsboat-test-fslock-start";
	const char* sem_stop_name = "/newsboat-test-fslock-stop";

	pid_t pid;
	sem_t* sem_start;
	sem_t* sem_stop;
};


TEST_CASE("try_lock() returns an error if lock-file permissions or location are invalid",
	"[FsLock]")
{

	GIVEN("An invalid lock location") {
		const test_helpers::TempDir test_directory;
		const auto non_existing_dir = test_directory.get_path().join("does-not-exist");
		const auto lock_location = non_existing_dir.join("lockfile");

		THEN("try_lock() will fail and return pid == 0") {
			FsLock lock;
			pid_t pid = -1;

			// try_lock() is expected to fail as the relevant directory does not exist
			std::string error_message;
			REQUIRE_FALSE(lock.try_lock(lock_location, pid, error_message));
			REQUIRE(pid == 0);
			REQUIRE(error_message.length() > 0);
		}
	}

	GIVEN("A lock file which does not grant write access") {
		const test_helpers::TempFile lock_location;
		const int fd = ::open(lock_location.get_path().c_str(), O_RDWR | O_CREAT, 0400);
		::close(fd);

		THEN("try_lock() will fail and return pid == 0") {
			FsLock lock;
			pid_t pid = -1;
			std::string error_message;
			REQUIRE_FALSE(lock.try_lock(lock_location.get_path(), pid, error_message));
			REQUIRE(pid == 0);
			REQUIRE(error_message.length() > 0);
		}
	}
}

TEST_CASE("try_lock() fails if lock was already created", "[FsLock]")
{
	const test_helpers::TempFile lock_location;

	WHEN("A different process has called try_lock()") {
		LockProcess lock_process(lock_location.get_path());

		FsLock lock;
		pid_t pid = 0;

		THEN("Calling try_lock() for the same lock location will fail") {
			std::string error_message;
			REQUIRE_FALSE(lock.try_lock(lock_location.get_path(), pid, error_message));
		}

		THEN("try_lock() returns the pid of the process holding the lock") {
			std::string error_message;
			REQUIRE_FALSE(lock.try_lock(lock_location.get_path(), pid, error_message));

			REQUIRE(pid == lock_process.get_child_pid());
		}
	}
}

TEST_CASE("try_lock() succeeds if lock file location is valid and not locked by a different process",
	"[FsLock]")
{
	const test_helpers::TempFile lock_location;
	FsLock lock;
	pid_t pid = 0;

	std::string error_message;
	REQUIRE(lock.try_lock(lock_location.get_path(), pid, error_message));

	SECTION("The lock file exists after a call to try_lock()") {
		REQUIRE(0 == ::access(lock_location.get_path().c_str(), F_OK));
	}

	SECTION("Calling try_lock() a second time for the same location succeeds") {
		REQUIRE(lock.try_lock(lock_location.get_path(), pid, error_message));
	}

	SECTION("Calling try_lock() a second time with a different location succeeds and cleans up old lock file") {
		const test_helpers::TempFile new_lock_location;

		REQUIRE(0 == ::access(lock_location.get_path().c_str(), F_OK));
		REQUIRE(lock.try_lock(new_lock_location.get_path(), pid, error_message));
		REQUIRE(0 != ::access(lock_location.get_path().c_str(), F_OK));
		REQUIRE(0 == ::access(new_lock_location.get_path().c_str(), F_OK));
	}
}


use libNewsboat::fslock;

use std::path::Path;

// cxx doesn't allow to share types from other crates, so we have to wrap it
// cf. https://github.com/dtolnay/cxx/issues/496
struct FsLock(fslock::FsLock);

#[cxx::bridge(namespace = "Newsboat::fslock::bridged")]
mod bridged {
    extern "Rust" {
        type FsLock;

        fn create() -> Box<FsLock>;
        fn try_lock(
            fslock: &mut FsLock,
            new_lock_path: &str,
            pid: &mut i64,
            error_message: &mut String,
        ) -> bool;
    }
}

fn create() -> Box<FsLock> {
    Box::new(FsLock(fslock::FsLock::default()))
}

fn try_lock(
    fslock: &mut FsLock,
    new_lock_path: &str,
    pid: &mut i64,
    error_message: &mut String,
) -> bool {
    let p: &mut libc::pid_t = &mut 0;
    let result = fslock.0.try_lock(Path::new(new_lock_path), p);
    *pid = i64::from(*p);
    match result {
        Ok(_) => return true,
        Err(message) => *error_message = message,
    }
    false
}

use libnewsboat::fslock::FsLock;

use std::path::Path;

#[cxx::bridge(namespace = "newsboat::fslock::bridged")]
mod bridged {
    extern "Rust" {
        type FsLock;

        fn create() -> Box<FsLock>;
        fn try_lock(fslock: &mut FsLock, new_lock_path: &str, pid: &mut i64) -> bool;
    }
}

fn create() -> Box<FsLock> {
    Box::new(FsLock::default())
}

fn try_lock(fslock: &mut FsLock, new_lock_path: &str, pid: &mut i64) -> bool {
    let p: &mut libc::pid_t = &mut 0;
    let result = fslock.try_lock(Path::new(new_lock_path), p);
    *pid = i64::from(*p);
    result
}

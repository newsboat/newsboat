use libnewsboat::fslock::FsLock;

#[cxx::bridge(namespace = "newsboat::fslock::bridged")]
mod bridged {
    extern "Rust" {
        type FsLock;

        fn create() -> Box<FsLock>;
        fn try_lock_ffi(&mut self, new_lock_path: &str, pid: &mut i64) -> bool;
    }
}

fn create() -> Box<FsLock> {
    Box::new(FsLock::default())
}

//! Helper binary to help lock process for testing.
use libNewsboat::fslock::FsLock;

fn main() {
    let lock_location = std::env::args().nth(1).unwrap();

    let mut lock = FsLock::default();
    assert!(lock.try_lock(lock_location.as_ref(), &mut 0).is_ok());

    // signal that we already lock file
    println!();

    // wait for signal to exit and drop lock
    std::io::stdin().read_line(&mut String::new()).unwrap();
}

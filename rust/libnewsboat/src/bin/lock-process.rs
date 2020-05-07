//! Helper binary to help lock process for testing.
use libnewsboat::fslock::FsLock;

fn main() {
    let lock_location = std::env::args().skip(1).next().unwrap();

    let mut lock = FsLock::default();
    assert!(lock.try_lock(lock_location.as_ref(), &mut 0));

    // signal that we already lock file
    println!();

    // wait for signal to exit and drop lock
    std::io::stdin().read_line(&mut String::new()).unwrap();
}

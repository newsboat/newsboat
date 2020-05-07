use libnewsboat::fslock::FsLock;
use std::io::{Read, Write};
use std::process::{Command, Stdio};
use tempfile::NamedTempFile;

#[test]
fn t_returns_an_error_if_invalid_lock_location() {
    let tmp = tempfile::tempdir().unwrap();
    let non_existing_dir = tmp.as_ref().join("does-not-exist/");
    let lock_location = non_existing_dir.join("lockfile");

    let mut lock = FsLock::default();
    let mut pid = -1;

    assert!(!lock.try_lock(lock_location.as_ref(), &mut pid));
    assert_eq!(pid, 0);
}

#[test]
fn t_returns_an_error_lock_file_without_write_access() {
    let lock_location = NamedTempFile::new().unwrap();
    let mut perms = lock_location.as_file().metadata().unwrap().permissions();
    perms.set_readonly(true);
    lock_location.as_file().set_permissions(perms).unwrap();

    let mut lock = FsLock::default();
    let mut pid = -1;
    assert!(!lock.try_lock(lock_location.as_ref(), &mut pid));
    assert_eq!(pid, 0);
}

#[test]
fn t_fails_if_lock_was_already_created() {
    let lock_location = NamedTempFile::new().unwrap();

    let mut child = Command::new(env!("CARGO_BIN_EXE_lock-process"))
        .arg(lock_location.path())
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .spawn()
        .unwrap();

    let mut lock = FsLock::default();
    let mut pid = 0;
    let cid = child.id() as i32;

    // wait for locked signal
    let stdout = child.stdout.as_mut().unwrap();
    stdout.read(&mut [0]).unwrap();

    assert!(!lock.try_lock(lock_location.as_ref(), &mut pid));
    assert_eq!(pid, cid, "pid should be process holding the lock");

    // notify child to exit and drop lock
    let stdin = child.stdin.as_mut().unwrap();
    stdin.write(b"\n").unwrap();
}

#[test]
fn t_succeeds_if_lock_file_location_is_valid_and_not_locked_by_different_process() {
    let lock_location = NamedTempFile::new().unwrap();
    let mut lock = FsLock::default();
    let mut pid = 0;

    assert!(lock.try_lock(lock_location.as_ref(), &mut pid));
    assert!(lock_location.path().exists());
    assert!(
        lock.try_lock(lock_location.as_ref(), &mut pid),
        "recall succeeds"
    );

    let new_lock_location = NamedTempFile::new().unwrap();

    assert!(lock_location.path().exists());
    assert!(lock.try_lock(new_lock_location.as_ref(), &mut pid));
    assert!(!lock_location.path().exists());
    assert!(new_lock_location.path().exists());
}

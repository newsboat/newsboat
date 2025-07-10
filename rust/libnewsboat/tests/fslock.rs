use libNewsboat::fslock::FsLock;
use std::env;
use std::io::{Error, ErrorKind};
use std::io::{Read, Write};
use std::path::PathBuf;
use std::process::{Command, Stdio};
use tempfile::NamedTempFile;

fn get_exe_path(exe: &str) -> Result<PathBuf, Error> {
    let mut p = env::current_exe().expect("exe path");
    while p.pop() {
        if p.join(exe).exists() {
            return Ok(p.join(exe));
        }
    }
    Err(Error::new(ErrorKind::NotFound, exe))
}

#[test]
fn t_returns_an_error_if_invalid_lock_location() {
    let tmp = tempfile::tempdir().unwrap();
    let non_existing_dir = tmp.as_ref().join("does-not-exist/");
    let lock_location = non_existing_dir.join("lockfile");

    let mut lock = FsLock::default();
    let mut pid = -1;

    assert!(lock.try_lock(lock_location.as_ref(), &mut pid).is_err());
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
    assert!(lock.try_lock(lock_location.as_ref(), &mut pid).is_err());
    assert_eq!(pid, 0);
}

#[test]
fn t_fails_if_lock_was_already_created() {
    let lock_location = NamedTempFile::new().unwrap();

    let cmd = match env::var("CARGO_BIN_EXE_lock-process") {
        Ok(dir) => dir,
        Err(_) => get_exe_path("lock-process")
            .unwrap()
            .to_str()
            .unwrap()
            .to_string(),
    };

    let mut child = Command::new(cmd)
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
    stdout.read_exact(&mut [0]).unwrap();

    let result = lock.try_lock(lock_location.as_ref(), &mut pid);
    assert!(result.is_err());
    if let Err(e) = result {
        assert!(!e.is_empty());
    }
    assert_eq!(pid, cid, "pid should be process holding the lock");

    // notify child to exit and drop lock
    let stdin = child.stdin.as_mut().unwrap();
    stdin.write_all(b"\n").unwrap();
    child.wait().unwrap();
}

#[test]
fn t_succeeds_if_lock_file_location_is_valid_and_not_locked_by_different_process() {
    let lock_location = NamedTempFile::new().unwrap();
    let mut lock = FsLock::default();
    let mut pid = 0;

    assert!(lock.try_lock(lock_location.as_ref(), &mut pid).is_ok());
    assert!(lock_location.path().exists());
    assert!(
        lock.try_lock(lock_location.as_ref(), &mut pid).is_ok(),
        "recall succeeds"
    );

    let new_lock_location = NamedTempFile::new().unwrap();

    assert!(lock_location.path().exists());
    assert!(lock.try_lock(new_lock_location.as_ref(), &mut pid).is_ok());
    assert!(!lock_location.path().exists());
    assert!(new_lock_location.path().exists());
}

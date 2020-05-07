use crate::logger::{self, Level};
use std::fs::{self, File, OpenOptions};
use std::io::{Read, Write};
use std::os::unix::io::AsRawFd;
use std::path::{Path, PathBuf};
use std::process;

fn remove_lock(lock_path: &Path) {
    fs::remove_file(lock_path).ok();
    log!(
        Level::Debug,
        "FsLock: removed lockfile {}",
        lock_path.display()
    );
}

#[derive(Default)]
pub struct FsLock {
    lock_path: PathBuf,
    lock_file: Option<File>,
}

impl Drop for FsLock {
    fn drop(&mut self) {
        if self.lock_file.is_some() {
            remove_lock(&self.lock_path);
        }
    }
}

impl FsLock {
    pub fn try_lock(&mut self, new_lock_path: &Path, pid: &mut libc::pid_t) -> bool {
        if self.lock_file.is_some() && self.lock_path == new_lock_path {
            return true;
        }

        // pid == 0 indicates that something went majorly wrong during locking
        *pid = 0;

        log!(
            Level::Debug,
            "FsLock: trying to lock `{}'",
            new_lock_path.display()
        );

        // first we open (and possibly create) the lock file
        let mut openoptions = OpenOptions::new();
        openoptions.read(true).write(true).create(true);
        let mut file = match openoptions.open(&new_lock_path) {
            Ok(file) => file,
            Err(_) => return false,
        };

        // then we lock it (returns immediately if locking is not possible)
        if unsafe { libc::lockf(file.as_raw_fd(), libc::F_TLOCK, 0) } == 0 {
            log!(
                Level::Debug,
                "FsLock: locked `{}', writing PID...",
                new_lock_path.display()
            );
            let pid = process::id().to_string();
            let buf = pid.as_bytes();
            let success = file.write_all(&buf).is_ok();
            log!(
                Level::Debug,
                "FsLock: PID written successfully: {}",
                success as usize
            );
            if success {
                if self.lock_file.take().is_some() {
                    remove_lock(&self.lock_path);
                }
                self.lock_file = Some(file);
                self.lock_path = new_lock_path.to_owned();
            }
            success
        } else {
            log!(
                Level::Error,
                "FsLock: something went wrong during locking: {}",
                errno::errno()
            );

            // locking was not successful -> read PID of locking process from the file
            let mut buf = String::new();
            if file.read_to_string(&mut buf).is_ok() {
                *pid = buf.parse().unwrap_or(0);
            }
            log!(
                Level::Debug,
                "FsLock: locking failed, already locked by {}",
                pid
            );
            false
        }
    }
}

use crate::logger::{self, Level};
use gettextrs::gettext;
use std::fs::{self, File, OpenOptions};
use std::io::{Error, Read, Write};
use std::os::unix::fs::OpenOptionsExt;
use std::os::unix::io::AsRawFd;
use std::path::{Path, PathBuf};
use std::process;

use strprintf::fmt;

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

// Temporarily ignore clippy lint until PR is merged:
// https://github.com/rust-lang/rust-clippy/pull/12756
#[allow(clippy::assigning_clones)]
impl FsLock {
    pub fn try_lock(&mut self, new_lock_path: &Path, pid: &mut libc::pid_t) -> Result<(), String> {
        if self.lock_file.is_some() && self.lock_path == new_lock_path {
            return Ok(());
        }

        // pid == 0 indicates that something went majorly wrong during locking
        *pid = 0;

        log!(
            Level::Debug,
            "FsLock: trying to lock `{}'",
            new_lock_path.display()
        );

        // first we open (and possibly create) the lock file
        let mut options = OpenOptions::new();
        options.create(true).read(true).write(true).mode(0o600);
        let mut file = match options.open(new_lock_path) {
            Ok(file) => file,
            Err(reason) => {
                return Err(fmt!(
                    &gettext("Failed to open lock file '%s': %s"),
                    new_lock_path
                        .to_str()
                        .unwrap_or(&gettext("<filename containing invalid UTF-8 codepoint>")),
                    reason.to_string()
                ));
            }
        };

        // then we lock it (returns immediately if locking is not possible)
        if unsafe { libc::lockf(file.as_raw_fd(), libc::F_TLOCK, 0) } == 0 {
            log!(
                Level::Debug,
                "FsLock: locked `{}', writing PID...",
                new_lock_path.display()
            );
            let pid = process::id().to_string();
            if let Err(reason) = file.set_len(0).and_then(|_| file.write_all(pid.as_bytes())) {
                log!(Level::Debug, "FsLock: Failed to write PID");
                return Err(fmt!(
                    &gettext("Failed to write PID to lock file '%s': %s"),
                    new_lock_path
                        .to_str()
                        .unwrap_or(&gettext("<filename containing invalid UTF-8 codepoint>")),
                    reason.to_string()
                ));
            }
            log!(Level::Debug, "FsLock: PID written successfully");
            if self.lock_file.take().is_some() {
                remove_lock(&self.lock_path);
            }
            self.lock_file = Some(file);
            self.lock_path = new_lock_path.to_owned();
            Ok(())
        } else {
            log!(
                Level::Error,
                "FsLock: something went wrong during locking: {}",
                Error::last_os_error()
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
            Err(fmt!(
                &gettext("Failed to lock '%s', already locked by process with PID %s"),
                new_lock_path
                    .to_str()
                    .unwrap_or(&gettext("<filename containing invalid UTF-8 codepoint>")),
                &(*pid).to_string()
            ))
        }
    }
}

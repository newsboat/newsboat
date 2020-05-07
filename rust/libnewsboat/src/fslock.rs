use crate::logger::{self, Level};
use std::ffi::{CString, OsStr};
use std::os::unix::ffi::OsStrExt;
use std::path::{Path, PathBuf};
use std::process;

fn remove_lock(lock_filepath: &Path) {
    let os: &OsStr = lock_filepath.as_ref();
    // Safety: Safe to convert when memory is enough
    let path = CString::new(os.as_bytes()).unwrap();
    unsafe { libc::unlink(path.as_ptr()) };
    log!(
        Level::Debug,
        "FsLock: removed lockfile {}",
        lock_filepath.display()
    );
}

pub struct FsLock {
    lock_filepath: PathBuf,
    fd: i32,
    locked: bool,
}

impl Default for FsLock {
    fn default() -> Self {
        FsLock {
            lock_filepath: PathBuf::new(),
            fd: -1,
            locked: false,
        }
    }
}

impl Drop for FsLock {
    fn drop(&mut self) {
        if self.locked {
            remove_lock(&self.lock_filepath);
            let _ = unsafe { libc::close(self.fd) };
        }
    }
}

impl FsLock {
    pub fn try_lock(&mut self, new_lock_filepath: &Path, pid: &mut libc::pid_t) -> bool {
        if self.locked && self.lock_filepath == new_lock_filepath {
            return true;
        }

        // pid == 0 indicates that something went majorly wrong during locking
        *pid = 0;

        // TODO: move to safe rust later
        // let openoptions = OpenOptions::new().read(true).write(true).create(true);
        // let mut file = match openoptions.open(path) {
        //     Ok(file) => file,
        //     Err(_) => return false,
        // };

        log!(
            Level::Debug,
            "FsLock: trying to lock `{}'",
            new_lock_filepath.display()
        );

        // first we open (and possibly create) the lock file
        self.fd = unsafe {
            let os: &OsStr = new_lock_filepath.as_ref();
            // Safety: Safe to convert when memory is enough
            let buf = CString::new(os.as_bytes()).unwrap();
            libc::open(buf.as_ptr(), libc::O_RDWR | libc::O_CREAT, 0o600)
        };
        if self.fd < 0 {
            return false;
        }

        // then we lock it (returns immediately if locking is not possible)
        if unsafe { libc::lockf(self.fd, libc::F_TLOCK, 0) } == 0 {
            log!(
                Level::Debug,
                "FsLock: locked `{}', writing PID...",
                new_lock_filepath.display()
            );
            let pidtext = process::id().to_string();
            let mut written = 0;
            if unsafe { libc::ftruncate(self.fd, 0) } == 0 {
                written = unsafe {
                    libc::write(
                        self.fd,
                        pidtext.as_bytes().as_ptr() as *const libc::c_void,
                        pidtext.len(),
                    )
                };
            }
            let success = written != -1 && written as usize == pidtext.len();
            log!(
                Level::Debug,
                "FsLock: PID written successfully: {}",
                success as usize
            );
            if success {
                if self.locked {
                    remove_lock(&self.lock_filepath);
                }
                self.locked = true;
                self.lock_filepath = new_lock_filepath.into();
            } else {
                let _ = unsafe { libc::close(self.fd) };
            }
            success
        } else {
            log!(Level::Error, "FsLock: something went wrong during locking");

            // locking was not successful -> read PID of locking process from the file
            if self.fd >= 0 {
                let mut buf = [0; 32];
                let len = unsafe {
                    libc::read(self.fd, buf.as_mut_ptr() as *mut libc::c_void, buf.len())
                } as usize;
                if len > 0 {
                    let buf = unsafe { std::str::from_utf8_unchecked(buf.get_unchecked(..len)) };
                    *pid = buf.parse().unwrap_or(0);
                }
                let _ = unsafe { libc::close(self.fd) };
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

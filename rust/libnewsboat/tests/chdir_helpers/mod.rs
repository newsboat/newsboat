use libnewsboat::utils;
use std::env;
use std::path::Path;
use std::path::PathBuf;

pub struct Chdir {
    old_path: PathBuf,
}

impl Chdir {
    pub fn new(path: &Path) -> Chdir {
        let c = Chdir {
            old_path: utils::getcwd().unwrap(),
        };
        assert!(env::set_current_dir(path).is_ok());
        c
    }
}

impl Drop for Chdir {
    fn drop(&mut self) {
        env::set_current_dir(Path::new(&self.old_path));
    }
}

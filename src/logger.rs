use std::fs::{File, OpenOptions};

pub struct Logger {
    logfile: Option<File>,
}

impl Logger {
    pub fn new() -> Logger {
        Logger {
            logfile: None,
        }
    }

    pub fn set_logfile(&mut self, filename: &str) {
        let file = OpenOptions::new()
            .create(true)
            .append(true)
            .open(filename);

        match file {
            Ok(file) => self.logfile = Some(file),
            Err(error) => eprintln!("Couldn't open `{}' as a logfile: {}", filename, error),
        }
    }
}

#[cfg(test)]
mod tests {
    extern crate tempfile;

    use super::*;
    
    use std::io;
    use self::tempfile::TempDir;

    #[test]
    fn t_set_logfile() -> io::Result<()> {
        let tmp = TempDir::new()?;
        let filename = {
            let mut filename = tmp.path().to_owned();
            filename.push("example.log");
            filename
        };

        assert!(!filename.exists());

        let mut logger = Logger::new();
        logger.set_logfile(filename.to_str().unwrap());

        assert!(filename.exists());

        Ok(())
    }
}

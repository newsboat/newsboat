use std::fs::{File, OpenOptions};
use std::io::{self, Write};

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

    pub fn log(&mut self, message: &str) -> io::Result<()> {
        if let Some(ref mut logfile) = self.logfile {
            logfile.write_all(message.as_bytes())?;
        }

        Ok(())
    }
}

#[cfg(test)]
mod tests {
    extern crate tempfile;

    use super::*;
    
    use self::tempfile::TempDir;
    use std::io::{self, Read};
    use std::path;

    fn setup_logger() -> io::Result<(TempDir, path::PathBuf, Logger)> {
        let tmp = TempDir::new()?;
        let logfile = {
            let mut logfile = tmp.path().to_owned();
            logfile.push("example.log");
            logfile
        };
        assert!(!logfile.exists());

        let mut logger = Logger::new();
        logger.set_logfile(logfile.to_str().unwrap());

        Ok((tmp, logfile, logger))
    }

    #[test]
    fn t_set_logfile() -> io::Result<()> {
        let (_tmp, logfile, _logger) = setup_logger()?;

        assert!(logfile.exists());

        Ok(())
    }

    #[test]
    fn t_log_writes_message_to_the_file() -> io::Result<()> {
        let (_tmp, logfile, mut logger) = setup_logger()?;

        let message = "Hello, world!";
        logger.log(message)?;

        // Dropping logger to force it to flush the log and close the file
        drop(logger);

        let mut file = File::open(logfile)?;
        let mut contents = String::new();
        file.read_to_string(&mut contents)?;

        assert_eq!(contents, message);

        Ok(())
    }
}

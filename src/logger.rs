extern crate chrono;

use self::chrono::offset::Local;
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
            let timestamp = Local::now().format("%Y-%m-%d %H:%M:%S");
            let line = format!("[{}] {}\n", timestamp, message);

            logfile.write_all(line.as_bytes())?;
        }

        Ok(())
    }
}

#[cfg(test)]
mod tests {
    extern crate tempfile;

    use super::*;
    
    use self::tempfile::TempDir;
    use self::chrono::{TimeZone, Duration};
    use std::io::{self, BufReader, BufRead};
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

        let messages = {
            let mut messages = vec![];
            messages.push("Hello, world!");
            messages.push("I'm doing fine, how are you?");
            messages.push("Time to wrap up, see ya!");
            messages
        };

        let start_time = Local::now();
        for msg in &messages {
            logger.log(msg)?;
        }
        let finish_time = Local::now();

        // Dropping logger to force it to flush the log and close the file
        drop(logger);

        let file = File::open(logfile)?;
        let reader = BufReader::new(file);
        for (line, expected) in reader.lines().zip(messages) {
            match line {
                Ok(line) => {
                    assert!(line.starts_with("["));

                    let timestamp_end = line.find(']').expect("Failed to find the end of the timestamp");
                    // "]" is an ASCII character, so it occupies one byte in UTF-8. As a result, we
                    // can simply add one to get the next character's byte offset. And since the
                    // next char should be a space, which is ASCII as well, we can just add 2.
                    //
                    // If timestamp is followed by something non-ASCII, indexing into &str will
                    // panic and fail the test, which is good.
                    assert_eq!(&line[timestamp_end+1..timestamp_end+2], " ");

                    // Timestamp starts with "[", we skip it by starting at 1 rather than 0.
                    let timestamp_str = &line[1..timestamp_end];
                    let timestamp =
                        Local::now()
                        .timezone()
                        .datetime_from_str(timestamp_str, "%Y-%m-%d %H:%M:%S")
                        .expect("Failed to parse the timestamp from the log file");
                    // `start_time` and `end_time` may have millisecond precision or better,
                    // whereas `timestamp` is limited to seconds. Therefore, we account for
                    // a situation where `start_time` is slightly bigger than `timestamp`.
                    assert!(timestamp - start_time > Duration::seconds(-1));
                    assert!(finish_time >= timestamp);

                    // Message starts after "] " that follows the timestamp, hence +2.
                    let message_str = &line[timestamp_end+2..];
                    assert_eq!(message_str, expected);
                }
                Err(e) => return Err(e),
            }
        }

        Ok(())
    }
}

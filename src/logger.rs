extern crate chrono;

use self::chrono::offset::Local;
use std::fmt;
use std::fs::{File, OpenOptions};
use std::io::{self, Write};

#[derive(Clone, Copy)]
pub enum Level {
    None,
    UserError,
    Critical,
    Error,
    Warn,
    Info,
    Debug,
}

impl fmt::Display for Level {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            Level::None => write!(f, "NONE"),
            Level::UserError => write!(f, "USERERROR"),
            Level::Critical => write!(f, "CRITICAL"),
            Level::Error => write!(f, "ERROR"),
            Level::Warn => write!(f, "WARNING"),
            Level::Info => write!(f, "INFO"),
            Level::Debug => write!(f, "DEBUG"),
        }
    }
}

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

    pub fn log(&mut self, level: Level, message: &str) -> io::Result<()> {
        if let Some(ref mut logfile) = self.logfile {
            let timestamp = Local::now().format("%Y-%m-%d %H:%M:%S");
            let line = format!("[{}] {}: {}\n", timestamp, level, message);

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

    fn parse_log_line(line: &str) -> Option<(&str, &str, &str)> {
        if !line.starts_with("[") {
            return None;
        }

        let timestamp_end = line.find(']').expect("Failed to find the end of the timestamp");
        // Timestamp starts with "[", we skip it by starting at 1 rather than 0.
        let timestamp = &line[1..timestamp_end];

        // "]" is an ASCII character, so it occupies one byte in UTF-8. As a result, we
        // can simply add one to get the next character's byte offset. And since the
        // next char should be a space, which is ASCII as well, we can just add 2.
        //
        // If timestamp is followed by something non-ASCII, indexing into &str will
        // panic and fail the test, which is good.
        if &line[timestamp_end+1..timestamp_end+2] != " " {
            return None;
        }

        let level_end =
            line[timestamp_end+2..]
            .find(':')
            .expect("Failed to find the end of the loglevel");
        let level = &line[timestamp_end+2..timestamp_end+2+level_end];

        // Message starts after ": " that follows the level, hence +2.
        let message = &line[timestamp_end+2+level_end+2..];

        Some((timestamp, level, message))
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

        let messages = vec![
            "Hello, world!",
            "I'm doing fine, how are you?",
            "Time to wrap up, see ya!",
        ];

        let start_time = Local::now();
        for msg in &messages {
            logger.log(Level::Debug, msg)?;
        }
        let finish_time = Local::now();

        // Dropping logger to force it to flush the log and close the file
        drop(logger);

        let file = File::open(logfile)?;
        let reader = BufReader::new(file);
        for (line, expected) in reader.lines().zip(messages) {
            match line {
                Ok(line) => {
                    let (timestamp_str, _level, message) =
                        parse_log_line(&line)
                        .expect("Failed to split the log line into parts");

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

                    assert_eq!(message, expected);
                }
                Err(e) => return Err(e),
            }
        }

        Ok(())
    }

    #[test]
    fn t_different_loglevels_have_different_names() -> io::Result<()> {
        let (_tmp, logfile, mut logger) = setup_logger()?;

        let levels = vec![
            (Level::UserError, "USERERROR"),
            (Level::Critical, "CRITICAL"),
            (Level::Error, "ERROR"),
            (Level::Warn, "WARNING"),
            (Level::Info, "INFO"),
            (Level::Debug, "DEBUG"),
        ];

        let msg = "Some test message";

        for (level, _level_str) in &levels {
            logger.log(*level, msg)?;
        }

        // Dropping logger to force it to flush the log and close the file
        drop(logger);

        let file = File::open(logfile)?;
        let reader = BufReader::new(file);
        for (line, expected) in reader.lines().zip(levels.iter().map(|(_, l)| l)) {
            match line {
                Ok(line) => {
                    let (_timestamp_str, level, _message) =
                        parse_log_line(&line)
                        .expect("Failed to split the log line into parts");

                    assert_eq!(&level, expected);
                }
                Err(e) => return Err(e),
            }
        }

        Ok(())
    }
}
